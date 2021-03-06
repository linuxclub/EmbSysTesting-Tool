/**
* Module:	Driver_SPI.cpp
* @author:	Artjom Siebert
* @date:	17.05.2014
* 
* @brief: 
*/

#include "Driver_SPI.h"
#include <stdlib.h>
#include <iostream>
#include <errno.h>

using namespace std;  

Driver_SPI::Driver_SPI(int channel, int speed){
	this->channel = channel;
	this->speed = speed;
}

Driver_SPI::Driver_SPI(S_driver){}

bool Driver_SPI::DUI_Initialization(S_driver)
{

	system("gpio load spi"); //Before you can use SPI, you may need to use the
				 //gpio utility to load the SPI drivers into the kernel 	

	if(wiringPiSPISetup(channel, speed) == SPI_SETUP_ERROR){
		cout << errno <<endl;
		return false;
	}
	else
		return true;
}

bool Driver_SPI::SendData(char *s) {return true;}
	 
R_data Driver_SPI::ReceiveData(){} 
