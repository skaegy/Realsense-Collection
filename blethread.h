#ifndef BLETHREAD_H
#define BLETHREAD_H

#include <QThread>
#include <QMutex>
#include <QString>
#include <QList>
#include <QVector>
#include <QDateTime>
#include <QListWidgetItem>

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QBluetoothServiceInfo>

#include "settings.h"
#include "deviceinfo.h"
#include "serviceinfo.h"
#include "characteristicinfo.h"

class blethread : public QThread
{
    Q_OBJECT
public:
    blethread(QObject *parent = nullptr);
    ~blethread();
    void stop();
    bool abort;

Q_SIGNALS:
    // -- BLE Signals -- //
    void devicesUpdated();
    void stateChanged();
    void characteristicsUpdated();
    void servicesUpdated();
    // -- BLE Data signals --//
    void resetGraph();
    void updateGraph(QVector<qint64> BLEdata);
    void sendItem(QListWidgetItem *item);

public slots:
    // -- BLE device -- //
    void startDeviceDiscovery();
    void deviceScanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void deviceConnected();
    void addDevice(const QBluetoothDeviceInfo &info);
    // -- BLE controller -- //
    void errorReceived(QLowEnergyController::Error /*error*/);
    // -- BLE service --//
    void scanServices(const QString &address);
    void addLowEnergyService(const QBluetoothUuid &serviceUuid);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState s);

    // -- QUIT BLE -- //
    void disconnectFromDevice();
    void deviceDisconnected();
    // -- BLE data -- //
    void updateIMUvalue(const QLowEnergyCharacteristic &ch, const QByteArray &value);
    void receiveSaveFlag(bool save_ble_flag);
    void receiveFileName(QString Subject, QString Action, QString Index);

protected:
    void run();

public:
    // BLE
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    DeviceInfo currentDevice;
    QBluetoothLocalDevice *localDevice;
    QList<QObject*> devices; //Y
    QList<QObject*> m_services; //Y
    QList<QObject*> m_characteristics; //Y
private:
    // BLE
    bool connected;
    QLowEnergyController *controller;
    QLowEnergyService *m_eARservice;
    QLowEnergyDescriptor m_notificationDesc;
    QLowEnergyCharacteristic m_IMUchar;
    bool m_deviceScanState;

    //qint64 BLEReceiveData[10] = {0}; // Received data from BLE sensor
    QList<qint64> BLEReceiveDataSet; // Received data (qint64) for save from BLE sensor
    QList<QString> BLEReceiveDatas;  // Received data (QString) for save from BLE sensor
    QString mSubjectName, mActionName, mIndexName;
    bool mSaveFlag;

    QMutex mutex;
};

#endif // BLETHREAD_H
