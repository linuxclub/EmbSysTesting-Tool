/**
 * Module:	Driver_I2C.h
 * @author	Artjom Siebert
 * @date	14.05.2014
 * 
 */ 

#include "Driver_I2C.h"

Driver_I2C::Driver_I2C(){}

Driver_I2C::Driver_I2C(S_driver){}

bool Driver_I2C::DUI_Initialization(S_driver){return true;}

bool Driver_I2C::SendData(char *s) {return true;}
	 
R_data Driver_I2C::ReceiveData(){} 
	
