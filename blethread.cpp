#include "blethread.h"

blethread::blethread(QObject* parent)
    : QThread(parent),
      discoveryAgent(new QBluetoothDeviceDiscoveryAgent),
      localDevice(new QBluetoothLocalDevice)
{
    mutex.lock();
    abort = false;
    devices.clear();
    m_characteristics.clear();
    m_services.clear();
    mSaveFlag = false;
    mutex.unlock();

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &blethread::deviceScanFinished);
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),this, SLOT(addDevice(QBluetoothDeviceInfo)));
}

blethread::~blethread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    delete discoveryAgent;
    delete controller;
    qDeleteAll(devices);
    qDeleteAll(m_services);
    qDeleteAll(m_characteristics);
    wait();
}

void blethread::run(){
    while(!abort){
        if (mSaveFlag){
            //save...
        }
    }
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

void blethread::addDevice(const QBluetoothDeviceInfo &info)
{
    if (info.name() == "e-AR2016"){
        QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());

        DeviceInfo *d = new DeviceInfo(info);
        devices.append(d);

        QListWidgetItem *item = new QListWidgetItem(label);
        QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(info.address());
        if (pairingStatus == QBluetoothLocalDevice::Paired || pairingStatus == QBluetoothLocalDevice::AuthorizedPaired )
            item->setTextColor(QColor(Qt::green));
        else
            item->setTextColor(QColor(Qt::black));

        emit sendItem(item);
    }
        //ui->List_BLE->addItem(item);

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
    connect(this, &blethread::startSaveCSV, this, &blethread::saveBLEData);

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
            //start();
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
    qDebug() << "[Disconnect...]";
    if (controller->state() != QLowEnergyController::UnconnectedState)
        controller->disconnectFromDevice();
    else{
        deviceDisconnected();
    }
}

void blethread::deviceDisconnected()
{
    emit resetGraph();
    disconnect(this, &blethread::startSaveCSV, this, &blethread::saveBLEData);
    mBLEstorage.clear();
}

//----------------------//
// -- Update BLE Data --//
//----------------------//
void blethread::updateIMUvalue(const QLowEnergyCharacteristic &ch, const QByteArray &value)
{
    if (ch.uuid() == QBluetoothUuid(IMU_uuid)) {
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

        /*
        VALUE = 19 (0-18) BYTES
        BYTE(0) = 0x00 - Start idx
        BYTE(1, 3, 5, 7, 9, 11) = 0xFF
        AX = Int16(BYTE(2)BYTE(1))
        AY = Int16(BYTE(4)BYTE(3))
        etc.
        */

        QString ax = QString(value.mid(2,1).toHex())+QString(value.mid(1,1).toHex());
        QString ay = QString(value.mid(4,1).toHex())+QString(value.mid(3,1).toHex());
        QString az = QString(value.mid(6,1).toHex())+QString(value.mid(5,1).toHex());
        QString gx = QString(value.mid(8,1).toHex())+QString(value.mid(7,1).toHex());
        QString gy = QString(value.mid(10,1).toHex())+QString(value.mid(9,1).toHex());
        QString gz = QString(value.mid(12,1).toHex())+QString(value.mid(11,1).toHex());
        QString mx = QString(value.mid(14,1).toHex())+QString(value.mid(13,1).toHex());
        QString my = QString(value.mid(16,1).toHex())+QString(value.mid(15,1).toHex());
        QString mz = QString(value.mid(18,1).toHex())+QString(value.mid(17,1).toHex());

        bool bStatus = false;

        // ACC, GYR, MAG of IMU
        int ACC_X =  ax.toInt(&bStatus,16);
        if (ACC_X > 32768) ACC_X = ACC_X - 65536;
        int ACC_Y =  ay.toInt(&bStatus,16);
        if (ACC_Y > 32768) ACC_Y = ACC_Y - 65536;
        int ACC_Z =  az.toInt(&bStatus,16);
        if (ACC_Z > 32768) ACC_Z = ACC_Z - 65536;
        int GYR_X =  gx.toInt(&bStatus,16);
        if (GYR_X > 32768) GYR_X = GYR_X - 65536;
        int GYR_Y =  gy.toInt(&bStatus,16);
        if (GYR_Y > 32768) GYR_Y = GYR_Y - 65536;
        int GYR_Z =  gz.toInt(&bStatus,16);
        if (GYR_Z > 32768) GYR_Z = GYR_Z - 65536;
        int MAG_X =  mx.toInt(&bStatus,16);
        if (MAG_X > 32768) MAG_X = std::abs(MAG_X - 65536);
        int MAG_Y =  my.toInt(&bStatus,16);
        if (MAG_Y > 32768) MAG_Y = std::abs(MAG_Y - 65536);
        int MAG_Z =  mz.toInt(&bStatus,16);
        if (MAG_Z > 32768) MAG_Z = std::abs(MAG_Z - 65536);

        QVector<qint64> BLEReceiveData(10);
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

        if (mLastTime != currTime){
            emit updateGraph(BLEReceiveData);
            if (mSaveFlag){
                mutex.lock();
                mBLEstorage.push_back(BLEReceiveData);
                mutex.unlock();
            }
        }
        mLastTime = currTime;
    }
}

void blethread::receiveSaveFlag(bool save_ble_flag){
    mutex.lock();
    mSaveFlag = save_ble_flag;
    mutex.unlock();
    if (mSaveFlag){
        mfilename = QString("%1/data/Yao/Capture/EAR/%2%3_EAR.csv").arg(QDir::homePath()).arg(mActionName).arg(mIndexName);
    }
    else{
        emit startSaveCSV(); // Save ble data while stop save is clicked
    }
}

void blethread::receiveFileName(QString Subject, QString Action, QString Index){
    mSubjectName = Subject;
    mActionName = Action;
    mIndexName = Index;
}

void blethread::saveBLEData(){
    QFile saveBLEfile(mfilename);
    QTextStream stream(&saveBLEfile);
    if(saveBLEfile.open(QFile::WriteOnly |QFile::Truncate))
    {
        for (QList<QVector<qint64>>::iterator it = mBLEstorage.begin(); it != mBLEstorage.end(); ++it){
            if (it == mBLEstorage.begin()){
                qDebug() << "Writing e-AR sensor data to ... " << mfilename;
                stream << "Timestamp" << "\t" << "ACC_X" << "\t" << "ACC_Y" << "\t" << "ACC_Z" << "\t"
                             << "GYR_X" << "\t" << "GYR_Y" << "\t" << "GYR_Z" << "\t"
                             << "MAG_X" << "\t" << "MAG_Y" << "\t" << "MAG_Z";
            }
            QVector<qint64> mBufData = *it;
            stream << "\n" << mBufData[0] << "\t" << mBufData[1] << "\t" << mBufData[2] << "\t" << mBufData[3] << "\t"
                         << mBufData[4] << "\t" << mBufData[5] << "\t" << mBufData[6] << "\t"
                         << mBufData[7] << "\t" << mBufData[8] << "\t" << mBufData[9];
        }
    }
    qDebug() << "[EAR] " << mBLEstorage.size() << " Frames has been saved";
    mBLEstorage.clear();
    saveBLEfile.close();
}
