#include "rssavethread.h"
using namespace cv;
rssavethread::rssavethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}
rssavethread::~rssavethread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}
void rssavethread::stop(){
    abort = true;
}
void rssavethread::run(){
    // To be added
}
void rssavethread::save_color_mat(cv::Mat color_mat){
    char color_file_name[50];
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
    QTextStream(stdout) << "Saving " << color_file_name << endl;
    imwrite(color_file_name, color_mat);
}
void rssavethread::save_depth_mat(cv::Mat depth_mat){
    char depth_file_name[50];
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
    QTextStream(stdout) << "Saving " << depth_file_name << endl;
    imwrite(depth_file_name, depth_mat);
}
