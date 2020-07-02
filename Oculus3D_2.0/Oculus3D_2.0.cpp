#include <SFML/Graphics.hpp>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <strstream>
#include <algorithm>
#include <string>
#include <sstream> 
#include "nurbs_v2.h"
#include "X3dSaver.h"
#include <ctime>

using namespace std;

#define windowX 1600
#define windowY 900

void save(controlPoints s)
{
	ofstream myfile;
	myfile.open("example.txt");
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{

			myfile << (int)(s.points[i][j].x * 10000);
			myfile << ' ';
			myfile << (int)(s.points[i][j].y * 10000);
			myfile << ' ';
			myfile << (int)(s.points[i][j].z * 10000);
			myfile << ' ';
		}
	}
	myfile.close();
}

void load(controlPoints& l)
{
	stringstream geek("2");
	int temi;
	std::string linia;
	ifstream myfile("example.txt", std::ios::in);
	int value;
	int i = 0;

	while (!myfile.eof())
	{
		myfile >> value;
		l.points[i / 4][i % 4].x = float(value / 10000.0);
		myfile >> value;
		l.points[i / 4][i % 4].y = float(value / 10000.0);
		myfile >> value;
		l.points[i / 4][i % 4].z = float(value / 10000.0);
		//l->points[i / 4][i % 4].w = 1.0;
		i++;


	}
	i++;

	l.choosePoint(0);
	l.setResolution(10);
}

struct mat4x4
{
	float m[4][4] = { 0 };
};

class Oculus3D
{
private:
	sf::RenderWindow *window;
	sf::ConvexShape temPolygon;

	controlPoints temCp;
	controlPoints cp;
	controlPoints cp2;
	bool l, c, b1, b2, s, change, ud;
	long long timer;
	float timeChangeBuffer;
	float zoom;

	mesh meshSurface;
	mat4x4 matProj;	// Matrix that converts from view space to screen space
	vec3d vCamera;	// Location of camera in world space
	vec3d vLookDir;	// Direction vector along the direction camera points
	float fYaw;
	float xr = 0, yr = 0, zr = 0;	// Spins World transform

	std::vector<std::vector<float>> transfusionToX3d;
	std::vector<float> temToX3d;

	vec3d Matrix_MultiplyVector(mat4x4& m, vec3d& i)
	{
		vec3d v;
		v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
		v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
		v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
		v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
		return v;
	}

	mat4x4 Matrix_MakeIdentity()
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeRotationX(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[1][2] = sinf(fAngleRad);
		matrix.m[2][1] = -sinf(fAngleRad);
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeRotationY(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][2] = sinf(fAngleRad);
		matrix.m[2][0] = -sinf(fAngleRad);
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeRotationZ(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][1] = sinf(fAngleRad);
		matrix.m[1][0] = -sinf(fAngleRad);
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeTranslation(float x, float y, float z)
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		matrix.m[3][0] = x;
		matrix.m[3][1] = y;
		matrix.m[3][2] = z;
		return matrix;
	}

	mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
	{
		float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
		mat4x4 matrix;
		matrix.m[0][0] = fAspectRatio * fFovRad;
		matrix.m[1][1] = fFovRad;
		matrix.m[2][2] = fFar / (fFar - fNear);
		matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matrix.m[2][3] = 1.0f;
		matrix.m[3][3] = 0.0f;
		return matrix;
	}

	mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
	{
		mat4x4 matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
		return matrix;
	}

	mat4x4 Matrix_PointAt(vec3d& pos, vec3d& target, vec3d& up)
	{
		// Calculate new forward direction
		vec3d newForward = Vector_Sub(target, pos);
		newForward = Vector_Normalise(newForward);

		// Calculate new Up direction
		vec3d a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
		vec3d newUp = Vector_Sub(up, a);
		newUp = Vector_Normalise(newUp);

		// New Right direction is easy, its just cross product
		vec3d newRight = Vector_CrossProduct(newUp, newForward);

		// Construct Dimensioning and Translation Matrix	
		mat4x4 matrix;
		matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
		return matrix;

	}

	mat4x4 Matrix_QuickInverse(mat4x4& m) // Only for Rotation/Translation Matrices
	{
		mat4x4 matrix;
		matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
		matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
		matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	vec3d Vector_Add(vec3d& v1, vec3d& v2)
	{
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}

	vec3d Vector_Sub(vec3d& v1, vec3d& v2)
	{
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}

	vec3d Vector_Mul(vec3d& v1, float k)
	{
		return { v1.x * k, v1.y * k, v1.z * k };
	}

	vec3d Vector_Div(vec3d& v1, float k)
	{
		return { v1.x / k, v1.y / k, v1.z / k };
	}

	float Vector_DotProduct(vec3d& v1, vec3d& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	float Vector_Length(vec3d& v)
	{
		return sqrtf(Vector_DotProduct(v, v));
	}

	vec3d Vector_Normalise(vec3d& v)
	{
		float l = Vector_Length(v);
		return { v.x / l, v.y / l, v.z / l };
	}

	vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
	{
		vec3d v;
		v.x = v1.y * v2.z - v1.z * v2.y;
		v.y = v1.z * v2.x - v1.x * v2.z;
		v.z = v1.x * v2.y - v1.y * v2.x;
		return v;
	}


public:
	int Start()
	{

		if (!OnUserCreate())
			return 0;


		while (window->isOpen())
		{
			
			sf::Event event;
			while (window->pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					window->close();
			}
			

			

			Sleep(40);
			window->clear();
			OnUserUpdate((clock() - timer)/1000.0f);
			timer = clock();
			window->display();
		}
		

		return 1;
	}

	bool OnUserCreate()
	{
		timer = clock();
		window = new sf::RenderWindow(sf::VideoMode(windowX, windowY), "SFML works!");
		matProj = Matrix_MakeProjection(90.0f, (float)windowY / (float)windowX, 0.1f, 1000.0f);

		cp.setBasic();
		cp.calculateLines();
		cp.setResolution(20);
		cp2 = cp;
		change = 0;
		l = 1; c = 1; b1 = 1; b2 = 1; s = 1; ud = 1;
		zoom = 3.0;

		temPolygon = sf::ConvexShape(3);
		

		/*std::vector<std::vector<float>> temTest =
		{ { 0, 0, 0, 0, 1 ,0, 0, 2, 0, 0, 3, 0, 0, 4, 0 },
		{	1, 0, 0, 1, 1, 0, 1, 2, 2, 1, 3 ,3 ,1 ,4, 2},
		{2, 0, 0, 2, 1, 0, 2, 2, 0, 2, 3, 0, 2, 4, 0 }};
		saveToX3d(temTest);*/

		return true;
	}

	bool OnUserUpdate(float fElapsedTime)
	{

		mat4x4 matRotZ, matRotX, matRotY;
		mat4x4 matTrans;


		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F7))
		{
			save(cp);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F8))
		{
			//shared_ptr<controlPoints> cps = make_shared<controlPoints>(cp);

			load(cp);

		}


		matTrans = Matrix_MakeTranslation(0.0f, 0.0f, zoom);

		float movement = 1.0f * fElapsedTime;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract))
			zoom -= movement;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add))
			zoom += movement;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			xr += 1.0f * fElapsedTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			xr -= 1.0f * fElapsedTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			yr += 1.0f * fElapsedTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			yr -= 1.0f * fElapsedTime;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::V))
			cp.choosePoint(0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
			cp.choosePoint(1);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
			cp.choosePoint(2);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
			cp.choosePoint(3);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F))
			cp.choosePoint(4);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			cp.choosePoint(5);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			cp.choosePoint(6);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			cp.choosePoint(7);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
			cp.choosePoint(8);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
			cp.choosePoint(9);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			cp.choosePoint(10);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
			cp.choosePoint(11);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4))
			cp.choosePoint(12);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))
			cp.choosePoint(13);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
			cp.choosePoint(14);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
			cp.choosePoint(15);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::I))
			cp.moveChosen(0.0, 0.0, movement);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::K))
			cp.moveChosen(0.0, 0.0, -1 * movement);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::J))
			cp.moveChosen(movement, 0.0, 0.0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
			cp.moveChosen(-1 * movement, 0.0, 0.0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::U))
			cp.moveChosen(0.0, movement, 0.0);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::O))
			cp.moveChosen(0.0, -1 * movement, 0.0);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
			cp.addResolution(1);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::N))
			cp.addResolution(-1);

		if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F1)) && change == 0)
		{
			change = 1;
			if (l == 0)
				l = 1;
			else
				l = 0;
		}
		else
			if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F2)) && change == 0)
			{
				change = 1;
				if (c == 0)
					c = 1;
				else
					c = 0;
			}
			else
				if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F3)) && change == 0)
				{
					change = 1;
					if (b1 == 0)
						b1 = 1;
					else
						b1 = 0;
				}
				else
					if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F4)) && change == 0)
					{
						change = 1;
						if (b2 == 0)
							b2 = 1;
						else
							b2 = 0;
					}
					else
						if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F5)) && change == 0)
						{
							change = 1;
							if (s == 0)
								s = 1;
							else
								s = 0;
						}
						else
							if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) && change == 0)
							{
								change = 1;
								if (ud == 0)
								{
									cp.upsideDown();
									ud = 1;
								}
								else
								{
									cp.upsideDown();
									ud = 0;
								}
							}
							else
								if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F9)) && change == 0)
								{
									change = 1;
									cp2 = cp;
								}
								else
									if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F10)) && change == 0)
									{
										change = 1;
										temCp = cp;
										cp = cp2;
										cp2 = temCp;
									}
									else
										if ((sf::Keyboard::isKeyPressed(sf::Keyboard::F6)) && change == 0)
										{

											for (auto i : cp.bezier2P)
											{
												for (auto j : i)
												{
													temToX3d.push_back(j.x);
													temToX3d.push_back(j.y);
													temToX3d.push_back(j.z);
													
												}
												transfusionToX3d.push_back(temToX3d);
												temToX3d.clear();
											}

											for (auto i : cp2.bezier2P)
											{
												for (auto j : i)
												{
													temToX3d.push_back(j.x);
													temToX3d.push_back(j.y);
													temToX3d.push_back(j.z);

												}
												transfusionToX3d.push_back(temToX3d);
												temToX3d.clear();
											}
										
											saveToX3d (transfusionToX3d);
										}
										else
										{
											timeChangeBuffer += fElapsedTime;
											if (timeChangeBuffer > 0.4)
											{
												change = 0;
												timeChangeBuffer = 0;
											}
										}

		cp.draw(l, c, b1, b2);

		matRotY = Matrix_MakeRotationY(yr);
		matRotX = Matrix_MakeRotationX(xr);
		matRotZ = Matrix_MakeRotationZ(zr);
		mat4x4 matWorld;
		matWorld = Matrix_MakeIdentity();	// Form World Matrix
		matWorld = Matrix_MultiplyMatrix(matRotY, matRotX); // Transform by rotation
		matWorld = Matrix_MultiplyMatrix(matWorld, matRotZ);
		matWorld = Matrix_MultiplyMatrix(matWorld, matTrans); // Transform by translation, moving from viewer

		// Create "Point At" Matrix for camera
		vec3d vUp = { 0,1,0 };
		vec3d vTarget = { 0,0,1 };
		vCamera = { 0,0,0 };
		mat4x4 matCamera = Matrix_PointAt(vCamera, vTarget, vUp);

		// Make view matrix from camera
		mat4x4 matView = Matrix_QuickInverse(matCamera);


		if (s)
		{
			// Store triagles for rastering later
			vector<triangle> vecTrianglesToRaster;

			// Draw Triangles
			cp.generateMesh();
			meshSurface = cp.surface1;
			for (auto tri : meshSurface.tris)
			{
				triangle triProjected, triTransformed, triViewed;

				// World Matrix Transform
				triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
				triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
				triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

				// Calculate triangle Normal
				vec3d normal, line1, line2;

				// Get lines either side of triangle
				line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
				line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

				// Take cross product of lines to get normal to triangle surface
				normal = Vector_CrossProduct(line1, line2);
				/*normal.x *= -1;
				normal.y *= -1;
				normal.z *= -1;*/

				// Normalise a normal
				normal = Vector_Normalise(normal);

				// Get ray from triangle to camera
				vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

				// If ray is aligned with normal, then triangle is visible
				if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
				{
					// Illumination
					vec3d light_direction = { -0.5f, 0.5f, -0.5f };
					light_direction = Vector_Normalise(light_direction);

					// How "aligned" are light direction and triangle surface normal?
					float dp = max(0.1f, Vector_DotProduct(light_direction, normal));

					// !!!
					// Choose console colours as required (much easier with RGB)
					triTransformed.dp = dp;


					// Convert World Space --> View Space
					triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
					triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
					triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);
					triViewed.dp = triTransformed.dp;



					// Project triangles from 3D --> 2D
					triProjected.p[0] = Matrix_MultiplyVector(matProj, triViewed.p[0]);
					triProjected.p[1] = Matrix_MultiplyVector(matProj, triViewed.p[1]);
					triProjected.p[2] = Matrix_MultiplyVector(matProj, triViewed.p[2]);
					triProjected.dp = triViewed.dp;

					// Scale into view, we moved the normalising into cartesian space
					// out of the matrix
					triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
					triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
					triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

					// X/Y are inverted
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					// !!!
					// Offset verts into visible normalised space
					vec3d vOffsetView = { 1,1,0 }; // to the middle of the screen
					triProjected.p[0] = Vector_Add(triProjected.p[0], vOffsetView);
					triProjected.p[1] = Vector_Add(triProjected.p[1], vOffsetView);
					triProjected.p[2] = Vector_Add(triProjected.p[2], vOffsetView);
					triProjected.p[0].x *= 0.5f * (float)windowX;
					triProjected.p[0].y *= 0.5f * (float)windowY;
					triProjected.p[1].x *= 0.5f * (float)windowX;
					triProjected.p[1].y *= 0.5f * (float)windowY;
					triProjected.p[2].x *= 0.5f * (float)windowX;
					triProjected.p[2].y *= 0.5f * (float)windowY;

					// Store triangle for sorting
					vecTrianglesToRaster.push_back(triProjected);

				}
			}

			// Sort triangles from back to front
			sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
				{
					float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
					float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
					return z1 > z2;
				});


			for (auto& t : vecTrianglesToRaster)
			{
				temPolygon.setPoint(0, sf::Vector2f(t.p[0].x, t.p[0].y));
				temPolygon.setPoint(1, sf::Vector2f(t.p[1].x, t.p[1].y));
				temPolygon.setPoint(2, sf::Vector2f(t.p[2].x, t.p[2].y));

				temPolygon.setFillColor(sf::Color(255 * t.dp, 255 * t.dp, 255 * t.dp));


				window->draw(temPolygon);

			}
		}

		// Recreating other side of the lean
		if (s)
		{
			// Store triagles for rastering later
			vector<triangle> vecTrianglesToRaster;
			// Draw Triangles
			meshSurface = cp2.surface1;
			for (auto tri : meshSurface.tris)
			{
				triangle triProjected, triTransformed, triViewed;

				// World Matrix Transform
				triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
				triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
				triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

				// Calculate triangle Normal
				vec3d normal, line1, line2;

				// Get lines either side of triangle
				line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
				line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

				// Take cross product of lines to get normal to triangle surface
				normal = Vector_CrossProduct(line1, line2);
				normal.x *= -1;
				normal.y *= -1;
				normal.z *= -1;

				// Normalise a normal
				normal = Vector_Normalise(normal);

				// Get ray from triangle to camera
				vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

				// If ray is aligned with normal, then triangle is visible
				if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
				{
					// Illumination
					vec3d light_direction = { -0.5f, 0.5f, -0.5f };
					light_direction = Vector_Normalise(light_direction);

					// How "aligned" are light direction and triangle surface normal?
					float dp = max(0.1f, Vector_DotProduct(light_direction, normal));

					triTransformed.dp = dp;

					// Convert World Space --> View Space
					triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
					triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
					triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);
					triViewed.dp = triTransformed.dp;


					// Project triangles from 3D --> 2D
					triProjected.p[0] = Matrix_MultiplyVector(matProj, triViewed.p[0]);
					triProjected.p[1] = Matrix_MultiplyVector(matProj, triViewed.p[1]);
					triProjected.p[2] = Matrix_MultiplyVector(matProj, triViewed.p[2]);
					triProjected.dp = triViewed.dp;

					// Scale into view, we moved the normalising into cartesian space
					// out of the matrix
					triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
					triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
					triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

					// X/Y are inverted
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					// Offset verts into visible normalised space
					vec3d vOffsetView = { 1,1,0 }; // to the middle of the screen
					triProjected.p[0] = Vector_Add(triProjected.p[0], vOffsetView);
					triProjected.p[1] = Vector_Add(triProjected.p[1], vOffsetView);
					triProjected.p[2] = Vector_Add(triProjected.p[2], vOffsetView);
					triProjected.p[0].x *= 0.5f * (float)windowX;
					triProjected.p[0].y *= 0.5f * (float)windowY;
					triProjected.p[1].x *= 0.5f * (float)windowX;
					triProjected.p[1].y *= 0.5f * (float)windowY;
					triProjected.p[2].x *= 0.5f * (float)windowX;
					triProjected.p[2].y *= 0.5f * (float)windowY;

					// Store triangle for sorting
					vecTrianglesToRaster.push_back(triProjected);

				}
			}

			// Sort triangles from back to front
			sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
			{
				float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
				float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
				return z1 > z2;
			});


			for (auto& t : vecTrianglesToRaster)
			{
				temPolygon.setPoint(0, sf::Vector2f(t.p[0].x, t.p[0].y));
				temPolygon.setPoint(1, sf::Vector2f(t.p[1].x, t.p[1].y));
				temPolygon.setPoint(2, sf::Vector2f(t.p[2].x, t.p[2].y));

				temPolygon.setFillColor(sf::Color(255 * t.dp, 255 * t.dp, 255 * t.dp));

				window->draw(temPolygon);

			}
		}

		if (1)
		{
			std::vector<line> lineShape;
			std::vector<line> linesToDraw;
			line lineTem;

			std::vector<line> linesTem;

			vec3d p1, p2, p3, p4;





			lineShape = cp.lines;

			for (auto lin : lineShape)
			{
				line linProjected, linTransformed, linViewed;

				// World Matrix Transform
				linTransformed.p[0] = Matrix_MultiplyVector(matWorld, lin.p[0]);
				linTransformed.p[1] = Matrix_MultiplyVector(matWorld, lin.p[1]);

				// Convert World Space --> View Space
				linViewed.p[0] = Matrix_MultiplyVector(matView, linTransformed.p[0]);
				linViewed.p[1] = Matrix_MultiplyVector(matView, linTransformed.p[1]);



				// Project lines from 3D --> 2D
				linProjected.p[0] = Matrix_MultiplyVector(matProj, linViewed.p[0]);
				linProjected.p[1] = Matrix_MultiplyVector(matProj, linViewed.p[1]);


				// Scale into view, we moved the normalising into cartesian space
				// out of the matrix
				linProjected.p[0] = Vector_Div(linProjected.p[0], linProjected.p[0].w);
				linProjected.p[1] = Vector_Div(linProjected.p[1], linProjected.p[1].w);


				// X/Y are inverted
				linProjected.p[0].x *= -1.0f;
				linProjected.p[1].x *= -1.0f;
				linProjected.p[0].y *= -1.0f;
				linProjected.p[1].y *= -1.0f;

				// Offset verts into visible normalised space
				vec3d vOffsetView = { 1,1,0 }; // to the middle of the screen
				linProjected.p[0] = Vector_Add(linProjected.p[0], vOffsetView);
				linProjected.p[1] = Vector_Add(linProjected.p[1], vOffsetView);
				linProjected.p[0].x *= 0.5f * (float)windowX;
				linProjected.p[0].y *= 0.5f * (float)windowY;
				linProjected.p[1].x *= 0.5f * (float)windowX;
				linProjected.p[1].y *= 0.5f * (float)windowY;


				// Store lines for drawing
				linesToDraw.push_back(linProjected);


			}

			for (auto& t : linesToDraw)
			{
				// !!!
				//DrawLine(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, PIXEL_SOLID, FG_GREEN);
			}
		}
		return true;
	}
};




int main()
{

	Oculus3D oculus;
	oculus.Start();


	return 0;
}

