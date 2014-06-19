/**
* Module:	DriverUsage.h
* Author:	Artjom Siebert
* Date:		14.05.2014
* 
* Description:
* This module defines the Interface between Middleware and the connection to Hardware (drivers). 
* Implement this Interface, if you develop new drivers. 
*/

#ifndef __DRIVER_USAGE_
#define __DRIVER_USAGE_
#include "../Middleware/header/Middleware_Structs.h"

class DriverUsageInterface{
	public:
	/**
	 * @brief
	 * @param S_driver
	 * @return
	 */
	 virtual bool DUI_Initialization(S_driver) = 0;

	/**
	 * @brief
	 * @return
	 */
	 virtual bool SendData(char *s) = 0;
	 
	/**
	 * @brief
	 * @return
	 */
	 virtual R_data ReceiveData() = 0; 
	 		
};
#endif /*__DRIVER_USAGE_*/

