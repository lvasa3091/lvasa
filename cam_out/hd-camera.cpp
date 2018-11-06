#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <malloc.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>

#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int countJpeg = 0, countOut = 0;

//#define cam "HD Camera test"
// #define WINDOW_NAME "HD Camera test"
#define DEFAULT_DEVICE_FILE "/dev/v4l/by-path/pci-0000:00:03.0-video-index2"
#define DEFAULT_DEVICE_ID 0
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_RGB565

#define POLL_ERROR_EVENTS (POLLERR | POLLHUP | POLLNVAL)

#define LOCALHOST "127.0.0.1"
//#define SERVER_IP "192.168.0.153"
#define SERVER_IP "10.42.0.10"
#define PORT 7200
#define FRAME_WIDTH         320 // 640
#define FRAME_HEIGHT        240 // 480

enum {
	CAPTURE_MODE_PREVIEW = 0x8000,
	CAPTURE_MODE_VIDEO = 0x4000,
	CAPTURE_MODE_STILL = 0x2000
};

static uint32_t _height, _width;

#define BUFFER_LEN 4
static void *_buffers[BUFFER_LEN];
static bool _should_run = true;

using namespace cv;
using namespace std;

int sockfd, portno, n, imgSize, photo_enable = 1, video_enable = 1, socket_enable = 1, IM_HEIGHT, IM_WIDTH;
struct sockaddr_in serv_addr;
string folderName, fileName, fileExt = ".jpg";
VideoCapture cam(13);          // video13

static void _camera_callback(const void *img, size_t len, const struct timeval *timestamp)
{
//	Mat original(_height, _width, CV_8UC2, (uchar*)img);
	Mat imgbgr;
	string str = "";
	vector<uchar> imgBuf;
	vector<int> quality_params = vector<int>(2);              	// Вектор параметров качества сжатия
	quality_params[0] = CV_IMWRITE_JPEG_QUALITY; 			// Кодек JPEG
	quality_params[1] = 100;                                        // По умолчанию качество сжатия (95) 0-100

//	cvtColor(original, imgbgr, CV_BGR5652BGR);

//	Size size(1920, 1080);
//	resize(uu,imgbgr,size);

	cam.read(imgbgr);
	int height = imgbgr.rows;
	int width = imgbgr.cols;

	// resize to monitor resolution
//	Size size(1920, 1080);
	// Size size(1024, 768);
//	Mat dest;
//	resize(imgbgr, dest, size);

//	imshow(WINDOW_NAME, dest);
//        flip(imgbgr, imgbgr, 0);
	if (!(countJpeg % 10) && photo_enable) {
		fileName = folderName + to_string(countOut++) + fileExt;
		try {
			imwrite(fileName, imgbgr, quality_params);
    		}
    		catch (runtime_error& ex) {
        		fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
        		error("ERROR writing file");
    		}
		cout << fileName << "\n\r";
	}
	countJpeg = (countJpeg == 10000)?0:countJpeg + 1;

	IM_HEIGHT = FRAME_HEIGHT;
        IM_WIDTH = FRAME_WIDTH;
	if (socket_enable && video_enable) {
		resize(imgbgr, imgbgr, Size( IM_WIDTH , IM_HEIGHT ));
		imgSize=imgbgr.total()*imgbgr.elemSize();
		n = send(sockfd, imgbgr.data, imgSize, 0);
		if (n < 0) error("ERROR writing to socket");
	}

}

static void _camera_stream_read(int fd)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;

//	int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
//	if (ret) {
//		printf("Error getting frame from camera: %s\n", strerror(errno));
//		return;
//	}

	_camera_callback((const void *)buf.m.userptr, buf.length, &buf.timestamp);

	// give buffer back to backend
//	ret = ioctl(fd, VIDIOC_QBUF, &buf);
//	if (ret) {
//		printf("Error returning buffer to backend\n");
//	}
}

static void _exit_signal_handler(int signum)
{
    _should_run = false;
}

static void _signal_handler_setup()
{
    struct sigaction sa = { };

    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = _exit_signal_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}


static void _loop(int fd)
{
//	struct pollfd desc[1];

//	desc[0].fd = fd;
//	desc[0].events = POLLIN | POLLPRI | POLL_ERROR_EVENTS;
//	desc[0].revents = 0;

//	namedWindow(WINDOW_NAME, WINDOW_AUTOSIZE);
//	startWindowThread();

//	_signal_handler_setup();

	while (_should_run) {
//		int ret = poll(desc, sizeof(desc) / sizeof(struct pollfd), -1);
//		if (ret < 1) {
//			continue;
//		}

		_camera_stream_read(fd);
	}

//	destroyAllWindows();
}

static void _shutdown(int fd)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret) {
		printf("Error starting streaming: %s\n", strerror(errno));
	}

	for (uint8_t i = 0; i < BUFFER_LEN; i++) {
		free(_buffers[i]);
		_buffers[i] = NULL;
	}
}

int main (int argc, char *argv[])
{
	// TODO parse arguments to make easy change camera parameters
	struct hostent *server;
	char buffer[256];
	time_t rawtime;
        struct tm * timeinfo;
	_width = DEFAULT_WIDTH;
	_height = DEFAULT_HEIGHT;
	portno = PORT;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");
	// server = gethostbyname(LOCALHOST);
	server = gethostbyname(SERVER_IP);

  	if (server == NULL) {
      		fprintf(stderr,"ERROR, no such host\n");
      		exit(0);
  	}

  	bzero((char *) &serv_addr, sizeof(serv_addr));
  	serv_addr.sin_family = AF_INET;
  	bcopy((char *)server->h_addr,
       	(char *)&serv_addr.sin_addr.s_addr,
       	server->h_length);
  	serv_addr.sin_port = htons(portno);

  	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
      		socket_enable = 0; // error("ERROR connecting");


	if (!cam.isOpened()) {
		error("cannot open camera");
	}

//	cam.set(CV_CAP_PROP_FRAME_WIDTH,1920);
//	cam.set(CV_CAP_PROP_FRAME_HEIGHT,1080);
/*
	int fd = _init(DEFAULT_DEVICE_FILE);
	if (fd == -1) {
		return -1;
	}
*/
        time (&rawtime);
        timeinfo = localtime ( &rawtime );
	string dateStr = to_string(1900 + timeinfo->tm_year) + "-" + to_string(timeinfo->tm_mon) + "-" + to_string(timeinfo->tm_mday) + "_" + \
			 to_string(timeinfo->tm_hour) + "-" + to_string(timeinfo->tm_min) + "-" + to_string(timeinfo->tm_sec);
	string folderCreateCommand = "mkdir " + dateStr;
	system(folderCreateCommand.c_str());
	folderName = dateStr + (string)"/";
//        folderName = asctime(timeinfo) + (string)"/";

	_loop(0);
//	_shutdown(fd);
	if (socket_enable) close(sockfd);
	return 0;
}
