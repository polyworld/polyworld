
// Self
#include "Camera.h"

// System
#include <fstream>
#include <iostream>
#include <math.h>

// qt
#include <qapplication.h>

using namespace std;


TCamera::TCamera()
{
	// Initial eye position
	eyeX = 0.0;
	eyeY = 0.0;
	eyeZ = -20.0;

	// Initial scene center
	centerX = 0.0;
	centerY = 0.0;
	centerZ = 0.0;

	// Initial up normal vector
	upX = 0.0;
	upY = 1.0;
	upZ = 0.0;

	rotationEyeX = 0.0;
	rotationEyeY = 0.0;

	cameraNumber = 0;

	// Make sure the vectors are up to date
	calculateNormal();	
}


TCamera::~TCamera()
{
}


void TCamera::save()
{
	// Write the camera settings to a camera#.dat file
    fstream outFile("camera.dat", ios::out);
    
	if (!outFile)
		return;

	outFile << enabled;

	// Projection information
	outFile << nearl << endl;
	outFile << farl << endl;
	outFile << left << endl;
	outFile << right << endl;
	outFile << top << endl;
	outFile << bottom << endl;
	outFile << aspect << endl;
	outFile << fov << endl;

	// Cumulative eye rotation
	outFile << rotationEyeX << endl;
	outFile << rotationEyeY << endl;
	
	// Eye position
	outFile << eyeX << endl;
	outFile << eyeY << endl;
	outFile << eyeZ << endl;

	// Scene center
	outFile << centerX << endl;
	outFile << centerY << endl;
	outFile << centerZ << endl;			

	// Up orientation vector
	outFile << upX << endl;
	outFile << upY << endl;
	outFile << upZ << endl;

	outFile.close();
}

void TCamera::restore()
{
	// Write the camera settings to a camera#.dat file
    fstream inFile("camera.dat", ios::in);
    
	if (!inFile)
		return;

	inFile >> enabled;

	// Projection information
	inFile >> nearl;
	inFile >> farl;
	inFile >> left;
	inFile >> right;
	inFile >> top;
	inFile >> bottom;
	inFile >> aspect;
	inFile >> fov;

	// Cumulative eye rotation
	inFile >> rotationEyeX;
	inFile >> rotationEyeY;
	
	// Eye position
	inFile >> eyeX;
	inFile >> eyeY;
	inFile >> eyeZ;

	// Scene center
	inFile >> centerX;
	inFile >> centerY;
	inFile >> centerZ;			

	// Up orientation vector
	inFile >> upX;
	inFile >> upY;
	inFile >> upZ;

	inFile.close();

	calculateNormal();
}

void TCamera::calculateNormal()
{
	// Determine viewing vector
	viewX = centerX - eyeX;
	viewY = centerY - eyeY;
	viewZ = centerZ - eyeZ;

	// Calculate the cross product of the view and up vectors
	normalX = (viewY * upZ) - (upY * viewZ);
	normalY = (viewZ * upX) - (upZ * viewX);
	normalZ = (viewX * upY) - (upX * viewY);

	// Set to be a unit normal
	double magnitude = sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
	normalX /= magnitude;
	normalY /= magnitude;
	normalZ /= magnitude;
}

// Left and right are along the normal vector to the camera - normals
// only change under a rotation.
void TCamera::moveLeft(double dist)
{
	eyeX = eyeX + (normalX * -dist);
	eyeY = eyeY + (normalY * -dist);
	eyeZ = eyeZ + (normalZ * -dist);
	
	centerX = centerX + (normalX * -dist);
	centerY = centerY + (normalY * -dist);
	centerZ = centerZ + (normalZ * -dist);
}

void TCamera::moveRight(double dist)
{
	eyeX = eyeX + (normalX * dist);
	eyeY = eyeY + (normalY * dist);
	eyeZ = eyeZ + (normalZ * dist);
	
	centerX = centerX + (normalX * dist);
	centerY = centerY + (normalY * dist);
	centerZ = centerZ + (normalZ * dist);
}

// Up and down are along the camera's up and down vector
void TCamera::moveUp(double dist)
{
	eyeX = eyeX + (upX * dist);
	eyeY = eyeY + (upY * dist);
	eyeZ = eyeZ + (upZ * dist);
	
	centerX = centerX + (upX * dist);
	centerY = centerY + (upY * dist);
	centerZ = centerZ + (upZ * dist);
}

void TCamera::moveDown(double dist)
{
	eyeX = eyeX + (upX * -dist);
	eyeY = eyeY + (upY * -dist);
	eyeZ = eyeZ + (upZ * -dist);
	
	centerX = centerX + (upX * -dist);
	centerY = centerY + (upY * -dist);
	centerZ = centerZ + (upZ * -dist);
}

// Forward and back are along the viewing vector
void TCamera::moveForward(double dist)
{
	eyeX = eyeX + (viewX * -dist);
	eyeY = eyeY + (viewY * -dist);
	eyeZ = eyeZ + (viewZ * -dist);
	
	centerX = centerX + (viewX * -dist);
	centerY = centerY + (viewY * -dist);
	centerZ = centerZ + (viewZ * -dist);
}

void TCamera::moveBackward(double dist)
{
	eyeX = eyeX + (viewX * dist);
	eyeY = eyeY + (viewY * dist);
	eyeZ = eyeZ + (viewZ * dist);
	
	centerX = centerX + (viewX * dist);
	centerY = centerY + (viewY * dist);
	centerZ = centerZ + (viewZ * dist);
}

// Rotate the camera about an arbitary axis (for positioning)
// pitch, yaw, roll
void TCamera::rotateAxis(double degrees, double x, double y, double z)
{
	glRotated(degrees, x, y, z);
}

void TCamera::moveEye(double dx, double dy, double dz)
{
	eyeX += dx;
	eyeY += dy;
	eyeZ += dz;
}

void TCamera::moveCenter(double dx, double dy, double dz)
{
	centerX += dx;
	centerY += dy;
	centerZ += dz;
}

// Rotates the scene about the camera center - changes the eye
void TCamera::rotatePoint(double dx, double dy)
{
	// Calculate the vector from the current view to the point
	setCenter(0,0,0);
	calculateNormal();

	double rotate[16];
	double newUpX, newUpY, newUpZ;
	double newSceneX, newSceneY, newSceneZ;
    double newSceneX1, newSceneY1, newSceneZ1;

	// Allow limiting of rotation (see main)
	rotationEyeX += dx;
	rotationEyeY += dy;
		
	// Rotate about the Up/Down Plane
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

    glLoadIdentity();
	glRotated(dy, normalX, normalY, normalZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, rotate);


	// We now need to calculate a new up direction
	newUpX = (rotate[0]*upX + rotate[1]*upY + rotate[2]*upZ  + rotate[3]);
	newUpY = (rotate[4]*upX + rotate[5]*upY + rotate[6]*upZ  + rotate[7]);
	newUpZ = (rotate[8]*upX + rotate[9]*upY + rotate[10]*upZ + rotate[11]);

	upX = newUpX;
	upY = newUpY;
	upZ = newUpZ;

	newSceneX = (rotate[0]*-viewX + rotate[1]*-viewY + rotate[2]*-viewZ  + rotate[3]);
	newSceneY = (rotate[4]*-viewX + rotate[5]*-viewY + rotate[6]*-viewZ  + rotate[7]);
	newSceneZ = (rotate[8]*-viewX + rotate[9]*-viewY + rotate[10]*-viewZ + rotate[11]);

    glLoadIdentity();
	glRotated(dx, upX, upY, upZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, rotate);
	glPopMatrix();

 	newSceneX1 = (rotate[0]*newSceneX + rotate[1]*newSceneY + rotate[2]*newSceneZ  + rotate[3]);
	newSceneY1 = (rotate[4]*newSceneX + rotate[5]*newSceneY + rotate[6]*newSceneZ  + rotate[7]);
    newSceneZ1 = (rotate[8]*newSceneX + rotate[9]*newSceneY + rotate[10]*newSceneZ + rotate[11]);
		
	eyeX = newSceneX1;
	eyeY = newSceneY1;
	eyeZ = newSceneZ1;

	// Make sure the vectors are up to date
	calculateNormal();
}

// Rotates about the eye - changes the scene center
void TCamera::rotateEye(double dx, double dy)
{
	double rotate[16];
	double newUpX, newUpY, newUpZ;
	double newSceneX, newSceneY, newSceneZ;
    double newSceneX1, newSceneY1, newSceneZ1;

	// Allow limiting of rotation (see main)
	rotationEyeX += dx;
	rotationEyeY += dy;

	
	// Rotate about the Up/Down Plane
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

    glLoadIdentity();
	glRotated(dy, normalX, normalY, normalZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, rotate);


	// We now need to calculate a new up direction
	newUpX = (rotate[0]*upX + rotate[1]*upY + rotate[2]*upZ  + rotate[3]);
	newUpY = (rotate[4]*upX + rotate[5]*upY + rotate[6]*upZ  + rotate[7]);
	newUpZ = (rotate[8]*upX + rotate[9]*upY + rotate[10]*upZ + rotate[11]);

	upX = newUpX;
	upY = newUpY;
	upZ = newUpZ;

	newSceneX = (rotate[0]*viewX + rotate[1]*viewY + rotate[2]*viewZ  + rotate[3]);
	newSceneY = (rotate[4]*viewX + rotate[5]*viewY + rotate[6]*viewZ  + rotate[7]);
	newSceneZ = (rotate[8]*viewX + rotate[9]*viewY + rotate[10]*viewZ + rotate[11]);

    glLoadIdentity();
	glRotated(dx, upX, upY, upZ);
	glGetDoublev(GL_MODELVIEW_MATRIX, rotate);
	glPopMatrix();

 	newSceneX1 = (rotate[0]*newSceneX + rotate[1]*newSceneY + rotate[2]*newSceneZ  + rotate[3]);
	newSceneY1 = (rotate[4]*newSceneX + rotate[5]*newSceneY + rotate[6]*newSceneZ  + rotate[7]);
    newSceneZ1 = (rotate[8]*newSceneX + rotate[9]*newSceneY + rotate[10]*newSceneZ + rotate[11]);
		
	centerX = eyeX + newSceneX1;
	centerY = eyeY + newSceneY1;
	centerZ = eyeZ + newSceneZ1;

	// Make sure the vectors are up to date
	calculateNormal();
}

// Enable this camera view
void TCamera::on()
{
	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 1.0, 1.0, 200.0);

	lookAtScene();   
}

// Force the camera to look at a scene
void TCamera::lookAtScene()
{
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
}

// Turns the camera off and resets any effects on opengl
void TCamera::off()
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

void TCamera::clear(float r, float g, float b)
{
	glClearColor(r,g,b,0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void TCamera::clearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void TCamera::clearAll(float r, float g, float b)
{
	glClearColor(r, g, b, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TCamera::setProjection(double l, double r, double t, double b, double n, double f)
{
	left = l;
	right = r;
	top = t;
	bottom = b;
	nearl = n;
	farl = f;
}

void TCamera::setProjection(double l, double r, double t, double b)
{
	left = l;
	right = r;
	top = t;
	bottom = b;
}

void TCamera::setCameraNumber(int val)
{
	cameraNumber = val;
}

void TCamera::setProjection(double f, double asp, double n)
{
	fov = f;
	aspect = asp;
	nearl = n;
	
}

void TCamera::setEye(double eX, double eY, double eZ)
{
	eyeX = eX;
	eyeY = eY;
	eyeZ = eZ;
}

void TCamera::setUp(double uX, double uY, double uZ)
{
	centerX = uX;
	centerY = uY;
	centerZ = uZ;
}

void TCamera::setCenter(double cX, double cY, double cZ)
{
	centerX = cX;
	centerY = cY;
	centerZ = cZ;
}

void TCamera::getEye(double &eX, double &eY, double &eZ)
{
	eX = eyeX;
	eY = eyeY;
	eZ = eyeZ;
}

void TCamera::getUp(double &uX, double &uY, double &uZ)
{
	uX = upX;
	uY = upY;
	uZ = upZ;
}

void TCamera::getCenter(double &cX, double &cY, double &cZ)
{
	cX = centerX;
	cY = centerY;
	cZ = centerZ;	
}
