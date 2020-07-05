#include <iostream>
#include <process.h>

#include "TuioClient.h"
#include "TuioListener.h"
#include <vector>
#include <list>
#include "GeometricRecognizer.h"

#include <GL/glut.h>


using namespace TUIO;
using namespace std;
using namespace DollarRecognizer;


TUIO::TuioClient *tuioClient; // global tuioClient for testing
std::vector<TuioCursor> cursors;
struct point2f{ float x; float y; };
vector<point2f> quadCoords;
GeometricRecognizer* greg = new GeometricRecognizer();


double videoWidth = GLUT_WINDOW_WIDTH; // get the frame size of the videocapture object
double videoHeight = GLUT_WINDOW_HEIGHT; // get the frame size of the videocapture object


float toCoordX(float norm){
	float result;
	result = 1 - norm;
	result = norm - result;
	return result;
}

float toCoordY(float norm){
	float result;
	result = 1 - norm;
	result = norm - result;
	result = result * (-1);
	return result;
}

/*int GestureRecognizer(TuioCursor *tcur, int gestureID){
	return 0;
};*/


class Quad {

	float transX;
	float transY;
	float scaling;
	point2f centerpoint;
	float colors[3];
	int mode;	//mode 0 = Default, mode 1 = Drag, mode 2 = Scale
	struct fingerStruct{ float x; float y; int id; };
	fingerStruct fingers[2];
	float dist;

public:

	Quad(point2f centerpoint) {
		this->centerpoint = centerpoint;
		this->transX = centerpoint.x;
		this->transY = centerpoint.y;
		this->scaling = 1.75;
		colors[0, 1, 2] = 0.0;
		mode = 0;
		fingers[0].id = NULL;
		fingers[1].id = NULL;
	};

	void Quad::draw() {
		glPushMatrix();

		glColor4f(colors[0], colors[1], colors[2], 1.0);
		glTranslatef( transX , transY , 0.0);
		//glRotatef();
		glScalef(scaling , scaling , scaling );
		glBegin(GL_QUADS);
		glVertex2f( - 0.1,  + 0.1);
		glVertex2f( + 0.1,  + 0.1);
		glVertex2f( + 0.1,  - 0.1);
		glVertex2f( - 0.1,  - 0.1);
		glEnd();

		glColor4f(255.0 , 0.0 , 0.0 , 1.0);
		glLineWidth(2.0);
		glBegin(GL_LINE_LOOP);
		glVertex2f(-0.1, +0.1);
		glVertex2f(+0.1, +0.1);
		glVertex2f(+0.1, -0.1);
		glVertex2f(-0.1, -0.1);
		glEnd();

		glPopMatrix();
	};

	void setTrans(float x , float y) {
		transX += x - centerpoint.x;
		transY += y - centerpoint.y;
		centerpoint.x = x;
		centerpoint.y = y;
	};

	void rndColor() {
		colors[0] = (((float)rand()) / 255.0) / 255.0;
		colors[1] = (((float)rand()) / 255.0) / 255.0;
		colors[2] = (((float)rand()) / 255.0) / 255.0;
	};

	bool inShape(point2f cursorpoint) {
		if (((centerpoint.x + (0.1 * scaling)) >= cursorpoint.x) && ((centerpoint.x - (0.1 * scaling)) <= cursorpoint.x )) {
			if (((centerpoint.y + (0.1 * scaling)) >= cursorpoint.y) && ((centerpoint.y - (0.1 * scaling)) <= cursorpoint.y)) {
				return true;
			}
		}
		return false;
	};

	void großklein(TuioCursor* x) {
		point2f vec;
		vec.x = toCoordX(fingers[1].x) - toCoordX(fingers[0].x);
		vec.y = toCoordY(fingers[1].y) - toCoordY(fingers[0].y);
		float distold = sqrt((vec.x*vec.x) + (vec.y*vec.y));
		float distnew = 0.0;
		for (int i = 0; i < 2; i++) {
			if (fingers[i].id == x->getSessionID()){
				point2f vec2;
				switch (i) {
				case 0:
					vec2.x = toCoordX(fingers[1].x) - toCoordX(x->getX());
					vec2.y = toCoordY(fingers[1].y) - toCoordY(x->getY());
					distnew = sqrt((vec2.x*vec2.x) + (vec2.y*vec2.y));
					if (fingers[0].x != x->getX() && fingers[0].y != x->getY()){
						setFingers(0, x);
					}
					break;
				case 1: 
					vec2.x = toCoordX(fingers[0].x) - toCoordX(x->getX());
					vec2.y = toCoordY(fingers[0].y) - toCoordY(x->getY());
					distnew = sqrt((vec2.x*vec2.x) + (vec2.y*vec2.y));
					if (fingers[1].x != x->getX() && fingers[1].y != x->getY()){
						setFingers(1, x);
					}
					break;
				default :
					std::cout << "UNGUELTUIG \n" ;
					break;
				}
				scaling += (distnew - distold) * 2;
				if (scaling < 0.7){
					scaling = 0.8;
				}
				else if (scaling > 3.1) {
					scaling = 3.0;
				}
			}
		}
	};

	void setMode(int i) {
		mode = i;
	};

	int getMode() {
		return mode;
	}

	void setFingers(int index, TuioCursor* tcur){
		if (!tcur == NULL) {
			fingers[index].id = tcur->getSessionID();
			fingers[index].x = tcur->getX();
			fingers[index].y = tcur->getY();
		}
		else {
			fingers[index].id = NULL;
			fingers[index].x = NULL;
			fingers[index].y = NULL;
		}
	}

	fingerStruct getFingers(int index) {
		return fingers[index];
	}
};

vector<Quad> quads;

class Client : public TuioListener {
	// these methods need to be implemented here since they're virtual methods
	// these methods will be called whenever a new package is received
	//std::vector<TuioCursor> cursors;

	void Client::addTuioObject(TuioObject *tobj){};
	void Client::updateTuioObject(TuioObject *tobj){};
	void Client::removeTuioObject(TuioObject *tobj){};

	void Client::addTuioCursor(TuioCursor *tcur)
	{
		cursors.push_back(tcur);
		point2f tempPoint = { toCoordX(tcur->getX()), toCoordY(tcur->getY()) };
		for (int i = 0; i < quads.size(); i++) {
			if (quads.at(i).inShape(tempPoint)) {
				if (quads.at(i).getFingers(0).id == NULL){
					quads.at(i).setFingers(0, tcur);
					quads.at(i).setMode(1);
					//std::cout << "MODE 1" << "\n";
				}
				else if (quads.at(i).getFingers(1).id == NULL){
					quads.at(i).setFingers(1, tcur);
					quads.at(i).setMode(2);
					//std::cout << "MODE 2" << "\n";
				}
			}
		}
		std::cout << "finger added: (id=" << tcur->getSessionID() << ", coordinates=" << tcur->getX() << "," << tcur->getY() << ")\n";
	};

	void Client::updateTuioCursor(TuioCursor *tcur){

		point2f tempPointNew = { toCoordX(tcur->getX()), toCoordY(tcur->getY()) };
		for(int i = 0; i < cursors.size(); i++) {
			if (cursors.at(i).getSessionID() == tcur->getSessionID()) {
				point2f tempPointOld = { toCoordX(cursors.at(i).getX()), toCoordY(cursors.at(i).getY()) };
				for (int j = 0; j < quads.size(); j++) {
					if (quads.at(j).inShape(tempPointOld) && (quads.at(j).getMode() == 0 || quads.at(j).getMode() == 1)) {
						quads.at(j).setFingers(0, tcur);
						quads.at(j).setTrans(tempPointNew.x , tempPointNew.y);
						quads.at(j).setMode(1);
					}
					else if (quads.at(j).inShape(tempPointOld) && quads.at(j).getMode() == 2) {
						quads.at(j).großklein(tcur);
					}
				}
				cursors.at(i) = tcur;
				long x = tcur->getStartTime().getTotalMilliseconds();
				long y = tcur->getTuioTime().getTotalMilliseconds();
				//std::cout << "finger updated: (id=" << tcur->getSessionID() << ", coordinates=" << tcur->getX() << ", " << tcur->getY() << ")\n";
			}
		}
	};

	void Client::removeTuioCursor(TuioCursor *tcur){

		Path2D points;
		Point2D p;
		list<TuioPoint> path = tcur->getPath();
		for (list<TuioPoint>::iterator it = path.begin(); it != path.end(); it++) {
			p.x = it->getX();
			p.y = it->getY();
			points.push_back(p);
		}
		
		long diff = 0;
		diff = tcur->getTuioTime().getTotalMilliseconds() - tcur->getStartTime().getTotalMilliseconds();
		point2f tempPoint = { toCoordX(tcur->getX()), toCoordY(tcur->getY()) };

		for (int i = 0; i < cursors.size(); i++) {
			if (cursors.at(i).getSessionID() == tcur->getSessionID()) {

				for (int j = 0; j < quads.size(); j++) {
						for (int f = 0; f < 2; f++) {
							if (quads.at(j).getFingers(f).id == NULL) {}
							else if (quads.at(j).getFingers(f).id == tcur->getSessionID()) {
								quads.at(j).setFingers(f, NULL);
								quads.at(j).setMode(0);
							}
					}
				}
				cursors.erase(cursors.begin()+i);
				std::cout << "finger removed: (id=" << tcur->getSessionID() << ", coordinates=" << tcur->getX() << "," << tcur->getY() << ")\n";
			}
		}
		if (diff <= 125 && tcur->getPath().size() <= 3){
			bool newQuad = true;
			for (int j = quads.size() - 1; j >= 0; j--) {
				if (quads.at(j).inShape(tempPoint) && newQuad) {
					quads.at(j).rndColor();
					newQuad = false;
				}
			}
		}
		else if (greg->recognize(points).name == "Rectangle" && greg->recognize(points).score >= 0.75) {
			Quad tempQuad(tempPoint);
			quads.push_back(tempQuad);
		}
		cout << "\n---------------Gestenerkennung---------------\n";
		cout << "Name : " << greg->recognize(points).name << "\n";
		cout << "Score: " << greg->recognize(points).score << "\n\n";
	};

	void  Client::refresh(TuioTime frameTime){};
};

void draw()
{
	glClearColor(255.0 , 255.0 , 255.0 , 100.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < quads.size(); i++){
		quads.at(i).draw();
	}

	glutSwapBuffers();

}

void tuioThread(void*)
{
	Client *app = new Client();
	tuioClient = new TUIO::TuioClient();
	tuioClient->addTuioListener(app);
	tuioClient->connect(true);
}

void animate(int value){
	
}

void idle(void)
{
	// this might be needed on some systems, otherwise the draw() function will only be called once
	glutPostRedisplay();
}

void glInit()
{	
		
}

int main(int argc, char** argv)
{
	// create a second thread for the TUIO listener
	HANDLE hThread_TUIO;
	unsigned threadID;
	//hThread = (HANDLE)_beginthreadex( NULL, 0, &tuioThread, NULL, 0, &threadID );
	hThread_TUIO = (HANDLE)_beginthread( tuioThread, 0, NULL );

	// GLUT Window Initialization (just an example):
	glutInit(&argc, argv);
	glutInitWindowSize(752, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("TUIO Client Example (GLUT Window)");

	// openGL init
	glInit();

	// Register callbacks:
	glutDisplayFunc(draw);
	glutIdleFunc(idle);
	glutTimerFunc(10, animate, 0);
	glutMainLoop();

	return 0;
}