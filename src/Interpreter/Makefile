CC=g++
CFLAGS=-c -Wall -ggdb -I/usr/local/include  -I../../lib/mosquitto-1.1.2/lib -I../../lib/mosquitto-1.1.2/lib/cpp
LDFLAGS=-L/usr/local/lib -lwiringPi -L../../lib/mosquitto-1.1.2/lib ../../lib/mosquitto-1.1.2/lib/cpp/libmosquittopp.so.1 ../../lib/mosquitto-1.1.2/lib/libmosquitto.so.1
SOURCES= ../Middleware/MQTTv3.cpp ../Middleware/Message.cpp ../Middleware/Subscriber.cpp ../Middleware/Publisher.cpp HardwareLayer/Connector.cpp  HardwareLayer/Driver_Uart.cpp main.cpp
DISTDIR=../../bin/
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=run

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(DISTDIR)$@

.cpp.o:   
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)

