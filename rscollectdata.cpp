#include "rscollectdata.h"
#include "ui_rscollectdata.h"

using namespace cv;
using namespace rs2;

rsCollectData::rsCollectData(QWidget *parent) :
    QWidget(parent),
    localDevice(new QBluetoothLocalDevice),
    discoveryAgent(new QBluetoothDeviceDiscoveryAgent),
    ui(new Ui::rsCollectData)
{
    ui->setupUi(this);

    qRegisterMetaType< cv::Mat >("cv::Mat");
    qRegisterMetaType< cv::Mat >("cv::Mat&");

    ui->Text_subject_name->setText("Hamlyn");
    ui->Text_subject_action->setText("NormalWalk");
    ui->Button_saveRGBD->setEnabled(false);
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_startSaveBLE->setEnabled(true);
    ui->Button_stopSaveBLE->setEnabled(false);

    connect(ui->Button_ScanBLE, SIGNAL(clicked()), this, SLOT(startDeviceDiscovery()));
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(ui->List_BLE, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(itemActivated(QListWidgetItem*)));
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &rsCollectData::deviceScanFinished);
    connect(ui->Button_QuitBLE,SIGNAL(clicked()), this, SLOT(disconnectFromDevice()));
    connect(this,SIGNAL(DataReceived()),this,SLOT(show_BLE_graph()));


    init_BLE_graph();
}

rsCollectData::~rsCollectData()
{
    rsCapture->stop();
    //rsFiltered->stop();
    rsSave->stop();
    delete discoveryAgent;
    delete controller;
    qDeleteAll(devices);
    qDeleteAll(m_services);
    qDeleteAll(m_characteristics);
    devices.clear();
    m_services.clear();
    m_characteristics.clear();
    delete ui;
}

void rsCollectData::on_Button_openRSThread_clicked()
{
    ui->Button_saveRGBD->setEnabled(true);
    ui->Button_closeRSThread->setEnabled(true);
    ui->Button_openRSThread->setEnabled(false);
    rsCapture = new rsCaptureThread();
    rsCapture->startCollect();
    //rsFiltered = new rsFilteredThread();
    //rsFiltered->startFilter();

    //QObject::connect(rsFiltered,SIGNAL(sendColorFiltered(cv::Mat)),this,SLOT(show_color_mat(cv::Mat)));
    //QObject::connect(rsFiltered,SIGNAL(sendDepthFiltered(cv::Mat)),this,SLOT(show_depth_mat(cv::Mat)));

    //connect(rsCapture,SIGNAL(sendColorMat(cv::Mat&)),this,SLOT(show_color_mat(cv::Mat&)));
    //connect(rsCapture,SIGNAL(sendDepthMat(cv::Mat&)),this,SLOT(show_depth_mat(cv::Mat&)));
    connect(rsCapture, SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&)), this, SLOT(show_RGBD_mat(cv::Mat&,cv::Mat&)));
}

void rsCollectData::on_Button_closeRSThread_clicked()
{
    ui->Button_closeRSThread->setEnabled(false);
    ui->Button_openRSThread->setEnabled(true);
    rsCapture->stop();
    //rsFiltered->stop();
    if (save_flag){
        rsSave->stop();
    }

    //disconnect(rsCapture,SIGNAL(sendColorMat(cv::Mat&)),this,SLOT(show_color_mat(cv::Mat)));
    //disconnect(rsCapture,SIGNAL(sendDepthMat(cv::Mat&)),this,SLOT(show_depth_mat(cv::Mat)));
    disconnect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&)), this,SLOT(show_RGBD_mat(cv::Mat&,cv::Mat&)));
}

void rsCollectData::on_Button_saveRGBD_clicked()
{
    ui->Button_stopSaveRGBD->setEnabled(true);
    ui->Button_saveRGBD->setEnabled(false);
    rsSave = new rssavethread();
    save_flag = true;

    // Emit subject & action names for saving
    connect(this,SIGNAL(send_RGBD_name(QString, QString)), rsSave, SLOT(receive_RGBD_name(QString,QString)));
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    emit send_RGBD_name(Subject, Action);
    qDebug() << "Subject:::" << Subject;
    qDebug() << "Action:::" << Action;

    // Emit rgb-d images for saving
    connect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&)), rsSave,SLOT(save_RGBD_mat(cv::Mat&,cv::Mat&)));
}

void rsCollectData::on_Button_stopSaveRGBD_clicked()
{
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_saveRGBD->setEnabled(true);
    rsSave->stop();
    save_flag = false;
    // --- RS_CAPTURE THREAD
    disconnect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&)), rsSave,SLOT(save_RGBD_mat(cv::Mat&,cv::Mat&)));
}

//=============================
// Show image slots
//=============================
void rsCollectData::show_color_mat(Mat &color_mat){
    cv::Mat QTmat = color_mat.clone();
    cvtColor(QTmat,QTmat,CV_BGR2RGB);
    QImage qcolor = QImage((QTmat.data), QTmat.cols, QTmat.rows, QImage::Format_RGB888);
    QImage qcolorshow = qcolor.scaled(QTmat.cols, QTmat.rows).scaled(480, 360, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->RGBImg->setPixmap(QPixmap::fromImage(qcolorshow));
    ui->RGBImg->resize(qcolorshow.size());

    // Calculate FPS
    QDateTime currTime = QDateTime::currentDateTime();
    uint CurrTimeT = currTime.toTime_t();
    if (CurrTimeT == LastTimeT){
        fps++;
    }
    else{
        if (fps>30){
            fps = 30;
        }
        QTextStream(stdout) << fps << endl;
        fps=1;
    }
    LastTimeT = CurrTimeT;
    frame_cnt++;
}

void rsCollectData::show_depth_mat(Mat &depth_mat){
    QImage qdepth = QImage( (depth_mat.data), depth_mat.cols, depth_mat.rows, QImage::Format_RGB16 );
    QImage qdepthshow = qdepth.scaled(depth_mat.cols, depth_mat.rows).scaled(480, 360, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->DepthImg->setPixmap(QPixmap::fromImage(qdepthshow));
    ui->DepthImg->resize(qdepthshow.size());
}

void rsCollectData::show_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat){
    cv::Mat QTmat = color_mat.clone();
    cvtColor(QTmat,QTmat,CV_BGR2RGB);
    QImage qcolor = QImage((QTmat.data), QTmat.cols, QTmat.rows, QImage::Format_RGB888);
    QImage qcolorshow = qcolor.scaled(QTmat.cols, QTmat.rows).scaled(480, 360, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->RGBImg->setPixmap(QPixmap::fromImage(qcolorshow));
    ui->RGBImg->resize(qcolorshow.size());

    cv::Mat QDepth = depth_mat.clone();
    QImage qdepth = QImage( (QDepth.data), QDepth.cols, QDepth.rows, QImage::Format_RGB16 );
    QImage qdepthshow = qdepth.scaled(QDepth.cols, QDepth.rows).scaled(480, 360, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->DepthImg->setPixmap(QPixmap::fromImage(qdepthshow));
    ui->DepthImg->resize(qdepthshow.size());

    cv::waitKey(2);
}

//=============================
// Show and Save BLE slots
//=============================

void rsCollectData::on_Button_startSaveBLE_clicked()
{
    ui->Button_stopSaveBLE->setEnabled(true);
    ui->Button_startSaveBLE->setEnabled(false);
    save_BLE_flag = true;
}

void rsCollectData::on_Button_stopSaveBLE_clicked()
{
    ui->Button_stopSaveBLE->setEnabled(false);
    ui->Button_startSaveBLE->setEnabled(true);
    save_BLE_flag = false;
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QString filename = QString("/home/skaegy/Data/EAR/%1_%2_%3.csv")
            .arg(currTime)
            .arg(ui->Text_subject_name->toPlainText())
            .arg(ui->Text_subject_action->toPlainText());

    QFile saveBLEfile(filename);
    if(saveBLEfile.open(QFile::WriteOnly |QFile::Truncate))
    {
        qDebug() << "Start writing data to ... " << filename;
        QTextStream stream(&saveBLEfile);
        stream << "Timestamp" << "\t" << "ACC_X" << "\t" << "ACC_Y" << "\t" << "ACC_Z" << "\t"
                     << "GYR_X" << "\t" << "GYR_Y" << "\t" << "GYR_Z" << "\t"
                     << "MAG_X" << "\t" << "MAG_Y" << "\t" << "MAG_Z" << "\n";
        for (QList<qint64>::iterator it = BLEReceiveDataSet.begin(); it!=BLEReceiveDataSet.end(); it++){
            save_BLE_cnt++;

            if (save_BLE_cnt != 10){
                stream << *it << "\t";
            }
            else{
                stream << *it << "\n";
                save_BLE_cnt = 0;
            }
        }

        saveBLEfile.close();
        qDebug() << "BLE-eAR sensor data is saved";
    }
    BLEReceiveDataSet.clear();
}

void rsCollectData::init_BLE_graph(){
    // =========== ACC PLOT =========== //
    ui->customPlot_ACC->addGraph(); // blue line
    ui->customPlot_ACC->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->customPlot_ACC->graph(0)->setName(QString("ACC_X"));
    ui->customPlot_ACC->addGraph(); // red line
    ui->customPlot_ACC->graph(1)->setPen(QPen(QColor(255, 110, 40)));
    ui->customPlot_ACC->graph(1)->setName(QString("ACC_Y"));
    ui->customPlot_ACC->addGraph(); // green line
    ui->customPlot_ACC->graph(2)->setPen(QPen(QColor(110, 255, 40)));
    ui->customPlot_ACC->graph(2)->setName(QString("ACC_Z"));

    ui->customPlot_ACC->axisRect()->setupFullAxesBox();
    ui->customPlot_ACC->yAxis->setRange(-32678, 32678);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->customPlot_ACC->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_ACC->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot_ACC->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_ACC->yAxis2, SLOT(setRange(QCPRange)));

    ui->customPlot_ACC->legend->setVisible(true);
    ui->customPlot_ACC->legend->setFont(QFont("Helvetica", 7));
    ui->customPlot_ACC->legend->setRowSpacing(-3);
    ui->customPlot_ACC->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignBottom);

    // =========== GYR PLOT =========== //
    ui->customPlot_GYR->addGraph(); // blue line
    ui->customPlot_GYR->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->customPlot_GYR->graph(0)->setName(QString("GYR_X"));
    ui->customPlot_GYR->addGraph(); // red line
    ui->customPlot_GYR->graph(1)->setPen(QPen(QColor(255, 110, 40)));
    ui->customPlot_GYR->graph(1)->setName(QString("GYR_Y"));
    ui->customPlot_GYR->addGraph(); // green line
    ui->customPlot_GYR->graph(2)->setPen(QPen(QColor(110, 255, 40)));
    ui->customPlot_GYR->graph(2)->setName(QString("GYR_Z"));

    ui->customPlot_GYR->axisRect()->setupFullAxesBox();
    ui->customPlot_GYR->yAxis->setRange(-32678, 32678);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->customPlot_GYR->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_GYR->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot_GYR->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_GYR->yAxis2, SLOT(setRange(QCPRange)));

    ui->customPlot_GYR->legend->setVisible(true);
    ui->customPlot_GYR->legend->setFont(QFont("Helvetica", 7));
    ui->customPlot_GYR->legend->setRowSpacing(-3);
    ui->customPlot_GYR->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignBottom);

    // =========== MAG PLOT =========== //
    ui->customPlot_MAG->addGraph(); // blue line
    ui->customPlot_MAG->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->customPlot_MAG->graph(0)->setName(QString("MAG_X"));
    ui->customPlot_MAG->addGraph(); // red line
    ui->customPlot_MAG->graph(1)->setPen(QPen(QColor(255, 110, 40)));
    ui->customPlot_MAG->graph(1)->setName(QString("MAG_Y"));
    ui->customPlot_MAG->addGraph(); // green line
    ui->customPlot_MAG->graph(2)->setPen(QPen(QColor(110, 255, 40)));
    ui->customPlot_MAG->graph(2)->setName(QString("MAG_Z"));

    ui->customPlot_MAG->axisRect()->setupFullAxesBox();
    ui->customPlot_MAG->yAxis->setRange(-32678, 32678);

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->customPlot_MAG->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_MAG->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot_MAG->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot_MAG->yAxis2, SLOT(setRange(QCPRange)));

    ui->customPlot_MAG->legend->setVisible(true);
    ui->customPlot_MAG->legend->setFont(QFont("Helvetica", 7));
    ui->customPlot_MAG->legend->setRowSpacing(-3);
    ui->customPlot_MAG->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignBottom);
}

void rsCollectData::show_BLE_graph(){
    if (BLEData_flag){
        // ========= PLOT ACC ========= //
        ui->customPlot_ACC->graph(0)->addData(key_ACC, BLEReceiveData[1]);
        ui->customPlot_ACC->graph(1)->addData(key_ACC, BLEReceiveData[2]);
        ui->customPlot_ACC->graph(2)->addData(key_ACC, BLEReceiveData[3]);
        ui->customPlot_ACC->xAxis->setRange(key_ACC, 100, Qt::AlignRight);
        ++key_ACC;
        ui->customPlot_ACC->replot();
        // ========= PLOT GYR ========= //
        ui->customPlot_GYR->graph(0)->addData(key_GYR, BLEReceiveData[4]);
        ui->customPlot_GYR->graph(1)->addData(key_GYR, BLEReceiveData[5]);
        ui->customPlot_GYR->graph(2)->addData(key_GYR, BLEReceiveData[6]);
        ui->customPlot_GYR->xAxis->setRange(key_GYR, 100, Qt::AlignRight);
        ++key_GYR;
        ui->customPlot_GYR->replot();
        // ========= PLOT MAG ========= //
        ui->customPlot_MAG->graph(0)->addData(key_MAG, BLEReceiveData[7]);
        ui->customPlot_MAG->graph(1)->addData(key_MAG, BLEReceiveData[8]);
        ui->customPlot_MAG->graph(2)->addData(key_MAG, BLEReceiveData[9]);
        ui->customPlot_MAG->xAxis->setRange(key_MAG, 100, Qt::AlignRight);
        ++key_MAG;
        ui->customPlot_MAG->replot();
    }
}

//=============================
// BLE process slot
//=============================

void rsCollectData::startDeviceDiscovery()
{
    qDeleteAll(devices);
    devices.clear();
    emit devicesUpdated();

    QTextStream(stdout) << "[startDeviceDiscovery]Scanning for devices ..." << endl;
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    ui->Button_ScanBLE->setEnabled(false);

    if (discoveryAgent->isActive()) {
        m_deviceScanState = true;
        Q_EMIT stateChanged();
    }
}

void rsCollectData::addDevice(const QBluetoothDeviceInfo &info)
{
    QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());
    QList<QListWidgetItem *> items = ui->List_BLE->findItems(label, Qt::MatchExactly);
    DeviceInfo *d = new DeviceInfo(info);
    devices.append(d);
    if (items.empty()) {
        QListWidgetItem *item = new QListWidgetItem(label);
        QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(info.address());
        if (pairingStatus == QBluetoothLocalDevice::Paired || pairingStatus == QBluetoothLocalDevice::AuthorizedPaired )
            item->setTextColor(QColor(Qt::green));
        else
            item->setTextColor(QColor(Qt::black));

        ui->List_BLE->addItem(item);
    }
}

void rsCollectData::itemActivated(QListWidgetItem *item){
    QString text = item->text();

    int index = text.indexOf(' ');
    if (index == -1)
        return;
    qDebug() << "Connect to: " << text;
    scanServices(text.left((index)));
}

void rsCollectData::deviceScanFinished()
{
    emit devicesUpdated();
    m_deviceScanState = false;
    emit stateChanged();
    if (devices.isEmpty())
        qDebug() << "[scanFinished]No Low Energy devices found...";
    else
        qDebug() << "[scanFinished]Done! Scan Again!";
    ui->Button_ScanBLE->setEnabled(true);
}

void rsCollectData::scanServices(const QString &address)
{
    for (int i = 0; i < devices.size(); i++) {
        if (((DeviceInfo*)devices.at(i))->getAddress() == address )
            currentDevice.setDevice(((DeviceInfo*)devices.at(i))->getDevice());
    }

    if (!currentDevice.getDevice().isValid()) {
         qDebug() << "[Warning]Not a valid device";
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
            this, &rsCollectData::deviceConnected);
    connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
            this, &rsCollectData::errorReceived);
    connect(controller, &QLowEnergyController::disconnected,
            this, &rsCollectData::deviceDisconnected);
    connect(controller, &QLowEnergyController::serviceDiscovered,
            this, &rsCollectData::addLowEnergyService);
    connect(controller, &QLowEnergyController::discoveryFinished,
            this, &rsCollectData::serviceScanDone);

    controller->connectToDevice();
}

void rsCollectData::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    qDebug()<< "[addLowEnergyService]::" << serviceUuid; //serv->getName() << endl;

    if(serviceUuid == QBluetoothUuid(ear_uuid)) {
        //create service object
        m_eARservice = controller->createServiceObject(QBluetoothUuid(ear_uuid),this);
        m_eARservice->discoverDetails();
        if(m_eARservice) {
            qDebug() << "Service object is created";
            connect(m_eARservice, &QLowEnergyService::stateChanged, this, &rsCollectData::serviceStateChanged);
            connect(m_eARservice, &QLowEnergyService::characteristicChanged, this,&rsCollectData::updateIMUvalue);
            // Set parameters for further processing ......
            BLEData_flag = true;
            ui->Button_startSaveBLE->setEnabled(true);

            m_eARservice->discoverDetails();
       }
    }
    emit servicesUpdated();
}

void rsCollectData::serviceScanDone()
{
    qDebug() << "[serviceScanDone]Service scan done!";
    if (m_services.isEmpty())
        emit servicesUpdated();
}

void rsCollectData::serviceStateChanged(QLowEnergyService::ServiceState s)
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

void rsCollectData::updateIMUvalue(const QLowEnergyCharacteristic &ch, const QByteArray &value)
{
    if (ch.uuid() == QBluetoothUuid(IMU_uuid)) {
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
        if (ACC_X > 32768) ACC_X = ACC_X - 65536;
        int ACC_Y =  QString(ay.toHex()).toInt(&bStatus,16);
        if (ACC_Y > 32768) ACC_Y = ACC_Y - 65536;
        int ACC_Z =  QString(az.toHex()).toInt(&bStatus,16);
        if (ACC_Z > 32768) ACC_Z = ACC_Z - 65536;
        int GYR_X =  QString(gx.toHex()).toInt(&bStatus,16);
        if (GYR_X > 32768) GYR_X = GYR_X - 65536;
        int GYR_Y =  QString(gy.toHex()).toInt(&bStatus,16);
        if (GYR_Y > 32768) GYR_Y = GYR_Y - 65536;
        int GYR_Z =  QString(gz.toHex()).toInt(&bStatus,16);
        if (GYR_Z > 32768) GYR_Z = GYR_Z - 65536;
        int MAG_X =  QString(mx.toHex()).toInt(&bStatus,16);
        if (MAG_X > 32768) MAG_X = std::abs(MAG_X - 65536);
        int MAG_Y =  QString(my.toHex()).toInt(&bStatus,16);
        if (MAG_Y > 32768) MAG_Y = std::abs(MAG_Y - 65536);
        int MAG_Z =  QString(mz.toHex()).toInt(&bStatus,16);
        if (MAG_Z > 32768) MAG_Z = std::abs(MAG_Z - 65536);

        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
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

        if (save_BLE_flag){
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
        emit DataReceived();
    }
}

void rsCollectData::deviceConnected()
{
    connected = true;
    controller->discoverServices();
}

void rsCollectData::errorReceived(QLowEnergyController::Error /*error*/)
{
    QTextStream(stdout) << "Error: " << controller->errorString();
}

void rsCollectData::disconnectFromDevice()
{
    // UI always expects disconnect() signal when calling this signal
    // TODO what is really needed is to extend state() to a multi value
    // and thus allowing UI to keep track of controller progress in addition to
    // device scan progress

    if (controller->state() != QLowEnergyController::UnconnectedState)
        controller->disconnectFromDevice();
    else
        deviceDisconnected();
}

void rsCollectData::deviceDisconnected()
{
    QTextStream(stdout) << "[Warning, deviceDisconnected] Disconnect from device" << endl;
    BLEData_flag = false;
    key_ACC = 1; key_GYR = 1; key_MAG = 1;

    ui->customPlot_ACC->graph(0)->data()->clear();
    ui->customPlot_ACC->graph(1)->data()->clear();
    ui->customPlot_ACC->graph(2)->data()->clear();
    ui->customPlot_ACC->replot();
    ui->customPlot_GYR->graph(0)->data()->clear();
    ui->customPlot_GYR->graph(1)->data()->clear();
    ui->customPlot_GYR->graph(2)->data()->clear();
    ui->customPlot_GYR->replot();
    ui->customPlot_MAG->graph(0)->data()->clear();
    ui->customPlot_MAG->graph(1)->data()->clear();
    ui->customPlot_MAG->graph(2)->data()->clear();
    ui->customPlot_MAG->replot();
    emit disconnected();
}

void rsCollectData::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::ServiceDiscovered) {
        if (newState != QLowEnergyService::DiscoveringServices) {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }
        return;
    }

    QLowEnergyService *service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
        return;

    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    foreach (const QLowEnergyCharacteristic &ch, chars) {
        CharacteristicInfo *cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }

    emit characteristicsUpdated();
}

void rsCollectData::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        QTextStream(stdout) << "[deviceScanError]The Bluetooth adaptor is powered off, power it on before doing discovery." << endl;
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        QTextStream(stdout) << "[deviceScanError]Writing or reading from the device resulted in an error." << endl;
    else
        QTextStream(stdout) << "[deviceScanError]An unknown error has occurred." << endl;

    m_deviceScanState = false;
    emit devicesUpdated();
    emit stateChanged();
}

