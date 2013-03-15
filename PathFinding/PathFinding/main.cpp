#pragma comment(lib, "glut32.lib")

#include <stdio.h>
#include <Windows.h>
#include <crtdbg.h>
#include <vector>
#include <list>
#include <iostream>

#include "util.h"
#include <glut.h>
#include <d3dx9math.h>
#include <cstdlib>
#include <time.h>

#ifdef _DEBUG
#define DB( x ) x
#else
#define DB( x ) {}
#endif

#define SCALE 1000.0f

struct RenderCall
{
	float posX;
	float posY;
	void (*f)(float , float);
};

//Global variables
std::list<RenderCall> renderList;
std::vector<D3DXVECTOR2> poly;
std::list<D3DXVECTOR2> minimalPath;
Scene gScene;
float secPerCnt;

//Standard callbacks
static void RenderCallback();
static void ReshapeCallback(int width, int height);
static void IdleCallback();
static void MouseCallback(int button, int state, int x, int y);
static void KeyboardCallback(unsigned char key, int x, int y);
//Init functions
void initScene();
void initPoly();

//Render functions
void RenderScene();
void RenderSquare(float posx, float posy);

int main(int argc, char *argv[])
{
	DB(	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF ) );
	
	srand( time(0) );

	__int64 cntsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cntsPerSec);
	secPerCnt = 1.0f / (float)cntsPerSec;

	glutInit(&argc, argv);
	glutInitWindowSize(512,512);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	int mainHandle = glutCreateWindow("PathFinder");
	glutSetWindow(mainHandle);
	glutDisplayFunc(RenderCallback);
	glutReshapeFunc(ReshapeCallback);
	glutIdleFunc(IdleCallback);
	glutMouseFunc(MouseCallback);
	glutKeyboardFunc(KeyboardCallback);

	glClearColor(0.3f, 0.4f, 0.5f, 1.0f);

	initScene();
	
	glutMainLoop();

	DB(system("Pause"));
	return 0;
}

static void RenderCallback()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	//Render code
	RenderScene();

	glFlush();

	//Swap buffers after each iteration
	glutSwapBuffers();
}

static void ReshapeCallback(int width, int height)
{
	glViewport(0, 0, width, height);
}

static void IdleCallback()
{
	glutPostRedisplay();
}

static void KeyboardCallback(unsigned char key, int x, int y)
{
	bool _boolean = false;
	float minPath = 0.0f;
	switch(key)
	{
	case 's':
	case 'S':
		minPath = gScene.findMinimalPathFromStartToEnd(minimalPath);
		break;
	default:
		break;
	}
}

static void MouseCallback(int button, int state, int x, int y)
{
	static __int64 t1 = 0, t2 = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	float elapsedTime = (t1-t2)*secPerCnt;
	D3DXVECTOR2 v;
	D3DXMATRIX viewMat, projMat, cameraMat, viewcamMat;
	D3DXMatrixIdentity(&cameraMat);
	if( elapsedTime > 0.2f )
	{
		switch( button )
		{
			case 0:
				printf("Mouse has been pressed!\n");
				GLint viewport[4];
				glGetIntegerv( GL_VIEWPORT, viewport );

				v.x = (2.0f*x / viewport[2] - 1.0f) ;
				v.y = -(2.0f*y / viewport[3] - 1.0f) ;

				poly.push_back(v);
				break;
			case 2:
				poly.pop_back();
				break;
			default:
				break;
		}
	}
	t2 = t1;
}

void initScene()
{
	//Here initialize all scene objects
	gScene.createSceneFromFile(std::string("scene.txt"));
}

void RenderScene()
{
	glColor3f(1.0f, 1.0f, 1.0f);
	int nObjs = gScene.objects.size();
	for(int i=0; i<nObjs; i++)
	{
		glBegin(GL_LINE_LOOP);	//Begin drawing
		for(int j=0; j<gScene.objects.at(i).verts.size(); j++)
			glVertex2f(gScene.objects.at(i).verts.at(j).x/SCALE, 
					   gScene.objects.at(i).verts.at(j).y/SCALE);
		glEnd();
	}
	glColor3f(0.0f, 1.0f, 0.0f);
	RenderSquare(gScene.start.x, gScene.start.y);	//start point
	RenderSquare(gScene.end.x, gScene.end.y);		//end point
	if(!minimalPath.empty())
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		std::list<D3DXVECTOR2>::iterator i = minimalPath.begin(), end = minimalPath.end();
		glBegin(GL_LINE_STRIP);
		while( i!=end )
		{
			glVertex2f(i->x / SCALE, i->y / SCALE);
			i++;
		}
		glEnd();
	}
}

void RenderSquare(float posx, float posy)
{
	glBegin(GL_LINE_LOOP);	//Begin drawing
	glVertex2f(posx/SCALE,			posy/SCALE);
	glVertex2f((posx+10.0f)/SCALE,	posy/SCALE);
	glVertex2f((posx+10.0f)/SCALE,	(posy+10.0f)/SCALE);
	glVertex2f(posx/SCALE,			(posy+10.0f)/SCALE);
	glEnd();
}