#ifndef RSCOLLECTDATA_H
#define RSCOLLECTDATA_H
#include <QWidget>
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

#include <QtBluetooth/qbluetoothaddress.h>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <rs.hpp>
#include "rscapturethread.h"


namespace Ui {
class rsCollectData;
}

class rsCollectData : public QWidget
{
    Q_OBJECT

public:
    explicit rsCollectData(QWidget *parent = 0);
    ~rsCollectData();

private slots:
    // UI
    void on_OpenRSButton_clicked();
    void on_Button_openRSThread_clicked();
    void on_Button_closeRSThread_clicked();
    void on_Button_saveRGBD_clicked();
    void on_Button_stopSaveRGBD_clicked();

    // RGB-D
    void show_color_mat(cv::Mat color_mat);
    void show_depth_mat(cv::Mat depth_mat);
    void save_color_mat(cv::Mat color_mat);
    void save_depth_mat(cv::Mat depth_mat);

    // BLE
    void startScan();
    void scanFinished();
    void addDevice(const QBluetoothDeviceInfo&);
    void itemActivated(QListWidgetItem *item);
    void on_Button_RescanBLE_clicked();

private:
    Ui::rsCollectData *ui;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothLocalDevice *localDevice;
    rsCaptureThread* rsCapture;
    rsFilteredThread* rsFiltered;
    // parameters
    bool save_flag = false;
    int frame_cnt = 1;
    int save_color_cnt = 1, save_depth_cnt = 1;
    int LastTimeT = 0;
    int fps = 0;

    rs2::pipeline pipe;

};

#endif // RSCOLLECTDATA_H
