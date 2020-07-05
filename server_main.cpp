/* Multitouch Finger Detection Framework v4.0
* for VS2013 & OpenCV 2.4.10
*
* Bjoern Froemmer, January 2010 - March 2015
*/

#include "opencv2/opencv.hpp"
#include <time.h>
#include <math.h>
#include <string>
#include <TuioServer.h>
#include <TuioTime.h>
#include <TuioCursor.h>

using namespace cv;
using namespace TUIO;

TuioServer *server = new TuioServer();
int countGlobal = 0;
cv::VideoCapture cap("../mt_camera_raw.AVI"); //VideoCapture cap(0); // use the first camera found on the system
double videoWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH); // get the frame size of the videocapture object
double videoHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT); // get the frame size of the videocapture object

float distance(Point2f one, Point2f two) {
	float diffx = (two.x - one.x);
	float diffy = (two.y - one.y);
	float dist = 0;
	diffx = (diffx * diffx);
	diffy = (diffy * diffy);
	dist = sqrt(diffx + diffy);
	return dist;
}

//normiert den übergebenen Point2f
Point2f norming(Point2f basis) {
	basis.x = (basis.x / videoWidth);
	basis.y = (basis.y / videoHeight);
	return basis;
}

int main(void)
{
	if (!cap.isOpened())
	{
		std::cout << "ERROR: Could not open camera / video stream.\n";
		return -1;
	}

	cv::Mat frame, original, grey, bgFrame, bsFrame, hpFrame, outputFrame;
	Size kSize = Size(23, 23);
	Size kSize2 = Size(10, 10);
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	struct SingleTrackingObject{ int id; Point2f centerPoint; TuioCursor *tcur; };
	vector<SingleTrackingObject> trackingObjects;
	float maxSP = 27;



	int currentFrame = 0; // frame counter
	clock_t ms_start, ms_end, ms_time; // time

	//---------------------------------


	//---------------------------------

	char buffer[10]; // buffer for int to ascii conversion -> itoa(...)

	for (;;)
	{
		server->initFrame(TuioTime::getSessionTime());

		ms_start = clock(); // time start

		float x = 0.1;

		cap >> frame; // get a new frame from the videostream

		if (frame.data == NULL) // terminate the program if there are no more frames
		{
			std::cout << "TERMINATION: Camerastream stopped or last frame of video reached.\n";
			break;
		}

		original = frame.clone(); // copy frame to original

		cvtColor(original, grey, CV_BGR2GRAY); // convert frame to greyscale image (copies the image in the process!)

		//--------------------------
		if (currentFrame == 0)
		{
			bgFrame = grey.clone();
		}

		absdiff(bgFrame, grey, bsFrame);
		blur(bsFrame, hpFrame, kSize);
		absdiff(bsFrame, hpFrame, hpFrame);
		blur(hpFrame, hpFrame, kSize2);
		threshold(hpFrame, outputFrame, 12, 255, THRESH_BINARY);
		findContours(outputFrame, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);


		if (hierarchy.size() > 0)
		{
			int count = 0;
			vector<SingleTrackingObject> current;

			for (int idx = 0; idx >= 0; idx = hierarchy[idx][0])
			{
				SingleTrackingObject currentObj;
				// check contour size (number of points) and area ("blob" size)
				if (contourArea(Mat(contours.at(idx))) > 30 && contours.at(idx).size() > 4)
				{
					ellipse(original, fitEllipse(Mat(contours.at(idx))), Scalar(0, 0, 255), 1, 8);
					// fit & draw ellipse to contour at index
					drawContours(original, contours, idx, Scalar(255, 0, 0), 1, 8, hierarchy);
					// draw contour at index
					RotatedRect r = fitEllipse(Mat(contours.at(idx)));
					currentObj.centerPoint = r.center;
					currentObj.id = count; // eigene id!!!
					count++;
					current.push_back(currentObj);
				}
			}

			if (trackingObjects.empty()){
				trackingObjects = current;
				for (int i = 0; i < trackingObjects.size(); i++) {
					//when trackingobjects is empty we fill it with all the objects in hierarchy and add corresponding cursors
					trackingObjects.at(i).tcur = new TuioCursor(trackingObjects.at(i).id, 0, norming(trackingObjects.at(i).centerPoint).x, norming(trackingObjects.at(i).centerPoint).y);
					server->addExternalTuioCursor(trackingObjects.at(i).tcur);
				}
				countGlobal = count;
			}

			else
			{
				for (int i = 0; i < current.size(); i++) {
					float min = NULL;
					int idx = NULL;
					for (int j = 0; j < trackingObjects.size(); j++){
						float dist = distance(current.at(i).centerPoint, trackingObjects.at(j).centerPoint);
							if (j == 0){
								min = dist;
								idx = j;
							}
							if (min > dist){
								min = dist;
								idx = j;
							}
					}
					if (min > maxSP) {
						SingleTrackingObject help;
						help.centerPoint = current.at(i).centerPoint;
						help.id = ++countGlobal;
						help.tcur = new TuioCursor(help.id, 0, norming(help.centerPoint).x, norming(help.centerPoint).y);
						//create a new external cursor locally
						trackingObjects.push_back(help);
						server->addExternalTuioCursor(help.tcur);
						//send the previously created local external cursor to the server
					} else {
						trackingObjects.at(idx).centerPoint = current.at(i).centerPoint;
						//trackingObjects.at(idx).tcur-> = currentFrame;
						server->updateTuioCursor(trackingObjects.at(idx).tcur,  norming(current.at(i).centerPoint).x, norming(current.at(i).centerPoint).y);
						//update the cursor, which fits our trackingobject-id, with the new coordinates
					}
				}

				for (int i = 0; i < trackingObjects.size(); i++){
					bool exists = false;
					for (int j = 0; j < current.size(); j++) {
						if (trackingObjects.at(i).centerPoint.x == current.at(j).centerPoint.x && trackingObjects.at(i).centerPoint.y == current.at(j).centerPoint.y){
							exists = true;
							break;
						}
					}

					if (!exists) {
						server->removeExternalTuioCursor(trackingObjects.at(i).tcur);
						//remove the cursor
						trackingObjects.erase(trackingObjects.begin() +i);
					}
				}
			}
		}

		//--------------------------
		
		
		
		//--------------------------

		if (cv::waitKey(1) == 27) // wait for user input
		{
			std::cout << "TERMINATION: User pressed ESC\n";
			break;
		}

		server->sendFullMessages();
		currentFrame++;
		
		// time end
		ms_end = clock();
		ms_time = ms_end - ms_start;
		
		putText(original, "frame #" + (std::string)_itoa(currentFrame, buffer, 10), cvPoint(0, 15), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8); // write framecounter to the image (useful for debugging)
		putText(original, "time per frame: " + (std::string)_itoa(ms_time, buffer, 10) + "ms", cvPoint(0, 30), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8); // write calculation time per frame to the image
		int count = 0;
		for (int i = 0; i < trackingObjects.size(); i++) {
			count++;
			putText(original, " ID " + std::to_string(trackingObjects.at(i).id), trackingObjects.at(i).centerPoint, cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8);
		}
		putText(original, " ID " + std::to_string(count), cvPoint(0, 45), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 255, 255), 1, 8);

	
		imshow("window", outputFrame); // render the frame to a window	
		imshow("window2", original);

		//Sleep(100);
	
	}
	std::cout << "SUCCESS: Program terminated like expected.\n";
	return 1;
}
