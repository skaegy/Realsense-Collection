#ifndef SETTINGS_H
#define SETTINGS_H

#include<QDebug>
// Realsense Parameters
const int IMG_WIDTH = 1280;
const int IMG_HEIGHT = 720;
const int IN_FRAME = 30;
const int CAPACITY = 1;

// UDP parameters
const int PORT_SCAN = 8888;
const unsigned long BUF_SIZE = 128;
const int PORT_MS = 2;

// Ear sensor parameters
const QString ear_uuid = "47442014-0f63-5b27-9122-728099603712";
const QString IMU_uuid = "47442020-0f63-5b27-9122-728099603712";



#endif // SETTINGS_H
