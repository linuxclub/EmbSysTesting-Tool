#include "connectionuart.h"
#include <iostream>
using namespace std;

ConnectionUART::ConnectionUART(QWidget *parent ) :  QWidget(parent){
    setupUi(this);

    connect(CancelBtn, SIGNAL (clicked()), this, SLOT(slotClose()));
    connect(ConnectBtn, SIGNAL (clicked()), this, SLOT(storeSettings()));

    connect(databitsCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(storeDataBits()));
    connect(parityComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(storeParityBit()));
    connect(baudCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(storeBaudrate()));
    connect(directionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(storeDirection()));

    this->storeDataBits();
    this->storeBaudrate();
    this->storeDirection();
    this->storeParityBit();

  //  this->uartDriver = uartDriver;
}

void ConnectionUART::slotClose(){
    close();
}


void ConnectionUART::storeSettings(){
    cout << "\nstoreSetting" << endl;
    //store the combobox values

    cout << "Data bits :    "       << databits         << endl;
    cout << "Parity bit :   "       << parityBit        << endl;
    cout << "Parity enable  :   "   << parityEnable     << endl;
    cout << "Baudrate   :   "       << baudrate         << endl;
    cout << "Direction  :   "       << dataDirection    << endl;

   this->uartDriver = new Driver_Uart();
}

void ConnectionUART::storeDataBits(){
    databits = databitsCombobox->currentText().toUInt();
}

void ConnectionUART::storeParityBit(){
    int parity = parityComboBox->currentIndex();

    if(parity != 0){
        parityEnable = true;
        if(parity == 1)
            parityBit = true;
        else if(parity == 2)
            parityBit = false;
    }else
        parityEnable = false;
}

void ConnectionUART::storeBaudrate(){
        baudrate = baudCombobox->currentText().toULong();
}

void ConnectionUART::storeDirection(){
    int dir = directionComboBox->currentIndex();
    switch(dir){
    case DIR_TRANSMITTER:   dataDirection = DIR_TRANSMITTER; break;
    case DIR_RECEIVER:      dataDirection = DIR_RECEIVER; break;
    case DIR_Transceiver:   dataDirection = DIR_Transceiver; break;
    }
}
