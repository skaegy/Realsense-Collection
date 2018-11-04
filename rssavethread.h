#ifndef RSSAVETHREAD_H
#define RSSAVETHREAD_H
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QFileDialog>
#include <QImageWriter>
#include <QDir>
#include <QList>
#include <QDebug>
#include <QFileInfo>
#include <rs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
class rssavethread : public QThread
{
    Q_OBJECT
public:
    rssavethread(QObject *parent = nullptr);
    ~rssavethread();
    void stop();
    bool abort;

public slots:
    void save_color_mat(cv::Mat &color_mat, qint64 timestamp);
    void save_depth_mat(cv::Mat &depth_mat, qint64 timestamp);
    void save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp);
    void receive_RGBD_name(QString Subject, QString Action, QString Index);

protected:
    void run();

private:
    QMutex mutex;
    QList<cv::Mat> mlColor; // List for color images
    QList<cv::Mat> mlDepth; // List for depth images
    QList<std::string> mlImageName; // RSCAPTURE --> List for images name (color == depth)
    QList<std::string> mlColorName;
    QList<std::string> mlDepthName;
    QString mColor_path;
    QString mDepth_path;
    int ReceiveFrameCnt = 0;
    int SaveFrameCnt = 0;
    int mFrameTobeSaved;
    bool mbSavedFinish;
};
#endif // RSSAVETHREAD_H
