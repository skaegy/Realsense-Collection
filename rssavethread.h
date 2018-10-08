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
    rssavethread(QObject *parent = 0);
    ~rssavethread();
    void stop();
    bool abort;

public slots:
    void save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat);
    void receive_RGBD_name(QString Subject, QString Action);

protected:
    void run();
private:
    QMutex mutex;
    QList<cv::Mat> mlColor; // List for color images
    QList<cv::Mat> mlDepth; // List for depth images
    QList<std::string> mlImageName; // List for images name (color == depth)
    QString qColor_path;
    QString qDepth_path;
};
#endif // RSSAVETHREAD_H
