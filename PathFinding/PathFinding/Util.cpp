#include "util.h"
#include "PriorityQueue.cpp"
#include <glut.h>
#include <d3dx9math.h>

std::ifstream& operator>>(std::ifstream& in, D3DXVECTOR2& v)
{
	in>>v.x>>v.y;
	return in;
}
//SCENE DEFINITION
Scene::Scene() {}

Scene::Scene(std::string& fname)
{
	createSceneFromFile(fname);
}

Scene::~Scene() {}

void Scene::createSceneFromFile(std::string& fname)
{
	size_t nObjects = 0;
	char letter;
	Poly poly;
	std::ifstream inFile(fname, std::ios::in);
	inFile.seekg(0);
	//input start point
	inFile>>letter;
	if(letter=='A')
		inFile>>start;
	else exit(1);
	//input end point
	inFile>>letter;
	if(letter=='B')
		inFile>>end;
	else exit(1);
	//input number of objects in scene
	inFile>>nObjects;
	nObjects += 2; //2 objects more: start and end points!
	objects.resize(nObjects);
	//input poligons
	objects.at(0).pos = D3DXVECTOR2(0.0f, 0.0f);
	objects.at(0).graph.resize(1);
	objects.at(0).graph.at(0).resize(1);
	objects.at(0).graph.at(0).at(0) = 1;
	objects.at(0).verts.resize(1);
	objects.at(0).verts.at(0) = start;
	objects.at(0).bound.center = start;
	for(int i=1; i<nObjects-1; i++)
	{
		inFile>>objects.at(i);
	}
	objects.at(nObjects-1).pos = D3DXVECTOR2(0.0f, 0.0f);
	objects.at(nObjects-1).graph.resize(1);
	objects.at(nObjects-1).graph.at(0).resize(1);
	objects.at(nObjects-1).graph.at(0).at(0) = 1;
	objects.at(nObjects-1).verts.resize(1);
	objects.at(nObjects-1).verts.at(0) = end;
	objects.at(nObjects-1).bound.center = end;
	inFile.close();
}

//POLY DEFINITION
Poly::Poly() {}

Poly::Poly(const Poly& poly)
{
	pos = poly.pos;
	verts = poly.verts;
	graph = poly.graph;
}

Poly::~Poly() {}

void Poly::computeBounds()
{
	int nVerts = verts.size();
	D3DXVECTOR2 _center(0.0f, 0.0f);
	for(int i=0; i<nVerts; i++)
		_center += verts.at(i);
	_center /= nVerts;

	bound.center = _center;

	//Find maximum radius
	D3DXVECTOR2 diff = verts.at(0) - bound.center;
	float maxLength = D3DXVec2Length(&diff), currLength;	//1st length
	for(int i=1; i<nVerts; i++)
	{
		diff = verts.at(i) - bound.center;
		currLength = D3DXVec2Length(&diff);
		if( currLength > maxLength )
		{
			maxLength = currLength;
		}
	}
	bound.radius = maxLength;
}

std::ifstream& operator>>(std::ifstream& inFile, Poly& poly)
{
	poly.pos.x = poly.pos.y = 0.0f;
	int nVerts = 0;
	inFile>>nVerts;
	poly.verts.resize(nVerts);
	//Fill vertex data
	float x,y;
	for(int i=0; i<nVerts; i++)
	{
		inFile>>poly.verts.at(i);
	}
	//Fill graph data
	poly.graph.resize(nVerts);
	for(int i=0;i<nVerts;i++)
		poly.graph.at(i).resize(nVerts);
	poly.graph.at(0).at(nVerts-1) = 1;
	poly.graph.at(0).at(1) = 1;
	for(int i=1; i<nVerts-1; i++)
	{
		poly.graph.at(i).at(i-1) = 1;
		poly.graph.at(i).at(i+1) = 1;
	}
	poly.graph.at(nVerts-1).at(nVerts-2) = 1;
	poly.graph.at(nVerts-1).at(0) = 1;

	poly.computeBounds();
	return inFile;
}

bool Scene::isReachable(D3DXVECTOR2& from, D3DXVECTOR2& to)
{
	int nObjs = objects.size();
	int nVerts = 0;
	float distObjToLine = 0;
	bool flag = true;
	for(int i=0; i<nObjs; i++)
	{
		distObjToLine = getDistFromPointToLine(objects.at(i).bound.center, from, to);
		if( distObjToLine<objects.at(i).bound.radius )
		{
			nVerts = objects.at(i).verts.size();
			flag = !segmentsIntersect(from, to,
									  objects.at(i).verts.at(0),
									  objects.at(i).verts.at(nVerts-1));
			if( !flag ) break;
			for(int j=0; j<nVerts-1; j++)
			{
				flag = !segmentsIntersect(from, to,
									  objects.at(i).verts.at(j),
									  objects.at(i).verts.at(j+1));
				if( !flag ) break;
			}
		}
		if( !flag ) break;
	}
	return flag;
}

float getDistFromPointToLine(D3DXVECTOR2& from, D3DXVECTOR2& A, D3DXVECTOR2& B)
{
	D3DXVECTOR2 diff = A - B;
	float normLength = D3DXVec2Length(&diff);
	float dist = fabsf( (from.x-A.x)*(B.y-A.y)-(from.y-A.y)*(B.x-A.x) ) / normLength;
	return dist;
}

float getDistFromPointToPoint(D3DXVECTOR2& from, D3DXVECTOR2& to)
{
	D3DXVECTOR2 diff = to - from;
	return D3DXVec2Length(&diff);
}

bool segmentsIntersect(D3DXVECTOR2& p1, D3DXVECTOR2& p2, D3DXVECTOR2& p3, D3DXVECTOR2& p4)
{
	/*if( p1==p3 && p2==p4 ) return false;
	if( p1==p4 && p2==p3 )*/
	if( p1==p3 || p1==p4 || p2==p3 || p2==p4 ) return false; //not sure if it's correct, need to test!
	//TODO: Division by zero can occur!!!
	float nom = (p1.x-p2.x)*(p3.x-p4.x)*(p3.y-p1.y)+
				p1.x*(p1.y-p2.y)*(p3.x-p4.x)-
				p3.x*(p3.y-p4.y)*(p1.x-p2.x);
	float den = (p1.y-p2.y)*(p3.x-p4.x)-(p3.y-p4.y)*(p1.x-p2.x);
	if( den==0 )
	{
		return false;
		MessageBox(0, L"Division by zero occured!", L"Arithmetic error!", 0);
	}
	float Xintersect = nom / den;
	float t1 = (p1.x - Xintersect)/(p1.x-p2.x);
	float t2 = (p3.x - Xintersect)/(p3.x-p4.x);
	if( t1>=1.0f || t1<=0.0f || t2>=1.0f || t2<=0.0f) return false;
	else return true;
}

float Scene::findMinimalPathFromStartToEnd(std::list<D3DXVECTOR2>& output_path)
{
	Path child;
	Path parent;
	PriorityQueue<Path> pqueue;
	std::vector<std::vector<bool>> explored; //marking explored verteces
	bool belongToPQueue = false;
	bool isAccesible = false;
	int nObjs = objects.size();
	int nVerts = 0;
	explored.resize(nObjs+2);		// 2 verteces more: start and end
	for(int i=0; i<nObjs; i++)
	{
		nVerts = objects.at(i).verts.size();
		explored.at(i).resize(nVerts);
		for(int j=0; j<nVerts; j++)
			explored.at(i).at(j) = false;	//initially unexplored anything
	}
	explored.at(nObjs).resize(1);	//start point
	explored.at(nObjs).at(0) = false;
	explored.at(nObjs+1).resize(1);	//end point
	explored.at(nObjs+1).at(0) = false;

	pqueue.clear();				//pqueue is empty initially
	child.vert_object = 0;
	child.vert_id = 0;
	child.vert = start;
	child.length = 0;
	child.total_dist = child.length + getDistFromPointToPoint(start, end);
	child.full_path.push_back(start);
	pqueue.push(child, child.total_dist); //push initial state(vertex)
	explored.at(nObjs).at(0) = true;
	while( !pqueue.isEmpty() )
	{
		parent = pqueue.pop();
		if( parent.vert == end ) 
		{
			output_path = parent.full_path;
			printf("Min path length is %.1f\n", parent.length);
			return parent.length;			
		}
		for(int i=1; i<nObjs; i++)
		{
			nVerts = objects.at(i).verts.size();
			for(int j=0; j<nVerts; j++)
			{
				isAccesible = false;
				if( parent.vert_object == i )
				{
					if( objects.at(i).graph.at(parent.vert_id).at(j) == 1 )
						isAccesible = true;
				}
				else
				if( isReachable(parent.vert, objects.at(i).verts.at(j)) )	//expensive function
				{
					isAccesible = true;
				}
				if( isAccesible )
				{
					child.vert_object = i;
					child.vert_id = j;
					child.vert = objects.at(i).verts.at(j);
					child.length = parent.length + getDistFromPointToPoint(parent.vert, child.vert);
					child.total_dist = child.length + + getDistFromPointToPoint(child.vert, end);	// + heuristic function
					child.full_path = parent.full_path;
					child.full_path.push_back(objects.at(i).verts.at(j));
					belongToPQueue = pqueue.doesBelong(child);
					if( !explored.at(i).at(j) && !belongToPQueue)
					{
						pqueue.push(child, child.total_dist);
						explored.at(i).at(j) = true;
					}
					else
						if( belongToPQueue && objects.at(i).verts.at(j)!=parent.vert )
						{
							pqueue.replaceExisting(child, child.total_dist);
						}
				}
			}
		}
	}
}