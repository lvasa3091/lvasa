#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <regex>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace std;

int main() {

boost::filesystem::path path( "/dev/video13" );
auto target = boost::filesystem::canonical(path).string();
std::regex exp( ".*video(\\d+)" );
std::smatch match;
std::regex_search( target, match, exp );
//auto id = strtod( match[1] );
//auto cap = cv::VideoCapture( id );
cout << "<-";
cout << match[1];
cout << "->";

VideoCapture cap(13);
// int frame_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
// int frame_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

//VideoWriter video("outcpp.avi",CV_FOURCC('M','J','P','G'),10, Size(frame_width,frame_height));

  while(1)

if (!cap.isOpened()) {
cout << "cannot open camera";
}

//unconditional loop
while (true) {
	Mat cameraFrame;
	cap.read(cameraFrame);
	imshow("cam", cameraFrame);
//	video.write(cameraFrame);
	if (waitKey(30) >= 0)
		break;
}

cap.release();
//video.release();
destroyAllWindows();
return 0;
}
