#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

std::vector<std::vector<std::vector<int>>> loadAdjMat(char* infilename, int& n)
{
	std::ifstream infile(infilename);
	std::string line;
	std::vector<std::vector<std::vector<int>>> adjmats;

	if (std::getline(infile, line))
	{
		n = std::stoi(line);
	}
	int i = 1;
	int m = 0;
	std::vector<std::vector<int>> row;
	while (std::getline(infile, line))
	{
		std::istringstream iss(line);

		if (i % (n + 1) != 0)
		{
			m++;
			std::vector<int> p(0);
			for (int j = 0; j < n; j++)
			{
				p.push_back(line[j] - '0');
			}
			row.push_back(p);

			if (m % n == 0)
			{
				std::vector<std::vector<int>> buf = std::move(row);
				adjmats.push_back(buf);
				m = 0;
			}
		}
		i++;
	}
	infile.close();

	return adjmats;
}