#ifndef RSCAPTURETHREAD_H
#define RSCAPTURETHREAD_H
#include <rs.hpp>
#include <QThread>
#include <QMutex>
#include <QObject>
#include <QImage>
#include <QMetaType>
#include <QDebug>
#include <QDateTime>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "settings.h"

class rsCaptureThread : public QThread
{
    Q_OBJECT
public:
    rsCaptureThread(QObject *parent = nullptr);
    ~rsCaptureThread();
    void startCollect();
    void stop();
    bool abort;
    rs2::sensor depth_sensor;

Q_SIGNALS:
    void sendColorMat(cv::Mat &color_mat);
    void sendDepthMat(cv::Mat &depth_mat);
    void sendRGBDMat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp);
    void sendRGBDFrame(rs2::frame color_frame, rs2::frame depth_frame, qint64 timestamp);

private:
    cv::Mat rsDepthFrame2Mat(rs2::frame DepthFrame);
    cv::Mat rsColorFrame2Mat(rs2::frame ColorFrame);

protected:
    void run();

private:
    QMutex mutex;
    // Params for RS
    rs2::pipeline pipe;
    rs2::config rs_cfg;
    rs2::colorizer color_map;
    rs2::pipeline_profile rs_device;
    rs2::device selected_device;
    cv::Mat color_mat, depth_mat;
};

#endif // RSCAPTURETHREAD_H
