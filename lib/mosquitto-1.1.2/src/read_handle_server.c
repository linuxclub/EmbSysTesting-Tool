/*
Copyright (c) 2009-2012 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <string.h>

#include <config.h>

#include <mosquitto_broker.h>
#include <mqtt3_protocol.h>
#include <memory_mosq.h>
#include <send_mosq.h>
#include <util_mosq.h>

extern unsigned int g_connection_count;

int mqtt3_handle_connect(struct mosquitto_db *db, struct mosquitto *context)
{
	char *protocol_name;
	uint8_t protocol_version;
	uint8_t connect_flags;
	char *client_id;
	char *will_payload = NULL, *will_topic = NULL;
	uint16_t will_payloadlen;
	struct mosquitto_message *will_struct = NULL;
	uint8_t will, will_retain, will_qos, clean_session;
	uint8_t username_flag, password_flag;
	char *username, *password = NULL;
	int i;
	int rc;
	struct _mosquitto_acl_user *acl_tail;
	int slen;
#ifdef WITH_TLS
	X509 *client_cert;
	X509_NAME *name;
	X509_NAME_ENTRY *name_entry;
#endif

	g_connection_count++;

	/* Don't accept multiple CONNECT commands. */
	if(context->state != mosq_cs_new){
		mqtt3_context_disconnect(db, context);
		return MOSQ_ERR_PROTOCOL;
	}

	if(_mosquitto_read_string(&context->in_packet, &protocol_name)){
		mqtt3_context_disconnect(db, context);
		return 1;
	}
	if(!protocol_name){
		mqtt3_context_disconnect(db, context);
		return 3;
	}
	if(strcmp(protocol_name, PROTOCOL_NAME)){
		if(db->config->connection_messages == true){
			_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid protocol \"%s\" in CONNECT from %s.",
					protocol_name, context->address);
		}
		_mosquitto_free(protocol_name);
		mqtt3_context_disconnect(db, context);
		return MOSQ_ERR_PROTOCOL;
	}
	_mosquitto_free(protocol_name);

	if(_mosquitto_read_byte(&context->in_packet, &protocol_version)){
		mqtt3_context_disconnect(db, context);
		return 1;
	}
	if((protocol_version&0x7F) != PROTOCOL_VERSION){
		if(db->config->connection_messages == true){
			_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid protocol version %d in CONNECT from %s.",
					protocol_version, context->address);
		}
		_mosquitto_send_connack(context, CONNACK_REFUSED_PROTOCOL_VERSION);
		mqtt3_context_disconnect(db, context);
		return MOSQ_ERR_PROTOCOL;
	}
	if((protocol_version&0x80) == 0x80){
		context->is_bridge = true;
	}

	if(_mosquitto_read_byte(&context->in_packet, &connect_flags)){
		mqtt3_context_disconnect(db, context);
		return 1;
	}
	clean_session = connect_flags & 0x02;
	will = connect_flags & 0x04;
	will_qos = (connect_flags & 0x18) >> 3;
	will_retain = connect_flags & 0x20;
	password_flag = connect_flags & 0x40;
	username_flag = connect_flags & 0x80;

	if(_mosquitto_read_uint16(&context->in_packet, &(context->keepalive))){
		mqtt3_context_disconnect(db, context);
		return 1;
	}

	if(_mosquitto_read_string(&context->in_packet, &client_id)){
		mqtt3_context_disconnect(db, context);
		return 1;
	}

	slen = strlen(client_id);
#ifdef WITH_STRICT_PROTOCOL
	if(slen > 23 || slen == 0){
#else
	if(slen == 0){
#endif
		_mosquitto_send_connack(context, CONNACK_REFUSED_IDENTIFIER_REJECTED);
		mqtt3_context_disconnect(db, context);
		return 1;
	}

	/* clientid_prefixes check */
	if(db->config->clientid_prefixes){
		if(strncmp(db->config->clientid_prefixes, client_id, strlen(db->config->clientid_prefixes))){
			_mosquitto_free(client_id);
			_mosquitto_send_connack(context, CONNACK_REFUSED_NOT_AUTHORIZED);
			mqtt3_context_disconnect(db, context);
			return MOSQ_ERR_SUCCESS;
		}
	}

	if(will){
		will_struct = _mosquitto_calloc(1, sizeof(struct mosquitto_message));
		if(!will_struct){
			_mosquitto_free(client_id);
			mqtt3_context_disconnect(db, context);
			return MOSQ_ERR_NOMEM;
		}
		if(_mosquitto_read_string(&context->in_packet, &will_topic)){
			_mosquitto_free(client_id);
			mqtt3_context_disconnect(db, context);
			return 1;
		}
		if(_mosquitto_read_uint16(&context->in_packet, &will_payloadlen)){
			_mosquitto_free(client_id);
			mqtt3_context_disconnect(db, context);
			return 1;
		}
		will_payload = _mosquitto_malloc(will_payloadlen);
		if(!will_payload){
			_mosquitto_free(client_id);
			mqtt3_context_disconnect(db, context);
			return 1;
		}

		rc = _mosquitto_read_bytes(&context->in_packet, will_payload, will_payloadlen);
		if(rc){
			_mosquitto_free(will_payload);
			_mosquitto_free(client_id);
			mqtt3_context_disconnect(db, context);
			return 1;
		}
	}

	if(username_flag){
		rc = _mosquitto_read_string(&context->in_packet, &username);
		if(rc == MOSQ_ERR_SUCCESS){
			if(password_flag){
				rc = _mosquitto_read_string(&context->in_packet, &password);
				if(rc == MOSQ_ERR_NOMEM){
					_mosquitto_free(username);
					_mosquitto_free(client_id);
					return MOSQ_ERR_NOMEM;
				}else if(rc == MOSQ_ERR_PROTOCOL){
					/* Password flag given, but no password. Ignore. */
					password_flag = 0;
				}
			}
		}else if(rc == MOSQ_ERR_NOMEM){
			_mosquitto_free(client_id);
			return MOSQ_ERR_NOMEM;
		}else{
			/* Username flag given, but no username. Ignore. */
			username_flag = 0;
		}
	}

#ifdef WITH_TLS
	if(context->listener->use_identity_as_username){
		if(!context->ssl){
			_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
			mqtt3_context_disconnect(db, context);
			_mosquitto_free(client_id);
			return MOSQ_ERR_SUCCESS;
		}
#ifdef WITH_TLS_PSK
		if(context->listener->psk_hint){
			/* Client should have provided an identity to get this far. */
			if(!context->username){
				_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
				mqtt3_context_disconnect(db, context);
				_mosquitto_free(client_id);
				return MOSQ_ERR_SUCCESS;
			}
		}else{
#endif /* WITH_TLS_PSK */
			client_cert = SSL_get_peer_certificate(context->ssl);
			if(!client_cert){
				_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
				mqtt3_context_disconnect(db, context);
				_mosquitto_free(client_id);
				return MOSQ_ERR_SUCCESS;
			}
			name = X509_get_subject_name(client_cert);
			if(!name){
				_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
				mqtt3_context_disconnect(db, context);
				_mosquitto_free(client_id);
				return MOSQ_ERR_SUCCESS;
			}

			i = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
			if(i == -1){
				_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
				mqtt3_context_disconnect(db, context);
				_mosquitto_free(client_id);
				return MOSQ_ERR_SUCCESS;
			}
			name_entry = X509_NAME_get_entry(name, i);
			context->username = _mosquitto_strdup((char *)ASN1_STRING_data(name_entry->value));
			if(!context->username){
				_mosquitto_free(client_id);
				return MOSQ_ERR_NOMEM;
			}
#ifdef WITH_TLS_PSK
		}
#endif /* WITH_TLS_PSK */
	}else{
#endif /* WITH_TLS */
		if(username_flag){
			rc = mosquitto_unpwd_check(db, username, password);
			context->username = username;
			context->password = password;
			if(rc == MOSQ_ERR_AUTH){
				_mosquitto_send_connack(context, CONNACK_REFUSED_BAD_USERNAME_PASSWORD);
				mqtt3_context_disconnect(db, context);
				_mosquitto_free(client_id);
				return MOSQ_ERR_SUCCESS;
			}else if(rc == MOSQ_ERR_INVAL){
				_mosquitto_free(client_id);
				return MOSQ_ERR_INVAL;
			}
		}

		if(!username_flag && db->config->allow_anonymous == false){
			_mosquitto_send_connack(context, CONNACK_REFUSED_NOT_AUTHORIZED);
			mqtt3_context_disconnect(db, context);
			_mosquitto_free(client_id);
			return MOSQ_ERR_SUCCESS;
		}
#ifdef WITH_TLS
	}
#endif

	/* Find if this client already has an entry. This must be done *after* any security checks. */
	for(i=0; i<db->context_count; i++){
		if(db->contexts[i] && db->contexts[i]->id && !strcmp(db->contexts[i]->id, client_id)){
			/* Client does match. */
			if(db->contexts[i]->sock == -1){
				/* Client is reconnecting after a disconnect */
				/* FIXME - does anything else need to be done here? */
			}else{
				/* Client is already connected, disconnect old version */
				if(db->config->connection_messages == true){
					_mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Client %s already connected, closing old connection.", client_id);
				}
			}
			db->contexts[i]->clean_session = clean_session;
			mqtt3_context_cleanup(db, db->contexts[i], false);
			db->contexts[i]->state = mosq_cs_connected;
			db->contexts[i]->address = _mosquitto_strdup(context->address);
			db->contexts[i]->sock = context->sock;
			db->contexts[i]->listener = context->listener;
			db->contexts[i]->last_msg_in = time(NULL);
			db->contexts[i]->last_msg_out = time(NULL);
			db->contexts[i]->keepalive = context->keepalive;
			db->contexts[i]->pollfd_index = context->pollfd_index;
#ifdef WITH_TLS
			db->contexts[i]->ssl = context->ssl;
#endif
			if(context->username){
				db->contexts[i]->username = _mosquitto_strdup(context->username);
			}
			context->sock = -1;
#ifdef WITH_TLS
			context->ssl = NULL;
#endif
			context->state = mosq_cs_disconnecting;
			context = db->contexts[i];
			if(context->msgs){
				mqtt3_db_message_reconnect_reset(context);
			}
			break;
		}
	}

	context->id = client_id;
	context->clean_session = clean_session;
	context->ping_t = 0;

#ifdef WITH_PERSISTENCE
	if(!clean_session){
		db->persistence_changes++;
	}
#endif
	context->will = will_struct;
	if(context->will){
		context->will->topic = will_topic;
		if(will_payload){
			context->will->payload = will_payload;
			context->will->payloadlen = will_payloadlen;
		}else{
			context->will->payload = NULL;
			context->will->payloadlen = 0;
		}
		context->will->qos = will_qos;
		context->will->retain = will_retain;
	}

	/* Associate user with its ACL, assuming we have ACLs loaded. */
	if(db->acl_list){
		acl_tail = db->acl_list;
		while(acl_tail){
			if(context->username){
				if(acl_tail->username && !strcmp(context->username, acl_tail->username)){
					context->acl_list = acl_tail;
					break;
				}
			}else{
				if(acl_tail->username == NULL){
					context->acl_list = acl_tail;
					break;
				}
			}
			acl_tail = acl_tail->next;
		}
	}else{
		context->acl_list = NULL;
	}

	if(db->config->connection_messages == true){
		_mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "New client connected from %s as %s.", context->address, client_id);
	}

	context->state = mosq_cs_connected;
	return _mosquitto_send_connack(context, CONNACK_ACCEPTED);
}

int mqtt3_handle_disconnect(struct mosquitto_db *db, struct mosquitto *context)
{
	if(!context){
		return MOSQ_ERR_INVAL;
	}
	if(context->in_packet.remaining_length != 0){
		return MOSQ_ERR_PROTOCOL;
	}
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received DISCONNECT from %s", context->id);
	context->state = mosq_cs_disconnecting;
	mqtt3_context_disconnect(db, context);
	return MOSQ_ERR_SUCCESS;
}


int mqtt3_handle_subscribe(struct mosquitto_db *db, struct mosquitto *context)
{
	int rc = 0;
	int rc2;
	uint16_t mid;
	char *sub;
	uint8_t qos;
	uint8_t *payload = NULL, *tmp_payload;
	uint32_t payloadlen = 0;
	int len;
	char *sub_mount;

	if(!context) return MOSQ_ERR_INVAL;
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received SUBSCRIBE from %s", context->id);
	/* FIXME - plenty of potential for memory leaks here */

	if(_mosquitto_read_uint16(&context->in_packet, &mid)) return 1;

	while(context->in_packet.pos < context->in_packet.remaining_length){
		sub = NULL;
		if(_mosquitto_read_string(&context->in_packet, &sub)){
			if(payload) _mosquitto_free(payload);
			return 1;
		}

		if(sub){
			if(_mosquitto_read_byte(&context->in_packet, &qos)){
				_mosquitto_free(sub);
				if(payload) _mosquitto_free(payload);
				return 1;
			}
			if(qos > 2){
				_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid QoS in subscription command from %s, disconnecting.",
					context->address);
				_mosquitto_free(sub);
				if(payload) _mosquitto_free(payload);
				return 1;
			}
			if(_mosquitto_fix_sub_topic(&sub)){
				_mosquitto_free(sub);
				if(payload) _mosquitto_free(payload);
				return 1;
			}
			if(!strlen(sub)){
				_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Empty subscription string from %s, disconnecting.",
					context->address);
				_mosquitto_free(sub);
				if(payload) _mosquitto_free(payload);
				return 1;
			}
			if(context->listener && context->listener->mount_point){
				len = strlen(context->listener->mount_point) + strlen(sub) + 1;
				sub_mount = _mosquitto_calloc(len, sizeof(char));
				if(!sub_mount){
					_mosquitto_free(sub);
					if(payload) _mosquitto_free(payload);
					return MOSQ_ERR_NOMEM;
				}
				snprintf(sub_mount, len, "%s%s", context->listener->mount_point, sub);
				_mosquitto_free(sub);
				sub = sub_mount;

			}
			_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "\t%s (QoS %d)", sub, qos);

			rc2 = mqtt3_sub_add(db, context, sub, qos, &db->subs);
			if(rc2 == MOSQ_ERR_SUCCESS){
				if(mqtt3_retain_queue(db, context, sub, qos)) rc = 1;
			}else if(rc2 != -1){
				rc = rc2;
			}
			_mosquitto_free(sub);

			tmp_payload = _mosquitto_realloc(payload, payloadlen + 1);
			if(tmp_payload){
				payload = tmp_payload;
				payload[payloadlen] = qos;
				payloadlen++;
			}else{
				if(payload) _mosquitto_free(payload);

				return MOSQ_ERR_NOMEM;
			}
		}
	}

	if(_mosquitto_send_suback(context, mid, payloadlen, payload)) rc = 1;
	_mosquitto_free(payload);
	
#ifdef WITH_PERSISTENCE
	db->persistence_changes++;
#endif

	return rc;
}

int mqtt3_handle_unsubscribe(struct mosquitto_db *db, struct mosquitto *context)
{
	uint16_t mid;
	char *sub;

	if(!context) return MOSQ_ERR_INVAL;
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received UNSUBSCRIBE from %s", context->id);

	if(_mosquitto_read_uint16(&context->in_packet, &mid)) return 1;

	while(context->in_packet.pos < context->in_packet.remaining_length){
		sub = NULL;
		if(_mosquitto_read_string(&context->in_packet, &sub)){
			return 1;
		}

		if(sub){
			_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "\t%s", sub);
			mqtt3_sub_remove(db, context, sub, &db->subs);
			_mosquitto_free(sub);
		}
	}
#ifdef WITH_PERSISTENCE
	db->persistence_changes++;
#endif

	return _mosquitto_send_command_with_mid(context, UNSUBACK, mid, false);
}

