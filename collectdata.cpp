#include "collectdata.h"
#include "ui_rscollectdata.h"

using namespace cv;
using namespace rs2;

rsCollectData::rsCollectData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::rsCollectData)
{
    ui->setupUi(this);
    // ======= Initialization ======= //

    // --- Register type
    qRegisterMetaType< cv::Mat >("cv::Mat");
    qRegisterMetaType< cv::Mat >("cv::Mat&");
    qRegisterMetaType< rs2::frame >("rs2::frame");

    // --- UI->TEXT
    ui->Text_subject_name->setText("Hamlyn");
    ui->Text_subject_action->setText("NormalWalk");
    ui->Text_subject_index->setText("1");
    ui->label_ConnectStatus->setText("<font color='red'>DISCONNECTED</font>");

    // --- UI->BUTTON
    ui->Button_saveRGBD->setEnabled(false);
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_startSaveBLE->setEnabled(false);
    ui->Button_stopSaveBLE->setEnabled(false);
    ui->Button_StopUDP->setEnabled(false);
    ui->Button_QuitBLE->setEnabled(false);
    ui->Button_ClearBLE->setEnabled(false);
    ui->Button_saveImage->setEnabled(false);

    // --- UI->SLIDER
    ui->spatial_alpha->setRange(25, 100);   ui->spatial_alpha->setSingleStep(1);
    ui->spatial_alpha->setValue(50);        ui->spatial_alpha_value->setNum(0.5);
    ui->spatial_delta->setRange(1,50);      ui->spatial_delta->setSingleStep(1);
    ui->spatial_delta->setValue(20);        ui->spatial_delta_value->setNum(20);
    ui->spatial_mag->setRange(1,5);         ui->spatial_mag->setSingleStep(1);
    ui->spatial_mag->setValue(2);           ui->spatial_mag_value->setNum(2);

    ui->temporal_alpha->setRange(0,100);    ui->temporal_alpha->setSingleStep(1);
    ui->temporal_alpha->setValue(50);       ui->temporal_alpha_value->setNum(0.5);
    ui->temporal_delta->setRange(1,100);    ui->temporal_delta->setSingleStep(1);
    ui->temporal_delta->setValue(50);       ui->temporal_delta_value->setNum(50);
    ui->disparity->setRange(0,512);         ui->disparity->setSingleStep(2);
    ui->disparity->setValue(0);             ui->disparity_value->setNum(0);
    ui->MAX->setRange(500,10000);           ui->MAX->setSingleStep(500);
    ui->MAX->setValue(5000);                ui->MAX_value->setNum(5);
    ui->Emitter_power->setRange(0,360);     ui->Emitter_power->setSingleStep(10);
    ui->Emitter_power->setValue(360);       ui->Emitter_power_value->setNum(360);

    // --- Connect signal and slot
    connect(ui->List_BLE, SIGNAL(itemActivated(QListWidgetItem*)),this, SLOT(itemActivated(QListWidgetItem*)));

    // Initialize BLE graph
    init_BLE_graph();
}

rsCollectData::~rsCollectData()
{
    delete rsCapture;
    delete rsFilter;
    delete rsSave;
    delete udpSync;
    EARSensor->disconnectFromDevice();
    EARSensor->wait();
    delete EARSensor;
    delete ui;
}

void rsCollectData::on_Button_openRSThread_clicked()
{
    mbStartRealsense = true;
    // --- UI Status
    ui->Button_saveRGBD->setEnabled(true);
    ui->Button_saveImage->setEnabled(true);
    ui->Button_closeRSThread->setEnabled(true);
    ui->Button_openRSThread->setEnabled(false);
    ui->lable_color_fps->setText("0");
    ui->lable_depth_fps->setText("0");

    // --- Create and Start thread
    rsCapture = new rsCaptureThread();
    rsCapture->startCollect();
    rsFilter = new rsFilterThread();
    rsFilter->startFilter();

    // --- Connect signals and slots
    connect(rsCapture, &rsCaptureThread::sendRGBDFrame, rsFilter, &rsFilterThread::receiveRGBDFrame);
    connect(rsFilter,  &rsFilterThread::sendRGBDFiltered, this, &rsCollectData::show_RGBD_mat);
    connect(this, &rsCollectData::send_Disparity, rsCapture, &rsCaptureThread::receiveDisparity);

    rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, float(ui->spatial_alpha->value()/100.0));
    rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, ui->spatial_delta->value());
    rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, ui->spatial_mag->value());
    rsFilter->temp_filter.set_option(RS2_OPTION_HOLES_FILL, ui->combo_persistency->currentIndex());
    rsFilter->temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, float(ui->temporal_alpha->value()/100.0));
    rsFilter->temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, ui->temporal_delta->value());

    // old version -- connect
    //connect(rsCapture, SIGNAL(sendRGBDFrame(rs2::frame,rs2::frame,qint64)),rsFilter,SLOT(receiveRGBDFrame(rs2::frame,rs2::frame,qint64)));
    //connect(rsFilter,SIGNAL(sendRGBDFiltered(cv::Mat&, cv::Mat&, qint64)), this, SLOT(show_RGBD_mat(cv::Mat&, cv::Mat&, qint64)));
    //connect(rsCapture, SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&,qint64)), this, SLOT(show_RGBD_mat(cv::Mat&,cv::Mat&,qint64)));
}

void rsCollectData::on_Button_closeRSThread_clicked()
{
    mbStartRealsense = false;

    ui->Button_closeRSThread->setEnabled(false);
    ui->Button_openRSThread->setEnabled(true);
    ui->Button_saveRGBD->setEnabled(false);
    ui->Button_saveImage->setEnabled(false);
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->lable_color_fps->setText("0");
    ui->lable_depth_fps->setText("0");

    disconnect(rsCapture, &rsCaptureThread::sendRGBDFrame, rsFilter, &rsFilterThread::receiveRGBDFrame);
    disconnect(rsFilter,  &rsFilterThread::sendRGBDFiltered, this, &rsCollectData::show_RGBD_mat);
    disconnect(this, &rsCollectData::send_Disparity, rsCapture, &rsCaptureThread::receiveDisparity);

    // old version -- disconnect
    //disconnect(rsCapture, SIGNAL(sendRGBDFrame(rs2::frame,rs2::frame,qint64)),rsFilter,SLOT(receiveRGBDFrame(rs2::frame,rs2::frame,qint64)));
    //disconnect(rsFilter,SIGNAL(sendRGBDFiltered(cv::Mat&, cv::Mat&, qint64)), this, SLOT(show_RGBD_mat(cv::Mat&, cv::Mat&, qint64)));
    //disconnect(rsFilter,SIGNAL(sendColorFiltered(cv::Mat&, qint64)), this, SLOT(show_color_mat(cv::Mat&,qint64)));
    //disconnect(rsFilter,SIGNAL(sendDepthFiltered(cv::Mat&, qint64)), this, SLOT(show_depth_mat(cv::Mat&,qint64)));
    //disconnect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&,qint64)), this,SLOT(show_RGBD_mat(cv::Mat&,cv::Mat&,qint64)));

    rsCapture->stop();
    rsCapture->quit();
    rsFilter->stop();
    rsFilter->quit();

    if (save_RGB_flag){
        disconnect(this, &rsCollectData::send_RGBD_name, rsSave, &rssavethread::receive_RGBD_name);
        disconnect(rsFilter, &rsFilterThread::sendRGBDFiltered, rsSave, &rssavethread::save_RGBD_mat);

        rsSave->stop();
        rsSave->quit();
    }


}

//=============================
// Save image button
//=============================
void rsCollectData::on_Button_saveRGBD_clicked()
{
    // --- UI Status
    ui->Button_stopSaveRGBD->setEnabled(true);
    ui->Button_saveRGBD->setEnabled(false);
    ui->Text_subject_name->setEnabled(false);
    ui->Text_subject_action->setEnabled(false);
    ui->Text_subject_index->setEnabled(false);

    // --- Create save thread
    rsSave = new rssavethread();
    rsSave->startSave();
    save_RGB_flag = true;

    // Emit subject & action names for saving
    connect(this, &rsCollectData::send_RGBD_name, rsSave, &rssavethread::receive_RGBD_name);
    //connect(this,SIGNAL(send_RGBD_name(QString, QString, QString)), rsSave, SLOT(receive_RGBD_name(QString,QString, QString)));
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    emit send_RGBD_name(Subject, Action, Index);

    // ---- Emit rgb-d images for saving ---- //
    // RS_FILTER & RSSAVE
    connect(rsFilter, &rsFilterThread::sendRGBDFiltered, rsSave, &rssavethread::save_RGBD_mat);
    //connect(rsFilter,SIGNAL(sendRGBDFiltered(cv::Mat&, cv::Mat&, qint64)), rsSave, SLOT(save_RGBD_mat(cv::Mat&, cv::Mat&, qint64)));
    // RS_CAPTURE THREAD & RS_SAVE THREAD
    //connect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&,qint64)), rsSave,SLOT(save_RGBD_mat(cv::Mat&,cv::Mat&,qint64)));
}

void rsCollectData::on_Button_stopSaveRGBD_clicked()
{
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_saveRGBD->setEnabled(true);
    ui->Text_subject_name->setEnabled(true);
    ui->Text_subject_action->setEnabled(true);
    ui->Text_subject_index->setEnabled(true);

    // ---- Emit rgb-d images for saving ---- //
    disconnect(this, &rsCollectData::send_RGBD_name, rsSave, &rssavethread::receive_RGBD_name);
    //disconnect(this,SIGNAL(send_RGBD_name(QString, QString, QString)), rsSave, SLOT(receive_RGBD_name(QString,QString, QString)));
    // RS_FILTER & RSSAVE
    disconnect(rsFilter, &rsFilterThread::sendRGBDFiltered, rsSave, &rssavethread::save_RGBD_mat);
    //disconnect(rsFilter,SIGNAL(sendRGBDFiltered(cv::Mat&, cv::Mat&, qint64)), rsSave, SLOT(save_RGBD_mat(cv::Mat&, cv::Mat&, qint64)));
    // RS_CAPTURE THREAD & RS_SAVE THREAD
    //disconnect(rsCapture,SIGNAL(sendRGBDMat(cv::Mat&,cv::Mat&,qint64)), rsSave,SLOT(save_RGBD_mat(cv::Mat&,cv::Mat&,qint64)));

    rsSave->stop();
    rsSave->quit();
    save_RGB_flag = false;
}

void rsCollectData::on_Button_saveImage_clicked()
{
    rsFilter->receiveSaveImageSignal();
}

void rsCollectData::on_Button_SAVEALL_clicked()
{
    if (ui->Button_saveRGBD->isEnabled() && ui->Button_startSaveBLE->isEnabled()){
        ui->Button_SAVESTOPALL->setEnabled(true);
        ui->Button_SAVEALL->setEnabled(false);
        ui->Button_saveRGBD->click();
        ui->Button_startSaveBLE->click();
    }
    else{
        qDebug() << "[WARNING]Invalid process...  Wait until realsense and EAR sensor are ready...";
    }
}

void rsCollectData::on_Button_SAVESTOPALL_clicked()
{
    if (ui->Button_stopSaveRGBD->isEnabled() && ui->Button_stopSaveBLE->isEnabled()){
        ui->Button_SAVESTOPALL->setEnabled(false);
        ui->Button_SAVEALL->setEnabled(true);
        ui->Button_stopSaveRGBD->click();
        ui->Button_stopSaveBLE->click();

        QString Index = ui->Text_subject_index->toPlainText();
        int idx = Index.toInt();
        ui->Text_subject_index->setText(QString::number(idx+1));

    }
    else{
        qDebug() << "[WARNING]Invalid process... Wait until realsense and EAR sensor are ready...";
    }
}

//=============================
// Show image slots
//=============================
void rsCollectData::show_color_mat(Mat &color_mat, qint64 timestamp){
    cv::Mat QTmat = color_mat.clone();
    cvtColor(QTmat,QTmat,CV_BGR2RGB);
    QImage qcolor = QImage((QTmat.data), IMG_WIDTH, IMG_HEIGHT, QImage::Format_RGB888);
    QImage qcolorshow = qcolor.scaled(IMG_WIDTH, IMG_HEIGHT).scaled(480, 280, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->RGBImg->setPixmap(QPixmap::fromImage(qcolorshow));
    ui->RGBImg->resize(qcolorshow.size());

    // Calculate FPS
    qint64 Interval = timestamp - mLastColorTimeT;
    mLastColorTimeT = timestamp;
    if ( Interval > 0 && Interval < 1000){
        mSumColorTime = mSumColorTime + static_cast<uint>(Interval);
        if (mSumColorTime > 1000){
            mColor_frame_cnt = mColor_frame_cnt > 30 ? 30 : mColor_frame_cnt;
            ui->lable_color_fps->setText(QString::number(mColor_frame_cnt));
            mColor_frame_cnt = 1;
            mSumColorTime = mSumColorTime - 1000;
        }
        else{
            mColor_frame_cnt++;
        }
    }
}

void rsCollectData::show_depth_mat(Mat &depth_mat, qint64 timestamp){
    QImage qdepth = QImage( (depth_mat.data), depth_mat.cols, depth_mat.rows, QImage::Format_RGB16);
    QImage qdepthshow = qdepth.scaled(depth_mat.cols, depth_mat.rows).scaled(480, 280, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->DepthImg->setPixmap(QPixmap::fromImage(qdepthshow));
    ui->DepthImg->resize(qdepthshow.size());

    // Calculate FPS
    qint64 Interval = timestamp - mLastDepthTimeT;
    mLastDepthTimeT = timestamp;
    if ( Interval > 0 && Interval < 1000){
        mSumDepthTime = mSumDepthTime + static_cast<uint>(Interval);
        if (mSumDepthTime > 1000){
            mDepth_frame_cnt = mDepth_frame_cnt > 30 ? 30 : mDepth_frame_cnt;
            ui->lable_depth_fps->setText(QString::number(mDepth_frame_cnt));
            mDepth_frame_cnt = 1;
            mSumDepthTime = mSumDepthTime - 1000;
        }
        else{
            mDepth_frame_cnt++;
        }
    }
}

void rsCollectData::show_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp){
    // Show color images
    cv::Mat QTmat = color_mat.clone();
    cvtColor(QTmat,QTmat,CV_BGR2RGB);
    QImage qcolor = QImage((QTmat.data), QTmat.cols, QTmat.rows, QImage::Format_RGB888);
    QImage qcolorshow = qcolor.scaled(QTmat.cols, QTmat.rows).scaled(480, 280, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->RGBImg->setPixmap(QPixmap::fromImage(qcolorshow));
    ui->RGBImg->resize(qcolorshow.size());

    // Show depth images
    cv::Mat QDepth = depth_mat.clone();
    // Color map of depth show
    cv::Mat QDepth8( QDepth.cols, QDepth.rows, CV_8UC1);

    QDepth.convertTo(QDepth8,CV_8UC1, 255.0/MAX_DIST);
    cv::Mat QDepthRender;
    applyColorMap(QDepth8, QDepthRender, 2);
    QImage qdepth = QImage( (QDepthRender.data), QDepthRender.cols, QDepthRender.rows, QImage::Format_RGB888 );
    QImage qdepthshow = qdepth.scaled(QDepthRender.cols, QDepthRender.rows).scaled(480, 280, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->DepthImg->setPixmap(QPixmap::fromImage(qdepthshow));
    ui->DepthImg->resize(qdepthshow.size());

    // Calculate Color FPS
    qint64 Interval = timestamp - mLastColorTimeT;
    mLastColorTimeT = timestamp;
    if ( Interval > 0 && Interval < 1000){
        mSumColorTime = mSumColorTime + static_cast<uint>(Interval);
        if (mSumColorTime > 1000){
            mColor_frame_cnt = mColor_frame_cnt > 30 ? 30 : mColor_frame_cnt;
            ui->lable_color_fps->setText(QString::number(mColor_frame_cnt));
            ui->lable_depth_fps->setText(QString::number(mColor_frame_cnt));
            mColor_frame_cnt = 1;
            mSumColorTime = mSumColorTime - 1000;
        }
        else{
            mColor_frame_cnt++;
        }
    }
}

//=======================================
// UDP Listener
//=======================================
void rsCollectData::on_Button_UDP_clicked()
{
    ui->Button_UDP->setEnabled(false);
    ui->Button_StopUDP->setEnabled(true);
    udpSync = new udpthread();
    udpSync->startSync();

    connect(this,&rsCollectData::send_RGBD_name,udpSync,&udpthread::receive_Subject_Action);
    connect(udpSync,&udpthread::udp4startALL,this,&rsCollectData::on_Button_SAVEALL_clicked);
    connect(udpSync,&udpthread::udp4stopALL,this,&rsCollectData::on_Button_SAVESTOPALL_clicked);

    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    emit send_RGBD_name(Subject, Action, Index);

    // Show local IP address
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
            ui->label_IP->setText(QString(address.toString()));
    }
}

void rsCollectData::on_Button_StopUDP_clicked()
{
    ui->Button_UDP->setEnabled(true);
    ui->Button_StopUDP->setEnabled(false);
    disconnect(this,&rsCollectData::send_RGBD_name,udpSync,&udpthread::receive_Subject_Action);
    disconnect(udpSync,&udpthread::udp4startALL,this,&rsCollectData::on_Button_SAVEALL_clicked);
    disconnect(udpSync,&udpthread::udp4stopALL,this,&rsCollectData::on_Button_SAVESTOPALL_clicked);

    udpSync->stop();
    udpSync->quit();
    qDebug() << "UDP is stop now!";
}


//=============================
// Show and Save BLE slots
//=============================
void rsCollectData::on_Button_ScanBLE_clicked()
{
    // --- UI status
    ui->Button_ScanBLE->setEnabled(false);
    ui->Button_QuitBLE->setEnabled(true);
    ui->Button_ClearBLE->setEnabled(true);
    ui->Button_startSaveBLE->setEnabled(true);
    ui->Button_stopSaveBLE->setEnabled(false);

    // --- Create blethread
    EARSensor = new blethread();

    // --- Connect all signals and slots related to BLE
    connect(this, &rsCollectData::send_RGBD_name,    EARSensor, &blethread::receiveFileName);
    connect(this, &rsCollectData::send_BLEsave_flag, EARSensor, &blethread::receiveSaveFlag);
    connect(ui->Button_QuitBLE,SIGNAL(clicked()), EARSensor, SLOT(disconnectFromDevice()));
    connect(EARSensor, &blethread::sendItem,   this, &rsCollectData::receiveItem);
    connect(EARSensor, &blethread::resetGraph, this, &rsCollectData::reset_BLE_graph);
    connect(EARSensor, &blethread::updateGraph, this, &rsCollectData::show_BLE_graph);

    //connect(EARSensor, SIGNAL(sendItem(QListWidgetItem*)), this, SLOT(receiveItem(QListWidgetItem*)));
    //connect(this, SIGNAL(send_RGBD_name(QString, QString, QString)), EARSensor, SLOT(receiveFileName(QString, QString, QString)));
    //connect(ui->Button_QuitBLE,SIGNAL(clicked()), EARSensor, SLOT(disconnectFromDevice()) );
    //connect(EARSensor, SIGNAL(resetGraph()), this, SLOT(reset_BLE_graph()));
    //connect(EARSensor,SIGNAL(updateGraph(QVector<qint64>)),this, SLOT(show_BLE_graph(QVector<qint64>)));
    //connect(this, SIGNAL(send_BLEsave_flag(bool)), EARSensor, SLOT(receiveSaveFlag(bool)));

    // --- Emit name
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    emit send_RGBD_name(Subject, Action, Index);

    // --- Start BLE scan
    EARSensor->startDeviceDiscovery();
}

void rsCollectData::on_Button_QuitBLE_clicked()
{ 
    ui->Button_ScanBLE->setEnabled(true);
}

void rsCollectData::on_Button_startSaveBLE_clicked()
{
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    emit send_RGBD_name(Subject, Action, Index);

    ui->Button_stopSaveBLE->setEnabled(true);
    ui->Button_startSaveBLE->setEnabled(false);
    ui->Text_subject_name->setEnabled(false);
    ui->Text_subject_action->setEnabled(false);
    ui->Text_subject_index->setEnabled(false);
    save_BLE_flag = true;
    emit send_BLEsave_flag(save_BLE_flag);
}

void rsCollectData::on_Button_stopSaveBLE_clicked()
{
    ui->Button_stopSaveBLE->setEnabled(false);
    ui->Button_startSaveBLE->setEnabled(true);
    ui->Text_subject_name->setEnabled(true);
    ui->Text_subject_action->setEnabled(true);
    ui->Text_subject_index->setEnabled(true);
    save_BLE_flag = false;
    emit send_BLEsave_flag(save_BLE_flag);
}

void rsCollectData::receiveItem(QListWidgetItem *item){
    ui->List_BLE->addItem(item);
}

void rsCollectData::itemActivated(QListWidgetItem *item){
    QString text = item->text();
    ui->List_BLE->setEnabled(false);

    int index = text.indexOf(' ');
    if (index == -1)
        return;
    qDebug() << "Connect to: " << text;
    EARSensor->scanServices(text.left(index));
}

//=============================
// BLE GRAPH
//=============================

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

void rsCollectData::show_BLE_graph(QVector<qint64> BLEdata){
    ui->label_ConnectStatus->setText("<font color='green'>CONNECTED</font>");

    if (BLEdata[0] > mLastEarTime){
        mSumEarTime = mSumEarTime + BLEdata[0] - mLastEarTime;
        save_BLE_cnt++;
        if (mSumEarTime > 1000){
            ui->label_BLEfps->setText(QString::number(save_BLE_cnt));
            mSumEarTime = 0;
            save_BLE_cnt = 0;
        }

        if (mbShowGraph){
            // ========= PLOT ACC ========= //
            ui->customPlot_ACC->graph(0)->addData(key_ACC, BLEdata[1]);
            ui->customPlot_ACC->graph(1)->addData(key_ACC, BLEdata[2]);
            ui->customPlot_ACC->graph(2)->addData(key_ACC, BLEdata[3]);
            ui->customPlot_ACC->xAxis->setRange(key_ACC, 200, Qt::AlignRight);
            ++key_ACC;
            ui->customPlot_ACC->replot();
            // ========= PLOT GYR ========= //
            ui->customPlot_GYR->graph(0)->addData(key_GYR, BLEdata[4]);
            ui->customPlot_GYR->graph(1)->addData(key_GYR, BLEdata[5]);
            ui->customPlot_GYR->graph(2)->addData(key_GYR, BLEdata[6]);
            ui->customPlot_GYR->xAxis->setRange(key_GYR, 200, Qt::AlignRight);
            ++key_GYR;
            ui->customPlot_GYR->replot();
            // ========= PLOT MAG ========= //
            ui->customPlot_MAG->graph(0)->addData(key_MAG, BLEdata[7]);
            ui->customPlot_MAG->graph(1)->addData(key_MAG, BLEdata[8]);
            ui->customPlot_MAG->graph(2)->addData(key_MAG, BLEdata[9]);
            ui->customPlot_MAG->xAxis->setRange(key_MAG, 200, Qt::AlignRight);
            ++key_MAG;
            ui->customPlot_MAG->replot();
        }
    }
    mLastEarTime = BLEdata[0];
}

void rsCollectData::reset_BLE_graph(){
    // Set UI parameters
    ui->label_ConnectStatus->setText("<font color='red'>DISCONNECTED</font>");
    ui->List_BLE->setEnabled(true);
    ui->label_BLEfps->setText("0");

    // Reset Graph
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
}

//=============================
// UI STATUS CHANGED
//=============================

void rsCollectData::on_Text_subject_name_textChanged()
{
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    ui->Text_subject_index->setText("1");
    emit send_RGBD_name(Subject, Action, Index);
}

void rsCollectData::on_Text_subject_action_textChanged()
{
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    ui->Text_subject_index->setText("1");
    emit send_RGBD_name(Subject, Action, Index);
}

void rsCollectData::on_Text_subject_index_textChanged()
{
    QString Subject = ui->Text_subject_name->toPlainText();
    QString Action = ui->Text_subject_action->toPlainText();
    QString Index = ui->Text_subject_index->toPlainText();
    emit send_RGBD_name(Subject, Action, Index);
}

void rsCollectData::on_checkPlotGraph_stateChanged(int arg1)
{
    if (arg1 == 0)
        mbShowGraph = false;
    else
        mbShowGraph = true;
}

void rsCollectData::on_spatial_alpha_valueChanged(int value)
{
    value = value > 100 ? 100 : value;
    value = value < 25 ? 25 : value;
    if (mbStartRealsense)
        rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, float(value/100.0));
    ui->spatial_alpha_value->setNum(value/100.0);
}

void rsCollectData::on_spatial_delta_valueChanged(int value)
{
    value = value > 50 ? 50 : value;
    value = value < 1 ? 1 : value;
    if (mbStartRealsense)
        rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, value);
    ui->spatial_delta_value->setNum(value);
}

void rsCollectData::on_spatial_mag_valueChanged(int value)
{
    value = value > 5 ? 5 : value;
    value = value < 1 ? 1 : value;
    if (mbStartRealsense)
        rsFilter->spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, value);
    ui->spatial_mag_value->setNum(value);
}

void rsCollectData::on_temporal_alpha_valueChanged(int value)
{
    value = value > 100 ? 100 : value;
    value = value < 1 ? 1 : value;
    if (mbStartRealsense)
        rsFilter->temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, float(value/100.0));
    ui->temporal_alpha_value->setNum(value/100.0);
}

void rsCollectData::on_temporal_delta_valueChanged(int value)
{
    value = value > 100 ? 100 : value;
    value = value < 1 ? 1 : value;
    if (mbStartRealsense)
        rsFilter->temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, value);
    ui->temporal_delta_value->setNum(value);
}

void rsCollectData::on_combo_persistency_currentIndexChanged(int index)
{
    if (mbStartRealsense)
        rsFilter->temp_filter.set_option(RS2_OPTION_HOLES_FILL, index);
}

void rsCollectData::on_checkEmitter_stateChanged(int arg1)
{
    if (arg1 == 2 && mbStartRealsense){
        if (rsCapture->depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)){
            rsCapture->depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f);
        }
    }
    if (arg1 == 0 && mbStartRealsense){
        if (rsCapture->depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)){
            rsCapture->depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0.f);
        }
    }
}

void rsCollectData::on_MAX_valueChanged(int value)
{
    // UPDATE VALUE AND UPDATE THE DEPTH MAXIMUM VALUE
    if (mbStartRealsense)
        MAX_DIST = value;

    ui->MAX_value->setNum(value);
}

void rsCollectData::on_disparity_valueChanged(int value)
{
    // SET THE DISPARITY VALUE AND UPDATE THE REALSENSE PARAMETER
    send_Disparity(value);
    ui->disparity_value->setNum(value);
}

void rsCollectData::on_Emitter_power_valueChanged(int value)
{
    if (mbStartRealsense){
        if (rsCapture->depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)){
            rsCapture->depth_sensor.set_option(RS2_OPTION_LASER_POWER, value);
        }
    }

    ui->Emitter_power_value->setNum(value);
}
