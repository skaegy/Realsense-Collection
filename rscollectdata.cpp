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
    ui->Button_saveRGBD->setEnabled(false);
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_RescanBLE->setEnabled(false);

    connect(ui->Button_ScanBLE, SIGNAL(clicked()), this, SLOT(startScan()));
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
    QBluetoothAddress adapterAddress = localDevice->address();
    connect(ui->List_BLE, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(itemActivated(QListWidgetItem*)));

}

rsCollectData::~rsCollectData()
{
    rsCapture->stop();
    delete ui;
}

void rsCollectData::on_OpenRSButton_clicked()
{
    ui->OpenRSButton->setEnabled(false);

    pipeline pipe;
    config rs_cfg;
    rs_cfg.enable_stream(RS2_STREAM_DEPTH, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_Z16, IN_FRAME); // Enable default depth
    rs_cfg.enable_stream(RS2_STREAM_COLOR, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_RGB8, IN_FRAME);
    rs_cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    rs_cfg.enable_stream(RS2_STREAM_INFRARED, 2);

    // frame_queue to process filtering and alignment
    const auto CAPACITY = 3; // allow max latency of 5 frames
    frame_queue processed_depth(CAPACITY);
    frame_queue processed_color(CAPACITY);
    frame_queue original_color(CAPACITY);
    frame_queue original_depth(CAPACITY);
    align align(RS2_STREAM_COLOR);
    colorizer color_map;

    // depth filter
    decimation_filter dec_filter;
    dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spatial_filter spat_filter;    //
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);
    temporal_filter temp_filter;   //
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.8);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 50);
    disparity_transform depth_to_disparity(true);
    disparity_transform disparity_to_depth(false);

    try{
        pipeline_profile rs_device = pipe.start(rs_cfg);
        auto depth_stream = rs_device.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
        while(1){

        // rs_frame
        frameset rs_d415 = pipe.wait_for_frames();
        frame depth_frame = rs_d415.get_depth_frame();
        frame color_frame = rs_d415.get_color_frame();
        //frame depth_show = depth_frame.apply_filter(color_map);
        frame depth_show = color_map(depth_frame);

        // OpenCV frame
        Mat color_cv(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, (void*)color_frame.get_data(), Mat::AUTO_STEP);
        cvtColor( color_cv, color_cv, CV_BGR2RGB);
        Mat depth_cv16(Size(IMG_WIDTH, IMG_HEIGHT), CV_16UC1, (void*)depth_frame.get_data(), Mat::AUTO_STEP);
        Mat depth_cv8(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, (void*)depth_show.get_data(), Mat::AUTO_STEP);


        show_color_mat(color_cv);
        show_depth_mat(depth_cv16);


        QImage qcolor = QImage( (const unsigned char*)(color_cv.data), color_cv.cols, color_cv.rows, QImage::Format_RGB888 );
        QImageWriter writer("a.jpeg","jpeg" );
        //writer.write(qcolor);

        QDateTime currTime = QDateTime::currentDateTime();
        QString str = currTime.toString("yyyy-MM-dd hh:mm:ss ddd");
        ui->LabelTime->setText(str);
        int timeT = currTime.toTime_t();
        QTextStream(stdout) << timeT << endl;

        QEventLoop eventloop;
        QTimer::singleShot(5, &eventloop, SLOT(quit()));
        eventloop.exec();

        QTextStream(stdout) << frame_cnt << endl;
        frame_cnt++;
        }

    }
    catch(const rs2::error & e){
        QTextStream(stdout) << "RealSense error calling " << QString::fromStdString(e.get_failed_function()) \
                            << "(" << QString::fromStdString(e.get_failed_args()) \
                            << "):\n " << QString::fromStdString(e.what()) << endl;
    }
}

void rsCollectData::on_Button_openRSThread_clicked()
{
    ui->Button_saveRGBD->setEnabled(true);
    rsCapture = new rsCaptureThread();
    rsCapture->startCollect();
    rsFiltered = new rsFilteredThread();
    rsFiltered->startFilter();

    qRegisterMetaType< cv::Mat >("cv::Mat");
    QObject::connect(rsFiltered,SIGNAL(sendColorFiltered(cv::Mat)),this,SLOT(show_color_mat(cv::Mat)));
    QObject::connect(rsFiltered,SIGNAL(sendDepthFiltered(cv::Mat)),this,SLOT(show_depth_mat(cv::Mat)));

    //QObject::connect(rsCapture,SIGNAL(sendColorMat(cv::Mat)),this,SLOT(show_color_mat(cv::Mat)));
    //QObject::connect(rsCapture,SIGNAL(sendDepthMat(cv::Mat)),this,SLOT(show_depth_mat(cv::Mat)));
}

void rsCollectData::on_Button_closeRSThread_clicked()
{
    rsCapture->stop();
    rsFiltered->stop();
    //close();
}

void rsCollectData::on_Button_saveRGBD_clicked()
{
    ui->Button_stopSaveRGBD->setEnabled(true);
    ui->Button_saveRGBD->setEnabled(false);
    save_flag = true;
    QObject::connect(rsFiltered,SIGNAL(sendColorFiltered(cv::Mat)),this,SLOT(save_color_mat(cv::Mat)));
    QObject::connect(rsFiltered,SIGNAL(sendDepthFiltered(cv::Mat)),this,SLOT(save_depth_mat(cv::Mat)));
}

void rsCollectData::on_Button_stopSaveRGBD_clicked()
{
    ui->Button_stopSaveRGBD->setEnabled(false);
    ui->Button_saveRGBD->setEnabled(true);
    QObject::disconnect(rsFiltered,SIGNAL(sendColorFiltered(cv::Mat)),this,SLOT(save_color_mat(cv::Mat)));
    QObject::disconnect(rsFiltered,SIGNAL(sendDepthFiltered(cv::Mat)),this,SLOT(save_depth_mat(cv::Mat)));
    save_color_cnt = 1;
    save_depth_cnt = 1;
}


// Show color image & fps on the UI
void rsCollectData::show_color_mat(Mat color_mat){
    //cvtColor(color_mat, color_mat, COLOR_BGR2RGB);
    QImage qcolor = QImage( (const unsigned char*)(color_mat.data), color_mat.cols, color_mat.rows, QImage::Format_RGB888);
    QImage qcolorshow = qcolor.scaled(color_mat.cols, color_mat.rows).scaled(320, 180, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->RGBImg->setPixmap(QPixmap::fromImage(qcolorshow));
    ui->RGBImg->resize(qcolorshow.size());

    // Calculate FPS
    QDateTime currTime = QDateTime::currentDateTime();
    int CurrTimeT = currTime.toTime_t();
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

void rsCollectData::show_depth_mat(Mat depth_mat){
    QImage qdepth = QImage( (const unsigned char*)(depth_mat.data), depth_mat.cols, depth_mat.rows, QImage::Format_RGB16 );
    QImage qdepthshow = qdepth.scaled(depth_mat.cols, depth_mat.rows).scaled(320, 180, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->DepthImg->setPixmap(QPixmap::fromImage(qdepthshow));
    ui->DepthImg->resize(qdepthshow.size());
}

void rsCollectData::save_color_mat(Mat color_mat){
    //cvtColor(color_mat, color_mat, COLOR_BGR2RGB);
    char color_file_name[50];
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
    QTextStream(stdout) << "Saving " << color_file_name << endl;
    imwrite(color_file_name, color_mat);
    save_color_cnt++;
}

void rsCollectData::save_depth_mat(Mat depth_mat){
    char depth_file_name[50];
    qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
    QTextStream(stdout) << "Saving " << depth_file_name << endl;
    imwrite(depth_file_name, depth_mat);
    save_depth_cnt++;
}

//=============================
// BLE process slot
//=============================
void rsCollectData::startScan()
{

    discoveryAgent->start();
    ui->Button_RescanBLE->setEnabled(true);
    ui->Button_ScanBLE->setEnabled(false);

}

void rsCollectData::addDevice(const QBluetoothDeviceInfo &info)
{

    QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());
    QList<QListWidgetItem *> items = ui->List_BLE->findItems(label, Qt::MatchExactly);
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

void rsCollectData::scanFinished()
{
    ui->Button_ScanBLE->setEnabled(true);
}

void rsCollectData::on_Button_RescanBLE_clicked()
{

    discoveryAgent->stop();
    ui->List_BLE->clear();
    startScan();

}

void rsCollectData::itemActivated(QListWidgetItem *item)
// Double clicked
{
    QString text = item->text();
    int index = text.indexOf(' ');
    if (index == -1)
        return;
    QBluetoothAddress address(text.left(index));
    QString name(text.mid(index + 1));
}
