#ifndef RSSAVETHREAD_H
#define RSSAVETHREAD_H
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QTextStream>
#include <QFileDialog>
#include <QDir>
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
    void save_color_mat(cv::Mat color_mat);
    void save_depth_mat(cv::Mat depth_mat);
protected:
    void run();
private:
    QMutex mutex;
};
#endif // RSSAVETHREAD_H
