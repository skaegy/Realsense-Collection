#include "udpthread.h"

udpthread::udpthread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}

udpthread::~udpthread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void udpthread::startSync(){
    //========== create socket ===========//
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==sockfd){
        qDebug() << "Failed to create socket";
    }

    // ========= set address and port =========//
   socklen_t          addr_len=sizeof(addr);
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;       // Use IPV4
   addr.sin_port   = htons(PORT_SCAN);    //
   addr.sin_addr.s_addr = htonl(INADDR_ANY);

   // ========= set listen interval =========== //
   tv.tv_sec  = 0;
   tv.tv_usec = 20*1000;  // millisecond to usecond
   setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(struct timeval));

   //========== Bind ports. ===========//
   if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), addr_len) == -1){
       qDebug() << "Failed to bind socket on port";
   }
   else{
       qDebug() << "UDP connection is established";
   }

   start();
}

void udpthread::run(){
    char buffer[128];
    memset(buffer, 0, buf_size);
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QString filename = QString("/home/skaegy/Data/VICON/time_%1.txt").arg(currTime);
    QFile timefile(filename);
    timefile.open(QIODevice::ReadWrite);
    while(!abort){
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));
        int sz = recvfrom(sockfd, buffer, buf_size, 0, reinterpret_cast<sockaddr*>(&src), &src_len);
        usleep(1000);
        if (sz > 0){
            // Timestamp of the received signal
            qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
            qDebug() << "Received" << currTime;

            QTextStream timestream(&timefile);
            timestream << mSubjectName << " " << mActionName << mIndexName << " " << currTime << endl;
        }
    }
}

void udpthread::stop(){
    abort = true;
    close(sockfd);
}

void udpthread::receive_Subject_Action(QString Subject, QString Action, QString Index){
    mSubjectName = Subject;
    mActionName = Action;
    mIndexName = Index;
}
