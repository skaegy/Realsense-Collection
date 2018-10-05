#include "rssavethread.h"
using namespace cv;
rssavethread::rssavethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
    std::thread processing_thread([&]() {
       while (!abort) //While application is running
       {
           if (mlColor.size()>0 && mlDepth.size()>0 && mlImageName.size()>0 && mlColorPathName.size()>0 && mlDepthPathName.size()>0){
               QList<cv::Mat>::iterator itIm = mlColor.begin();
               QList<std::string>::iterator itName = mlImageName.begin();
               QList<QString>::iterator itColorPath =mlColorPathName.begin();
               QList<QString>::iterator itDepthPath =mlDepthPathName.begin();
               QString color_path = *itColorPath;
               QString depth_path = *itDepthPath;

               QFileInfo check_color(color_path);
               QFileInfo check_depth(depth_path);
               if (!check_color.exists())
               {
                   qDebug() << "Mkdir color::" << color_path;
                   QDir color_dir = QDir::root();
                   color_dir.mkpath(color_path);
               }
               if (!check_depth.exists())
               {
                   qDebug() << "Mkdir depth::";
                   QDir depth_dir = QDir::root();
                   depth_dir.mkpath(depth_path);
               }

               imwrite(color_path.toStdString() + *itName,  *itIm);
               itIm = mlDepth.begin();
               imwrite(depth_path.toStdString() + *itName, *itIm);

               mlColor.pop_front();
               mlDepth.pop_front();
               mlImageName.pop_front();
               mlColorPathName.pop_front();
               mlDepthPathName.pop_front();
           }
       }
    });
    processing_thread.detach();
}

rssavethread::~rssavethread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void rssavethread::stop(){
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void rssavethread::run(){    // To be added}

void rssavethread::save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat){
    if (!abort){
        Mat Buf_color_mat = color_mat.clone();
        Mat Buf_depth_mat = depth_mat.clone();
        char image_name[20];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        QString qColor_Path_name = QString("/home/skaegy/Data/RGBD/%1/%2/rgb/").arg(Subject_name).arg(Action_name);
        QString qDepth_Path_name = QString("/home/skaegy/Data/RGBD/%1/%2/depth/").arg(Subject_name).arg(Action_name);
        sprintf(image_name, "%13ld.png", currTime);
        mlColor.push_back(Buf_color_mat);
        mlDepth.push_back(Buf_depth_mat);
        mlColorPathName.push_back(qColor_Path_name);
        mlDepthPathName.push_back(qDepth_Path_name);
        mlImageName.push_back(image_name);
    }
}

void rssavethread::receive_RGBD_name(QString Subject, QString Action){
    Subject_name = Subject;
    Action_name = Action;
}
