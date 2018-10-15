#ifndef UDPTHREAD_H
#define UDPTHREAD_H
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QDateTime>
#include <QFile>

#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>

namespace Ui {
class rsCollectData;
class rsCaptureThread;
class rssavethread;
class udpthread;
class CharacteristicInfo;
class DeviceInfo;
class ServiceInfo;
}

class udpthread : public QThread
{
    Q_OBJECT
public:
    udpthread(QObject *parent = 0);
    ~udpthread();
    void stop();
    void startSync();
    bool abort;

public slots:
    void receive_Subject_Action(QString Subject, QString Action);

protected:
    void run();

private:
    QMutex mutex;
    Ui::rsCollectData* mpUI;
    struct sockaddr_in addr;
    struct sockaddr_in src;
    struct timeval tv;
    const unsigned long buf_size = 128;
    int sockfd;
    QString SubjectName, ActionName;
};

#endif // UDPTHREAD_H
