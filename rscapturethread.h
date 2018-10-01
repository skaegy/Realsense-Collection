#ifndef RSCAPTURETHREAD_H
#define RSCAPTURETHREAD_H
#include <rs.hpp>
#include <QThread>
#include <QMutex>
#include <QObject>
#include <QImage>
#include <QMetaType>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>


#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define IN_FRAME 30

class rsCaptureThread : public QThread
{
    Q_OBJECT
public:
    rsCaptureThread(QObject *parent = 0);
    ~rsCaptureThread();
    void startCollect();
    void stop();
    bool abort;
signals:
    void sendColorMat(cv::Mat color_mat);
    void sendDepthMat(cv::Mat depth_mat);
protected:
    void run();
private:
    QMutex mutex;
    rs2::pipeline pipe;
    cv::Mat color_mat, depth_mat;
    QImage qcolorshow, qdepthshow;
    cv::Mat rsDepthFrame2Mat(rs2::frame DepthFrame);
    cv::Mat rsColorFrame2Mat(rs2::frame ColorFrame);
};

class rsFilteredThread : public QThread
{
    Q_OBJECT
public:
    rsFilteredThread(QObject *parent = 0);
    ~rsFilteredThread();
    bool abort;
    void startFilter();
    void stop();
protected:
    void run();
signals:
    void sendColorFiltered(cv::Mat color_mat);
    void sendDepthFiltered(cv::Mat depth_mat);
    void sendDepthShow(cv::Mat depth_show_mat);
private:
    QMutex mutex;
    cv::Mat rsDepthFrame2Mat(rs2::frame DepthFrame);
    cv::Mat rsColorFrame2Mat(rs2::frame ColorFrame);
};
#endif // RSCAPTURETHREAD_H
