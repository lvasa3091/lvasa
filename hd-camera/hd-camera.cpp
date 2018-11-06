/****************************************************************************
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

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

/*
#include <iostream>
#include <string>
#include <libsocket/inetserverstream.hpp>
#include <libsocket/exception.hpp>
#include <unistd.h>
#include <stdio.h>
#include <utility>
#include <memory>
#include <libsocket/socket.hpp>
#include <libsocket/select.hpp>

using std::string;
using std::unique_ptr;

using libsocket::inet_stream_server;
using libsocket::inet_stream;
using libsocket::selectset;

string host = "::1";
string port = "1235";
string answ;

*/

void error(const char *msg)
{
    perror(msg);
    system("killall getjpg.sh");
    printf("END...\n\r");
    exit(1);
}

int countJpeg = 0, countOut = 0;

#define WINDOW_NAME "HD Camera test"
#define DEFAULT_DEVICE_FILE "/dev/v4l/by-path/pci-0000:00:03.0-video-index2"
// #define DEFAULT_DEVICE_FILE "/dev/v4l/by-path/pci-0000:00:14.0-usb-0:4:1.4-video-index0"
#define DEFAULT_DEVICE_ID 0
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_RGB565

#define POLL_ERROR_EVENTS (POLLERR | POLLHUP | POLLNVAL)

#define LOCALHOST "127.0.0.1"
// #define SERVER_IP "192.168.0.153"
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

int sockfd, portno, n, imgSize, photo_enable = 1, IM_HEIGHT, IM_WIDTH;
struct sockaddr_in serv_addr;
string folderName, fileName, fileExt = ".jpg";

static void _camera_callback(const void *img, size_t len, const struct timeval *timestamp)
{
	Mat original(_height, _width, CV_8UC2, (uchar*)img);
	Mat imgbgr;
	string str = "";
	vector<uchar> imgBuf;
	vector<int> quality_params = vector<int>(2);              	// Вектор параметров качества сжатия
	quality_params[0] = CV_IMWRITE_JPEG_QUALITY; 			// Кодек JPEG
	quality_params[1] = 100;                                        // По умолчанию качество сжатия (95) 0-100

	cvtColor(original, imgbgr, CV_BGR5652BGR);

	int height = imgbgr.rows;
	int width = imgbgr.cols;


	// resize to monitor resolution
//	Size size(1920, 1080);
	// Size size(1024, 768);
//	Mat dest;
//	resize(imgbgr, dest, size);

//	imshow(WINDOW_NAME, dest);
        flip(imgbgr, imgbgr, 0);
	if (!(countJpeg % 20) && photo_enable) {
//		sprintf(str,"%d",countOut++);
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
/*
	Mat cropped = Mat(imgbgr, Rect(width/2 - width/7,
                                       height/2 - height/9,
                                       2*width/7, 2*height/7));
	imgbgr = cropped;
*/
	IM_HEIGHT = FRAME_HEIGHT;
        IM_WIDTH = FRAME_WIDTH;

	resize(imgbgr, imgbgr, Size( IM_WIDTH , IM_HEIGHT ));
//	flip(imgbgr, imgbgr, 0);
//	rotate(imgbgr, imgbgr, ROTATE_90_CLOCKWISE);

	imgSize=imgbgr.total()*imgbgr.elemSize();
	n = send(sockfd, imgbgr.data, imgSize, 0);
	if (n < 0) error("ERROR writing to socket");

//	printf("countJpeg = %d",countJpeg);
//	imencode(".jpg", imgbgr, imgBuf, quality_params);
//	send(clientSocket, (const char*)&imgBuf[0], static_cast<int>(imgBuf.size()), 0);
}

static void _camera_stream_read(int fd)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;

	int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	if (ret) {
		printf("Error getting frame from camera: %s\n", strerror(errno));
		return;
	}

	_camera_callback((const void *)buf.m.userptr, buf.length, &buf.timestamp);

	// give buffer back to backend
	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if (ret) {
		printf("Error returning buffer to backend\n");
	}
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
	struct pollfd desc[1];

	desc[0].fd = fd;
	desc[0].events = POLLIN | POLLPRI | POLL_ERROR_EVENTS;
	desc[0].revents = 0;

//	namedWindow(WINDOW_NAME, WINDOW_AUTOSIZE);
//	startWindowThread();

	_signal_handler_setup();

	while (_should_run) {
		int ret = poll(desc, sizeof(desc) / sizeof(struct pollfd), -1);
		if (ret < 1) {
			continue;
		}

		_camera_stream_read(fd);
	}

//	destroyAllWindows();
}

static int _backend_user_ptr_streaming_init(int fd, uint32_t sizeimage)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	int pagesize;
	size_t buffer_len;

	// initialize v4l2 backend
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = BUFFER_LEN;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if (ioctl(fd, VIDIOC_REQBUFS, &req)) {
		goto error;
	}

	// allocate buffer
	pagesize = getpagesize();
	buffer_len = (sizeimage + pagesize - 1) & ~(pagesize - 1);

	for (uint8_t i = 0; i < BUFFER_LEN; i++) {
		_buffers[i] = memalign(pagesize, buffer_len);
		if (!_buffers[i]) {
			buffer_len = 0;
			while (i) {
				i--;
				free(_buffers[i]);
				_buffers[i] = NULL;
			}

			goto error;
		}
	}

	// give buffers to backend
	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	buf.length = buffer_len;

	for (uint8_t i = 0; i < BUFFER_LEN; i++) {
		buf.index = i;
		buf.m.userptr = (unsigned long)_buffers[i];
		if (ioctl(fd, VIDIOC_QBUF, &buf)) {
			printf("Error giving buffers to backend: %s | i=%i\n", strerror(errno), i);
			goto backend_error;
		}
	}

	return 0;

backend_error:
	for (uint8_t i = 0; i < BUFFER_LEN; i++) {
		free(_buffers[i]);
		_buffers[i] = NULL;
	}
error:
	return -1;
}

static int _init(const char *device)
{
	struct stat st;
	int camera_id = DEFAULT_DEVICE_ID;
	struct v4l2_streamparm parm;
	struct v4l2_format fmt;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int ret = stat(device, &st);
	if (ret) {
		printf("Unable to get device stat\n");
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		printf("Device is not a character device\n");
		return -1;
	}

	int fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		printf("Unable to open device file descriptor: %s\n", device);
		return -1;
	}

	// set device_id
	ret = ioctl(fd, VIDIOC_S_INPUT, (int *)&camera_id);
	if (ret) {
		printf("Error setting device id: %s\n", strerror(errno));
		goto error;
	}

	// set stream parameters
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.capturemode = CAPTURE_MODE_PREVIEW;
	ret = ioctl(fd, VIDIOC_S_PARM, &parm);
	if (ret) {
		printf("Unable to set stream parameters: %s\n", strerror(errno));
		goto error;
	}

	// set pixel format
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = _width;
	fmt.fmt.pix.height = _height;
	fmt.fmt.pix.pixelformat = DEFAULT_PIXEL_FORMAT;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret) {
		printf("Setting pixel format: %s\n", strerror(errno));
		goto error;
	}

	ret = _backend_user_ptr_streaming_init(fd, fmt.fmt.pix.sizeimage);
	if (ret) {
		printf("Error initializing streaming backend: %s\n", strerror(errno));
		goto error;
	}

	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret) {
		printf("Error starting streaming: %s\n", strerror(errno));
	}

	return fd;

error:
	close(fd);
	return -1;
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
      		error("ERROR connecting");

	int fd = _init(DEFAULT_DEVICE_FILE);
	if (fd == -1) {
		return -1;
	}

        time (&rawtime);
        timeinfo = localtime ( &rawtime );
	string dateStr = to_string(1900 + timeinfo->tm_year) + "-" + to_string(timeinfo->tm_mon) + "-" + to_string(timeinfo->tm_mday) + "_" + \
			 to_string(timeinfo->tm_hour) + "-" + to_string(timeinfo->tm_min) + "-" + to_string(timeinfo->tm_sec);
	string folderCreateCommand = "mkdir " + dateStr;
	system(folderCreateCommand.c_str());
	folderName = dateStr + (string)"/";
//        folderName = asctime(timeinfo) + (string)"/";
	system("./getjpg.sh &");

	_loop(fd);
	_shutdown(fd);
	close(sockfd);
	system("bash killall getjpg.sh");
	cout << "END...";
	return 0;
}
