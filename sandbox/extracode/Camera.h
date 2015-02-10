#ifndef CAMERA
#define CAMERA

// System
#include <gl.h>
#include <glu.h>
#include <math.h>

#define ORTHOGRAPHIC = 10000;
#define PROJECTION   = 10001;

class TCamera
{
public:	
	TCamera();
	~TCamera();
	
	// Actions
	void on();
	void off();
	void lookAtScene();
	void clear(float r=0, float g=0, float b=0);
	void clearDepth();
	void clearAll(float r=0, float g=0, float b=0);
	
	// Save/restore settings
	void save();
	void restore();
	
	// Motion
	void moveLeft(double dist);
	void moveRight(double dist);
	void moveUp(double dist);
	void moveDown(double dist);
	void moveForward(double dist);
	void moveBackward(double dist);
	
	void moveEye(double dx, double dy, double dz);
	void moveCenter(double dx, double dy, double dz);
	
	void rotateEye(double dx, double dy);
	void rotatePoint(double dx, double dy);
	void rotateAxis(double degrees, double x, double y, double z);	
	
	// Modifiers
	void setProjection(double l, double r, double t, double b, double n, double f);
	void setProjection(double l, double r, double t, double b);
	void setProjection(double f, double asp, double n);
	void setEye(double eX, double eY, double eZ);
	void setUp(double uX, double uY, double uZ);
	void setCenter(double cX, double cY, double cZ);
	void setCameraNumber(int val);
	
	//void getProjection(double &l, double &r, double &t, double &b, double &n, double &f);
	//void getProjection(double &l, double &r, double &t, double &b);
	//void getProjection(double &f, double &asp, double &n, double &fr, int &i);
	void getEye(double &eX, double &eY, double &eZ);
	void getUp(double &uX, double &uY, double &uZ);
	void getCenter(double &cX, double &cY, double &cZ);
	
	void calculateNormal();

private:
	// What is the current camera number?
	int cameraNumber;
	
	// Is camera active?
	int enabled;
	
	// Projection information
	double nearl;
	double farl;
	double left;
	double right;
	double top;
	double bottom;
	double aspect;
	double fov;
	
	// Normal to the eye position
	double normalX;
	double normalY;
	double normalZ;
	
	// View to scene center vector
	double viewX;
	double viewY;
	double viewZ;
	
	// Cumulative eye rotation
	double rotationEyeX;
	double rotationEyeY;
	
	// Eye position
	double eyeX;
	double eyeY;
	double eyeZ;
	
	// Scene center
	double centerX;
	double centerY;
	double centerZ;			
	
	// Up orientation vector
	double upX;
	double upY;
	double upZ;
};
#endif