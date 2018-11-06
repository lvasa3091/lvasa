/*******************************************************************************

Intel Realsene SDK
The program starts Color and Depth Stream using Intel Realsense SDK 
and converting its frame from PXCImage to Mat variable. 
Easy for Image processing in Intel Realsense SDK Camera.

*******************************************************************************/


# #include <windows.h>
#include <wchar.h>
#include <pxcsensemanager.h>
#include "util_render.h"  //SDK provided utility class used for rendering (packaged in libpxcutils.lib)

#include <conio.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <wchar.h>
#include <vector>
#include "pxcsession.h"
#include "pxccapture.h"
#include "util_render.h"
#include "util_cmdline.h"
#include "pxcprojection.h"
#include "pxcmetadata.h"
#include "util_cmdline.h"

using namespace std;
using namespace cv;

int wmain(int argc, WCHAR* argv[])
{
	cout << "Intel Realsense SDK Hacking using Opencv" << endl;
    cout << "Intel Realsense Camera SDK Frame Capture in opencv Mat Variable       -- by Deepak" << endl;
    cout << "Compiled with OpenCV version " << CV_VERSION << endl << endl;

	PXCSenseManager *psm=0;
	psm = PXCSenseManager::CreateInstance();
	if (!psm) {
		wprintf_s(L"Unable to create the PXCSenseManager\n");
		return 1;
	}

	psm->EnableStream(PXCCapture::STREAM_TYPE_COLOR, 640, 480); //depth resolution
	psm->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, 640, 480); //depth resolution
	psm->Init();

	UtilRender color_render(L"Color Stream");
	UtilRender depth_render(L"Depth Stream");

	///////////// OPENCV
	IplImage *image=0;
	CvSize gab_size;
	gab_size.height=480;
	gab_size.width=640;
	image=cvCreateImage(gab_size,8,3);

	IplImage *depth=0;
	CvSize gab_size_depth;
	gab_size_depth.height=240;
	gab_size_depth.width=320;
	depth=cvCreateImage(gab_size,8,1);

	PXCImage::ImageData data;
	PXCImage::ImageData data_depth;

	unsigned char *rgb_data;
	float *depth_data;
	
	PXCImage::ImageInfo rgb_info;
	PXCImage::ImageInfo depth_info;

	///////
	for (;;) 
	{
	if (psm->AcquireFrame(true)<PXC_STATUS_NO_ERROR) break;
	
		PXCCapture::Sample *sample = psm->QuerySample();
		PXCImage *colorIm,*depthIm;

		// retrieve the image or frame by type from the sample
		colorIm = sample->color;
		depthIm = sample->depth;

		PXCImage *color_image = colorIm;
		PXCImage *depth_image = depthIm;

		color_image->AcquireAccess(PXCImage::ACCESS_READ_WRITE,PXCImage::PIXEL_FORMAT_RGB24,&data);
		depth_image->AcquireAccess(PXCImage::ACCESS_READ_WRITE,&data_depth);

		rgb_data=data.planes[0];
		for(int y=0; y<480; y++)
		{
			for(int x=0; x<640; x++)
			{ 
				for(int k=0; k<3 ; k++)
				{
					image->imageData[y*640*3+x*3+k]=rgb_data[y*640*3+x*3+k];
				}
			}
		}

		short* depth_data = (short*)data_depth.planes[0]; //
		for(int y=0; y<920; y++)
		{
			for(int x=0; x<320; x++)
			{ 
			depth->imageData[y*320+x] = depth_data[y*320+x];
			}
		}

		color_image->ReleaseAccess(&data);
		depth_image->ReleaseAccess(&data_depth);

		cv::Mat rgb(image);
		imshow("Color_cv2",rgb);
		cv::Mat dep(depth);
		imshow("depth_cv2",dep);
		/////////////opencv

		if( cvWaitKey(10) >= 0 )
			break;

		////////

		if (!color_render.RenderFrame(color_image)) break;
		if (!depth_render.RenderFrame(depth_image)) break;
		psm->ReleaseFrame();

	}
		cvReleaseImage(&image);
		cvReleaseImage(&depth);
		psm->Release();
	return 0;
}
