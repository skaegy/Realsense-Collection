#ifndef BLE_H
#define BLE_H

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QThread>
#include <QMutex>
#include <QObject>

class BLEprocessThread : public QThread
{
    Q_OBJECT

public:
    BLEprocess(QWidget *parent = 0);
    ~BLEprocess();
    void startScan();
    void startPair();
signals:

public slots:

private slots:
    void addDevice(const QBluetoothDeviceInfo&);

private:
    bool abort;

    QBluetoothLocalDevice *localDevice;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;


};

#endif // BLE_H
