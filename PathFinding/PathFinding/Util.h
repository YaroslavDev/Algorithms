#ifndef UTIL_H
#define UTIL_H


#include <Windows.h>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <d3dx9math.h>

//==================================================================
//DATA STRUCTURE DEFINITIONS
//==================================================================
class Poly;
struct BoundCircle
{
	BoundCircle() : center(D3DXVECTOR2(0.0f, 0.0f)), radius(0.0f) {}
	D3DXVECTOR2 center;
	float radius;
};
struct Path
{
	Path() : length(0), total_dist(0), vert_object(-1), vert_id(-1) {}
	Path(D3DXVECTOR2 _vert, int _length = 0, int _dist = 0) 
		: vert(_vert), length(_length), total_dist(_dist), vert_object(-1), vert_id(-1) {}	//-1 neutral value for index
	bool operator==(const Path& lhs)
	{
		return vert==lhs.vert;
	}
	Path& operator=(const Path& lhv)
	{
		vert = lhv.vert;
		length = lhv.length;
		total_dist = lhv.total_dist;
		vert_object = lhv.vert_object;
		vert_id = lhv.vert_id;
		full_path = lhv.full_path;
		return *this;
	}
	int vert_object;
	int vert_id;
	std::list<D3DXVECTOR2> full_path;
	D3DXVECTOR2 vert;
	float length;
	float total_dist;
};

//==================================================================
//FUNCTION DEFINITIONS
//==================================================================
std::ifstream& operator>>(std::ifstream& in, D3DXVECTOR2&);
float getDistFromPointToLine(D3DXVECTOR2& from, D3DXVECTOR2& A, D3DXVECTOR2& B);
bool segmentsIntersect(D3DXVECTOR2& p1, D3DXVECTOR2& p2, D3DXVECTOR2& p3, D3DXVECTOR2& p4);
float getDistFromPointToPoint(D3DXVECTOR2& from, D3DXVECTOR2& to);
//==================================================================
//CLASS DEFINITIONS
//==================================================================
class Scene
{
public:
	Scene();
	Scene(std::string& fname);
	~Scene();
	void createSceneFromFile(std::string& fname);
	bool isReachable(D3DXVECTOR2& from, D3DXVECTOR2& to);
	float findMinimalPathFromStartToEnd(std::list<D3DXVECTOR2>& path);
public:
	D3DXVECTOR2 start;
	D3DXVECTOR2 end;
	std::vector<Poly> objects;
};

class Poly
{
	friend std::ifstream& operator>>(std::ifstream& in, Poly& poly);
public:
	Poly();
	Poly(const Poly&);
	~Poly();
	void computeBounds();
public:
	D3DXVECTOR2 pos;
	std::vector<D3DXVECTOR2> verts;
	std::vector<std::vector<int>> graph;
	BoundCircle bound;
};

#endif //UTIL_H