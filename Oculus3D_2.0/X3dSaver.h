#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

void saveToX3d(std::vector<std::vector<float>> points)
{
	for (auto i : points)
	{
		for (auto j : i)
		{
			std::cout << j << " ";
		}
		std::cout << "\n";
	}

	std::ifstream defaultX3d ("defaultX3d.txt");
	std::ofstream plik ("soczewkaX3d.html");
	std::string linia;

	int length = 0;
	int width = 0;

	int i = 0;
	while (std::getline(defaultX3d, linia))
	{
		if (i == 43)
		{
			width = points.size() - 1;
			length = points[0].size() / 3;


			for (int row = 0; row < width; row++)
			{

				if (row % 2 == 0)
				{
					for (int i = 0; i < length; i++)
					{
						plik << row * length + i << " ";
						plik << (row + 1) * length + i << " ";

						/*std::cout << row * length + i << " ";
						std::cout << (row + 1) * length + i << " ";*/
					}
				}
				else
				{
					for (int i = length - 1; i >= 0; i--)
					{
						plik << row * length + i << " ";
						plik << (row + 1) * length + i << " ";

						/*std::cout << row * length + i << " ";
						std::cout << (row + 1) * length + i << " ";*/

					}
				}
				
			}
			plik << "\n";
		}

		if (i == 46)
		{
			for (auto i : points)
			{
				for (auto j : i)
				{
					plik << j << " ";
				}
			}
			plik << "\n";
		}

		plik << linia << '\n';


		i++;
	}
	
}

