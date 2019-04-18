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
   socklen_t addr_len=sizeof(addr);
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;       // Use IPV4
   addr.sin_port   = htons(PORT_SCAN);    //
   addr.sin_addr.s_addr = htonl(INADDR_ANY);

   // ========= set listen interval =========== //
   tv.tv_sec  = 0;
   tv.tv_usec = PORT_MS*1000;  // millisecond to usecond
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
    memset(buffer, 0, BUF_SIZE);
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QString filename = QString("%1/data/Yao/Capture/VICON/time_%2.txt").arg(QDir::homePath()).arg(currTime);
    QFile timefile(filename);
    timefile.open(QIODevice::ReadWrite);
    QTextStream timestream(&timefile);
    while(!abort){
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));
        int sz = recvfrom(sockfd, buffer, BUF_SIZE, 0, reinterpret_cast<sockaddr*>(&src), &src_len);
        if (sz > 0){
            QByteArray qBuffer(QByteArray::fromRawData(buffer, 15)); //convert to QByteArray

            // The cliend should only send '1' or '2' for communication
            // '1' -- start all;  '2' -- stop all
            // char receiveCh = buffer[0];
            int receiveKey = qBuffer.left(1).toInt();
            qint64 receiveTimestamp = qBuffer.right(14).toLongLong();

            if (receiveKey==1 || receiveKey==2){
                // Timestamp of the received signal
                qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                qDebug() << "[UDP] SIGNAL: " << receiveKey <<  " (linux)~" << currTime
                         << " (VICON)~" << receiveTimestamp;


                if (receiveKey == 1){
                    emit udp4startALL();
                    timestream << "START " << mActionName << mIndexName << " " << currTime << " " << receiveTimestamp << endl;
                }
                else if(receiveKey == 2){
                    emit udp4stopALL();
                    timestream << "STOP " << mActionName << mIndexName << " " << currTime << " " << receiveTimestamp << endl;
                }
            }
        }
        usleep(100);
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
