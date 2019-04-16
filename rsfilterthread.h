#ifndef RSFILTERTHREAD_H
#define RSFILTERTHREAD_H

#include <rs.hpp>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QDateTime>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "settings.h"

class rsFilterThread : public QThread
{
    Q_OBJECT
public:
    rsFilterThread(QObject *parent = nullptr);
    ~rsFilterThread();
    bool abort;
    void startFilter();
    void stop();

Q_SIGNALS:
    void sendColorFiltered(cv::Mat &color_mat, qint64 timestamp);
    void sendDepthFiltered(cv::Mat &depth_mat, qint64 timestamp);
    void sendRGBDFiltered(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp);

public slots:
    void receiveColorFrame(rs2::frame ColorFrame);
    void receiveDepthFrame(rs2::frame DepthFrame);
    void receiveRGBDFrame(rs2::frame ColorFrame, rs2::frame DepthFrame, qint64 timestamp);
    void receiveSaveImageSignal();
    void receive_temporalFilter_params(bool persistency);

private:
    cv::Mat rsDepthFrame2Mat(rs2::frame DepthFrame);
    cv::Mat rsColorFrame2Mat(rs2::frame ColorFrame);

protected:
    void run();

private:
    QMutex mutex;
    // Parameters
    rs2::spatial_filter spat_filter;    // Spatial filter of depth image
    rs2::temporal_filter temp_filter;   // Temporal filter of depth image
    rs2::hole_filling_filter holefill_filter;
    rs2::frame mColorFrame;
    rs2::frame mDepthFrame;
    //const int CAPACITY = 3;
    bool mSaveImageFlag = false;
    bool mRS_tempPersistency = false;

    std::list<rs2::frame> mlColorFrame;
    std::list<rs2::frame> mlDepthFrame;
    std::list<qint64> mlTimestamp;

};
#endif // RSFILTERTHREAD_H
