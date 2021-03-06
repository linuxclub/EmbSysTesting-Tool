/*
 * @author	Artjom Siebert
 * @date	14.05.2014
 * @brief
 * 
 * Driver_I2C.h:
 *
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as 
 *    published by the Free Software Foundation, either version 3 of 
 *    the License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General 
 *    Public License along with wiringPi.  
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */
 
#ifndef __DRIVER_I2C_
#define __DRIVER_I2C_
#include "DriverUsage.h"
class Driver_I2C: public DriverUsageInterface{
public:
	Driver_I2C(); 
	Driver_I2C(S_driver); 
	bool DUI_Initialization(S_driver) ;
	bool SendData(char *s);
	R_data ReceiveData(); 
	
};

#endif /*__DRIVER_I2C_*/
