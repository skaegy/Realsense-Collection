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
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>

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
#include "rsfilterthread.h"
#include "rssavethread.h"
#include "deviceinfo.h"
#include "serviceinfo.h"
#include "characteristicinfo.h"
#include "udpthread.h"
#include "blethread.h"


namespace Ui {
class rsCollectData;
class rsCaptureThread;
class rssavethread;
class udpthread;
class blethread;
class CharacteristicInfo;
class DeviceInfo;
class ServiceInfo;
}


class rsCollectData : public QWidget
{
    Q_OBJECT

public:
    explicit rsCollectData(QWidget *parent = nullptr);
    ~rsCollectData();

private slots:
    // UI
    void on_Button_openRSThread_clicked();
    void on_Button_closeRSThread_clicked();
    void on_Button_saveRGBD_clicked();
    void on_Button_stopSaveRGBD_clicked();
    void on_Button_startSaveBLE_clicked();
    void on_Button_stopSaveBLE_clicked();
    void on_Button_UDP_clicked();
    void on_Button_StopUDP_clicked();
    void on_Button_saveImage_clicked();

    void on_Text_subject_name_textChanged();
    void on_Text_subject_action_textChanged();
    void on_Text_subject_index_textChanged();
    void on_checkPlotGraph_stateChanged(int arg1);

    // RGB-D
    void show_color_mat(cv::Mat &color_mat, qint64 timestamp);
    void show_depth_mat(cv::Mat &depth_mat, qint64 timestamp);
    void show_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp);

    // BLE plot
    void init_BLE_graph();
    void show_BLE_graph(QVector<qint64> BLEdata);
    void reset_BLE_graph();

    // BLE
    void itemActivated(QListWidgetItem *item);
    void receiveItem(QListWidgetItem *item);
    void on_Button_ScanBLE_clicked();
    void on_Button_QuitBLE_clicked();

    // SAVE&STOP ALL
    void on_Button_SAVEALL_clicked();
    void on_Button_SAVESTOPALL_clicked();



    void on_combo_persistency_currentIndexChanged(int index);

    void on_spatial_alpha_valueChanged(int value);

    void on_spatial_delta_valueChanged(int value);

    void on_spatial_mag_valueChanged(int value);

    void on_temporal_alpha_valueChanged(int value);

    void on_temporal_delta_valueChanged(int value);

Q_SIGNALS:
    void send_RGBD_name(QString Subject, QString Action, QString Index);
    void send_BLEsave_flag(bool save_ble_flag);

private:
    // UI
    Ui::rsCollectData *ui;
    int InitIdx = 1;
    bool mbStartRealsense = false;
    bool mbShowGraph = true;
    bool mbRS_persistency = false;

    // Thread
    rsCaptureThread* rsCapture;
    rsFilterThread* rsFilter;
    rssavethread* rsSave;
    udpthread* udpSync;
    blethread* EARSensor;

    // parameters
    // ---- RGBD fps ---- //
    uint mColor_frame_cnt = 0;  uint mDepth_frame_cnt = 0;
    qint64 mLastColorTimeT = 0; qint64 mLastDepthTimeT = 0;
    uint mSumColorTime = 0;     uint mSumDepthTime = 0;

    bool save_RGB_flag = false;
    // ---- BLE ---- //
    int save_BLE_cnt = 0;
    bool save_BLE_flag = false;
    qint64 BLEReceiveData[10] = {0}; // Received data from BLE sensor
    QList<qint64> BLEReceiveDataSet; // Received data for save from BLE sensor
    qint64 mLastEarTime = 0;
    qint64 mSumEarTime = 0;

    // ---- BLE plot ----//
    int key_ACC = 1, key_GYR = 1, key_MAG = 1;  // xlabel of BLE graph



};

#endif // RSCOLLECTDATA_H
