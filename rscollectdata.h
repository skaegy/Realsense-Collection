#ifndef RSCOLLECTDATA_H
#define RSCOLLECTDATA_H
#include <QWidget>
#include <QObject>
#include <QTextStream>
#include <QFileDialog>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QObject>
#include <QImageWriter>
#include <QDateTime>
#include <QListWidgetItem>
#include <QtEndian>
#include <QPainter>
#include <QVarLengthArray>
#include <QList>

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QBluetoothServiceInfo>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <rs.hpp>
#include "rscapturethread.h"
#include "rssavethread.h"
#include "deviceinfo.h"
#include "serviceinfo.h"
#include "characteristicinfo.h"


namespace Ui {
class rsCollectData;
}

class rsCollectData : public QWidget
{
    Q_OBJECT

public:
    explicit rsCollectData(QWidget *parent = 0);
    ~rsCollectData();

    const QString ear_uuid = "47442014-0f63-5b27-9122-728099603712";
    const QString IMU_uuid = "47442020-0f63-5b27-9122-728099603712";

private slots:
    // UI
    void on_Button_openRSThread_clicked();
    void on_Button_closeRSThread_clicked();
    void on_Button_saveRGBD_clicked();
    void on_Button_stopSaveRGBD_clicked();
    void on_Button_startSaveBLE_clicked();
    void on_Button_stopSaveBLE_clicked();

    // RGB-D
    void show_color_mat(cv::Mat &color_mat);
    void show_depth_mat(cv::Mat &depth_mat);
    void show_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat);

    // BLE plot
    void init_BLE_graph();
    void show_BLE_graph();

    // BLE
    void startDeviceDiscovery();
    void scanServices(const QString &address);
    void disconnectFromDevice();
    void updateIMUvalue(const QLowEnergyCharacteristic &ch, const QByteArray &value);
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void addDevice(const QBluetoothDeviceInfo&);
    void itemActivated(QListWidgetItem *item);
    // QBluetoothDeviceDiscoveryAgent related
    void deviceScanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error);
    // QLowEnergyController realted
    void addLowEnergyService(const QBluetoothUuid &uuid);
    void deviceConnected();
    void errorReceived(QLowEnergyController::Error);
    void serviceScanDone();
    void deviceDisconnected();
    // QLowEnergyService related
    void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);

Q_SIGNALS:
    void devicesUpdated();
    void servicesUpdated();
    void characteristicsUpdated();
    void updateChanged();
    void stateChanged();
    void disconnected();
    void randomAddressChanged();
    void DataReceived(); // For plot BLE received data
    void send_RGBD_name(QString Subject, QString Action);

private:
    // UI
    Ui::rsCollectData *ui;
    // BLE
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    DeviceInfo currentDevice;
    QBluetoothLocalDevice *localDevice;
    QList<QObject*> devices;
    QList<QObject*> m_services;
    QList<QObject*> m_characteristics;
    QString m_previousAddress;
    QString m_message;
    bool connected;
    QLowEnergyController *controller;
    QLowEnergyService *m_eARservice;
    QLowEnergyDescriptor m_notificationDesc;
    QLowEnergyCharacteristic m_IMUchar;
    bool m_deviceScanState;
    // Thread
    rsCaptureThread* rsCapture;
    rsFilteredThread* rsFiltered;
    rssavethread* rsSave;

    // parameters
    // ---- RGBD ---- //
    bool save_flag = false;
    int frame_cnt = 1; int fps = 0; // To calculate the fps of the rgb-d images
    uint LastTimeT = 0;
    // ---- BLE ---- //
    int save_BLE_cnt = 0;
    bool save_BLE_flag = false;
    bool BLEData_flag = false;
    qint64 BLEReceiveData[10] = {0}; // Received data from BLE sensor
    QList<qint64> BLEReceiveDataSet; // Received data for save from BLE sensor
    // ---- BLE plot ----//
    int key_ACC = 1, key_GYR = 1, key_MAG = 1;  // xlabel of BLE graph

};

#endif // RSCOLLECTDATA_H
