#include "blethread.h"

blethread::blethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    devices.clear();
    m_characteristics.clear();
    m_services.clear();
    mSaveFlag = false;
    mutex.unlock();

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &blethread::deviceScanFinished);
    //connect(ui->Button_ScanBLE, SIGNAL(clicked()), this, SLOT(startDeviceDiscovery()));
    //connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            //this, SLOT(addDevice(QBluetoothDeviceInfo)));
    //connect(ui->List_BLE, SIGNAL(itemActivated(QListWidgetItem*)),
            //this, SLOT(itemActivated(QListWidgetItem*)));

    //connect(ui->Button_QuitBLE,SIGNAL(clicked()), this, SLOT(disconnectFromDevice()));
    //connect(this,SIGNAL(DataReceived()),this,SLOT(show_BLE_graph()));
}

blethread::~blethread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

//----------------------//
// ---  BLE DEVICE  --- //
//----------------------//
void blethread::startDeviceDiscovery(){
    qDeleteAll(devices);
    devices.clear();

    QTextStream(stdout) << "[BLE] Scanning for devices ..." << endl;
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    if (discoveryAgent->isActive()) {
        m_deviceScanState = true;
    }
}

void blethread::deviceScanFinished()
{
    m_deviceScanState = false;
    if (devices.isEmpty())
        qDebug() << "[scanFinished]No Low Energy devices found...";
    else
        qDebug() << "[scanFinished]Done! Scan Again!";
}

void blethread::deviceConnected()
{
    connected = true;
    controller->discoverServices();
}

void blethread::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        QTextStream(stdout) << "[deviceScanError]The Bluetooth adaptor is powered off, power it on before doing discovery." << endl;
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        QTextStream(stdout) << "[deviceScanError]Writing or reading from the device resulted in an error." << endl;
    else
        QTextStream(stdout) << "[deviceScanError]An unknown error has occurred." << endl;

    m_deviceScanState = false;
}

//----------------------//
// --- BLE CONTROLLER --//
//----------------------//

void blethread::errorReceived(QLowEnergyController::Error /*error*/)
{
    QTextStream(stdout) << "Error: " << controller->errorString();
}

//----------------------//
// ---  BLE SERVICE  ---//
//----------------------//
void blethread::scanServices(const QString &address)
{
    for (int i = 0; i < devices.size(); i++) {
        if (reinterpret_cast<DeviceInfo*>(devices.at(i))->getAddress() == address)
            currentDevice.setDevice(reinterpret_cast<DeviceInfo*>(devices.at(i))->getDevice());
    }

    if (!currentDevice.getDevice().isValid()) {
         qDebug() << "[Warning] Not a valid device";
        return;
    }

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit characteristicsUpdated();
    qDeleteAll(m_services);
    m_services.clear();
    emit servicesUpdated();

    qDebug() << "Connecting to device...";

    // Connecting signals and slots for connecting to LE services.
    controller = new QLowEnergyController(currentDevice.getDevice());
    connect(controller, &QLowEnergyController::connected,
            this, &blethread::deviceConnected);
    connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
            this, &blethread::errorReceived);
    connect(controller, &QLowEnergyController::disconnected,
            this, &blethread::deviceDisconnected);
    connect(controller, &QLowEnergyController::serviceDiscovered,
            this, &blethread::addLowEnergyService);
    connect(controller, &QLowEnergyController::discoveryFinished,
            this, &blethread::serviceScanDone);

    controller->connectToDevice();
}

void blethread::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    qDebug()<< "[addLowEnergyService]::" << serviceUuid; //serv->getName() << endl;

    if(serviceUuid == QBluetoothUuid(ear_uuid)) {
        //create service object
        m_eARservice = controller->createServiceObject(QBluetoothUuid(ear_uuid),this);
        m_eARservice->discoverDetails();
        if(m_eARservice) {
            qDebug() << "Service object is created";
            connect(m_eARservice, &QLowEnergyService::stateChanged, this, &blethread::serviceStateChanged);
            connect(m_eARservice, &QLowEnergyService::characteristicChanged, this,&blethread::updateIMUvalue);

            m_eARservice->discoverDetails();

            start();
       }
    }
    emit servicesUpdated();
}

void blethread::serviceScanDone()
{
    qDebug() << "[serviceScanDone]Service scan done!";
    if (m_services.isEmpty())
        emit servicesUpdated();
}

void blethread::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
        case QLowEnergyService::DiscoveringServices:
            qDebug() << "[serviceStateChanged]xDiscovering services...";
            break;
        case QLowEnergyService::ServiceDiscovered:
        {
            qDebug() << "[serviceStateChanged]xService discovered.";
            foreach(QLowEnergyCharacteristic c, m_eARservice->characteristics()) {
                        qDebug() << "[serviceStateChanged]Characteristic:" << c.uuid() << c.name();
                    }
            qDebug() << "[serviceStateChanged]Getting the IMU characteristic" << endl;
                    m_IMUchar = m_eARservice->characteristic(QBluetoothUuid(IMU_uuid));
                    if (!m_IMUchar.isValid()) {
                        qDebug() << "[serviceStateChanged]IMU characteristic is not found.";
                        break;
                    }
            qDebug() << "[serviceStateChanged]Registering for notifications on the IMU service";
                    m_notificationDesc = m_IMUchar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                    if (m_notificationDesc.isValid())
                        m_eARservice->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
                    break;
        }
        default:
            //nothing for now
            break;
        }
}

//----------------------//
//   ---  QUIT BLE  --- //
//----------------------//
void blethread::disconnectFromDevice()
{
    // UI always expects disconnect() signal when calling this signal
    // TODO what is really needed is to extend state() to a multi value
    // and thus allowing UI to keep track of controller progress in addition to
    // device scan progress

    if (controller->state() != QLowEnergyController::UnconnectedState)
        controller->disconnectFromDevice();
    else{
        deviceDisconnected();
    }
}

void blethread::deviceDisconnected()
{
    emit resetGraph();
}

//----------------------//
// -- Update BLE Data --//
//----------------------//
void blethread::updateIMUvalue(const QLowEnergyCharacteristic &ch, const QByteArray &value)
{
    if (ch.uuid() == QBluetoothUuid(IMU_uuid)) {
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

        QByteArray ax = value.mid(2,2);
        QByteArray ay = value.mid(4,2);
        QByteArray az = value.mid(6,2);
        QByteArray gx = value.mid(8,2);
        QByteArray gy = value.mid(10,2);
        QByteArray gz = value.mid(12,2);
        QByteArray mx = value.mid(14,2);
        QByteArray my = value.mid(16,2);
        QByteArray mz = value.right(2);

        bool bStatus = false;
        // ACC, GYR, MAG of IMU
        int ACC_X =  QString(ax.toHex()).toInt(&bStatus,16);
        //if (ACC_X > 32768) ACC_X = ACC_X - 65536;
        int ACC_Y =  QString(ay.toHex()).toInt(&bStatus,16);
        //if (ACC_Y > 32768) ACC_Y = ACC_Y - 65536;
        int ACC_Z =  QString(az.toHex()).toInt(&bStatus,16);
        //if (ACC_Z > 32768) ACC_Z = ACC_Z - 65536;
        int GYR_X =  QString(gx.toHex()).toInt(&bStatus,16);
        //if (GYR_X > 32768) GYR_X = GYR_X - 65536;
        int GYR_Y =  QString(gy.toHex()).toInt(&bStatus,16);
        //if (GYR_Y > 32768) GYR_Y = GYR_Y - 65536;
        int GYR_Z =  QString(gz.toHex()).toInt(&bStatus,16);
        //if (GYR_Z > 32768) GYR_Z = GYR_Z - 65536;
        int MAG_X =  QString(mx.toHex()).toInt(&bStatus,16);
        //if (MAG_X > 32768) MAG_X = std::abs(MAG_X - 65536);
        int MAG_Y =  QString(my.toHex()).toInt(&bStatus,16);
        //if (MAG_Y > 32768) MAG_Y = std::abs(MAG_Y - 65536);
        int MAG_Z =  QString(mz.toHex()).toInt(&bStatus,16);
        //if (MAG_Z > 32768) MAG_Z = std::abs(MAG_Z - 65536);

        QString aa = QString("HI%1").arg(1);

        QString DataStr = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10")
                .number(currTime).number(ACC_X).number(ACC_Y).number(ACC_Z)
                .number(GYR_X).number(GYR_Y).number(GYR_Z)
                .number(MAG_X).number(MAG_Y).number(MAG_Z);

        qDebug() << "Received DATA --- " << DataStr;


        BLEReceiveData[0]=currTime;
        BLEReceiveData[1]=ACC_X;
        BLEReceiveData[2]=ACC_Y;
        BLEReceiveData[3]=ACC_Z;
        BLEReceiveData[4]=GYR_X;
        BLEReceiveData[5]=GYR_Y;
        BLEReceiveData[6]=GYR_Z;
        BLEReceiveData[7]=MAG_X;
        BLEReceiveData[8]=MAG_Y;
        BLEReceiveData[9]=MAG_Z;

        if (mSaveFlag){
            BLEReceiveDataSet.push_back(BLEReceiveData[0]);
            BLEReceiveDataSet.push_back(BLEReceiveData[1]);
            BLEReceiveDataSet.push_back(BLEReceiveData[2]);
            BLEReceiveDataSet.push_back(BLEReceiveData[3]);
            BLEReceiveDataSet.push_back(BLEReceiveData[4]);
            BLEReceiveDataSet.push_back(BLEReceiveData[5]);
            BLEReceiveDataSet.push_back(BLEReceiveData[6]);
            BLEReceiveDataSet.push_back(BLEReceiveData[7]);
            BLEReceiveDataSet.push_back(BLEReceiveData[8]);
            BLEReceiveDataSet.push_back(BLEReceiveData[9]);
        }
        emit updateGraph();
    }
}

void blethread::receiveSaveFlag(bool save_ble_flag){
    mutex.lock();
    mSaveFlag = save_ble_flag;
    mutex.unlock();
}

void blethread::receiveFileName(QString Subject, QString Action, QString Index){
    mSubjectName = Subject;
    mActionName = Action;
    mIndexName = Index;
}
