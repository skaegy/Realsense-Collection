#include "ble.h"

using namespace QBluetoothDeviceDiscoveryAgent;
using namespace QBluetoothDeviceInfo;

BLEprocessThread::BLEprocessThread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}

void BLEprocessThread::startScan(){
    discoveryAgent->start();
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
}

