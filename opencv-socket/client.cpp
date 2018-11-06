#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <netinet/in.h>

#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define LOCALHOST "127.0.0.1"
#define SERVER_IP "192.168.0.153"
#define PORT 7200
#define FRAME_WIDTH         640
#define FRAME_HEIGHT        480

using namespace std;
using namespace cv;

static uint32_t _height, _width;
int countJpeg = 0;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

static void _camera_callback(const void *img, size_t len, const struct timeval *timestamp)
{
        Mat original(_height, _width, CV_8UC2, (uchar*)img);
        Mat imgbgr;
        string fileName, fileExt = ".jpg";
//      char str[16] = "";
        time_t rawtime;
        struct tm * timeinfo;
        vector<uchar> imgBuf;
        vector<int> quality_params = vector<int>(2);                    // Вектор параметров качества сж$
        quality_params[0] = CV_IMWRITE_JPEG_QUALITY;                    // Кодек JPEG
        quality_params[1] = 100;                                        // По умолчанию качество сжатия $

        cvtColor(original, imgbgr, CV_BGR5652BGR);

        // resize to monitor resolution
        Size size(1920, 1080);
        // Size size(1024, 768);
//      Mat dest;
//      resize(imgbgr, dest, size);

//      imshow(WINDOW_NAME, dest);
//      sprintf(str,"%d",countJpeg++);
        time (&rawtime);
        timeinfo = localtime ( &rawtime );
        fileName = asctime(timeinfo) + fileExt;
        if (!(countJpeg % 10)) {
                imwrite(fileName, imgbgr, quality_params);
                printf("countJpeg = %d\n\r",countJpeg);
        }
        countJpeg = (countJpeg == 10000)?0:countJpeg + 1;
//      printf("countJpeg = %d",countJpeg);
//      imencode(".jpg", imgbgr, imgBuf, quality_params);
//      send(clientSocket, (const char*)&imgBuf[0], static_cast<int>(imgBuf.size()), 0);
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

int main()
{
  int sockfd, portno, n, imgSize, IM_HEIGHT, IM_WIDTH;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];
  Mat cameraFeed;

  portno = PORT;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) error("ERROR opening socket");

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

  VideoCapture capture;

	capture.open(1);

  while(true)
  {
    /* store image to matrix && test frame */
    if(!capture.read(cameraFeed))
    {
        cout << "Video Ended" << endl;
        break;
    }

    int height = cameraFeed.rows;
    int width = cameraFeed.cols;

    Mat cropped = Mat(cameraFeed, Rect(width/2 - width/7,
                                       height/2 - height/9,
                                       2*width/7, 2*height/7));
    cameraFeed = cropped;

    IM_HEIGHT = FRAME_HEIGHT;
    IM_WIDTH = FRAME_WIDTH;

    resize(cameraFeed, cameraFeed, Size( IM_WIDTH , IM_HEIGHT ));

    imgSize=cameraFeed.total()*cameraFeed.elemSize();

    n = send(sockfd, cameraFeed.data, imgSize, 0);
    if (n < 0) error("ERROR writing to socket");
  }

  close(sockfd);

  return 0;
}
