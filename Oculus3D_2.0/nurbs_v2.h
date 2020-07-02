#pragma once
#include <fstream>
#include <thread>
#include <mutex>



template <typename T>
std::vector<T> operator+(const std::vector<T>& A, const std::vector<T>& B)
{
	std::vector<T> AB;
	AB.reserve(A.size() + B.size());
	AB.insert(AB.end(), A.begin(), A.end());
	AB.insert(AB.end(), B.begin(), B.end());
	return AB;
}



struct vec3d
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1;

	void move(float mx, float my, float mz)
	{
		x += mx;
		y += my;
		z += mz;
	}
};

struct line
{
	vec3d p[2];
};

struct triangle
{
	vec3d p[3];
	float dp;
};

struct mesh
{
	std::vector<triangle> tris;

};

class box
{
public:
	vec3d points[8];
	std::vector<line> lines;

	box()
	{
		points[0] = { 0,0,0,1 };
		points[1] = { 1, 0, 0,1 };
		points[2] = { 1, 0, 1,1 };
		points[3] = { 0, 0, 1,1 };
		points[4] = { 0, 1, 0,1 };
		points[5] = { 1, 1, 0,1 };
		points[6] = { 1, 1, 1,1 };
		points[7] = { 0, 1, 1,1 };

		lines.push_back({ points[0], points[1] });
		lines.push_back({ points[0], points[4] });
		lines.push_back({ points[1], points[5] });
		lines.push_back({ points[1], points[2] });

		lines.push_back({ points[2], points[6] });
		lines.push_back({ points[2], points[3] });
		lines.push_back({ points[3], points[7] });
		lines.push_back({ points[3], points[0] });

		lines.push_back({ points[4], points[5] });
		lines.push_back({ points[5], points[6] });
		lines.push_back({ points[6], points[7] });
		lines.push_back({ points[7], points[4] });

		moveMiddle(-0.5);
		scale(0.1);
	}

	void scale(float scale)
	{
		for (auto& i : lines)
		{
			i.p[0].x *= scale;
			i.p[1].x *= scale;
			i.p[0].y *= scale;
			i.p[1].y *= scale;
			i.p[0].z *= scale;
			i.p[1].z *= scale;
		}
	}

	void moveMiddle(float offset)
	{
		for (auto& i : lines)
		{
			i.p[0].x += offset;
			i.p[1].x += offset;
			i.p[0].y += offset;
			i.p[1].y += offset;
			i.p[0].z += offset;
			i.p[1].z += offset;
		}
	}

	void setPosition(vec3d position)
	{
		for (auto& i : lines)
		{
			i.p[0].x += position.x;
			i.p[1].x += position.x;
			i.p[0].y += position.y;
			i.p[1].y += position.y;
			i.p[0].z += position.z;
			i.p[1].z += position.z;
		}
	}


};

std::vector<vec3d> bezier(int s, vec3d a, vec3d b, vec3d c, vec3d d)
{
	std::vector<line> lines;
	std::vector<vec3d> points;
	float t = 0;
	float xtem, ytem, ztem;
	float step = 1.0f / s;
	line teml;
	points.push_back(a);
	while (t < 1)
	{
		t += step;
		xtem = pow(1 - t, 3) * a.x +
			pow(1 - t, 2) * 3 * t * b.x +
			(1 - t) * 3 * t * t * c.x +
			t * t * t * d.x;
		ytem = pow(1 - t, 3) * a.y +
			pow(1 - t, 2) * 3 * t * b.y +
			(1 - t) * 3 * t * t * c.y +
			t * t * t * d.y;
		ztem = pow(1 - t, 3) * a.z +
			pow(1 - t, 2) * 3 * t * b.z +
			(1 - t) * 3 * t * t * c.z +
			t * t * t * d.z;

		points.push_back({ xtem, ytem, ztem, 1 });
	}
	points.push_back(d);



	return points;
}

std::vector<line> lines;
std::vector<vec3d> bezier1[4];
std::vector<std::vector<vec3d>> bezier2P;

void bezier2Thread(int left, int right, bool draww, int resolution)
{
	std::vector<vec3d> pointsTem;
	line lineTem;
	for (int b = left; b < right; b++)
	{

		pointsTem = bezier(resolution, bezier1[0][b], bezier1[1][b], bezier1[2][b], bezier1[3][b]);

		bezier2P[b] = pointsTem;

		if (0)
			for (int i = 1; i < pointsTem.size(); i++)
			{
				lineTem.p[0] = pointsTem[i - 1];
				lineTem.p[1] = pointsTem[i];
				lines.push_back(lineTem);
			}

	}
}

class controlPoints
{

public:
	

public:
	vec3d points[4][4];
	int resolution;

private:
	int wx, wy;
	
	

public:
	mesh surface1;

public:
	controlPoints()
	{
		wx = 1;
		wy = 1;

		setBasic();
	}

public:
	void setBasic()
	{
		float step = 0.33;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				points[i][j] = { (i * step - 0.5f) * 2, 0, (j * step - 0.5f) * 2, 1 };
			}
		}


	}

public:
	void draw(bool helpLines, bool choesen, bool bezier1, bool bezier2)
	{
		lines.clear();


		if (helpLines)
		{
			calculateLines();
		}

		if (choesen)
		{
			box boxTem;
			boxTem.setPosition(points[wx][wy]);
			lines = lines + boxTem.lines;
		}

		generateBezier1(bezier1);
		generateBezier2(bezier2);
	}

	void calculateLines()
	{
		std::vector<line> linesTem;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				linesTem.push_back({ points[j][i] ,points[j + 1][i] });
			}
		}
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				linesTem.push_back({ points[i][j] ,points[i][j + 1] });
			}
		}
		lines = linesTem;
	}

	void generateBezier1(bool draw)
	{
		
		std::vector<vec3d> pointsTem;
		line lineTem;

		for (int j = 0; j < 4; j++)
		{
			pointsTem = bezier(resolution, points[j][0], points[j][1], points[j][2], points[j][3]);
			bezier1[j] = pointsTem;

			if (draw)
				for (int i = 1; i < pointsTem.size(); i++)
				{
					lineTem.p[0] = pointsTem[i - 1];
					lineTem.p[1] = pointsTem[i];
					lines.push_back(lineTem);
				}
		}
	}

	

	void generateBezier2(bool draww)
	{
		std::vector<std::vector<vec3d>> emp(bezier1[0].size());
		bezier2P = emp;

		std::thread t1 = std::thread(bezier2Thread, 0, bezier1[0].size()/4, draww, resolution);
		std::thread t2 = std::thread(bezier2Thread, bezier1[0].size() / 4, bezier1[0].size() * 2/4, draww, resolution);
		std::thread t3 = std::thread(bezier2Thread, bezier1[0].size() * 2 / 4, bezier1[0].size() * 3 / 4, draww, resolution);
		std::thread t4 = std::thread(bezier2Thread, bezier1[0].size() * 3 / 4, bezier1[0].size(), draww, resolution);


		t1.join();
		t2.join();
		t3.join();
		t4.join();

		/*std::vector<vec3d> pointsTem;
		line lineTem;
		for (int b = 0; b < bezier1[0].size(); b++)
		{

			pointsTem = bezier(resolution, bezier1[0][b], bezier1[1][b], bezier1[2][b], bezier1[3][b]);

			bezier2P[b] = pointsTem;

			if (draw)
				for (int i = 1; i < pointsTem.size(); i++)
				{
					lineTem.p[0] = pointsTem[i - 1];
					lineTem.p[1] = pointsTem[i];
					lines.push_back(lineTem);
				}

		}*/
	}

	void setResolution(int r)
	{
		resolution = r;
	}

	void addResolution(int r)
	{
		resolution += r;
		if (resolution == 0)
			resolution++;
	}

	void choosePoint(int x)
	{
		wx = x % 4;
		wy = x / 4;
	}

	void moveChosen(float x, float y, float z)
	{
		points[wx][wy].move(x, y, z);
	}
	// !!!
	void generateMesh()
	{

		triangle triangleTem;
		mesh emp;
		surface1 = emp;
		for (int j = 0; j < bezier2P.size() - 1; j++)
		{
			for (int i = 1; i < bezier2P[j].size(); i++)
			{
				triangleTem.p[2] = bezier2P[j][i - 1];
				triangleTem.p[1] = bezier2P[j][i];
				triangleTem.p[0] = bezier2P[j + 1][i - 1];

				surface1.tris.push_back(triangleTem);
			}
		}

		for (int j = 0; j < bezier2P.size() - 1; j++)
		{
			for (int i = 1; i < bezier2P[j].size(); i++)
			{
				triangleTem.p[2] = bezier2P[j + 1][i];
				triangleTem.p[1] = bezier2P[j + 1][i - 1];
				triangleTem.p[0] = bezier2P[j][i];

				surface1.tris.push_back(triangleTem);
			}
		}
	}

	void upsideDown()
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				points[i][j].y *= -1;
			}
		}
	}


};


