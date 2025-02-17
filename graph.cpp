#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

std::vector<std::vector<std::vector<int>>> loadAdjMat(char* infilename, int& n)
{
	std::ifstream infile(infilename);
	if (!infile.is_open())
	{
		std::cerr << "Error: file not found" << std::endl;
		return {};
	}

	std::string line;
	std::vector<std::vector<std::vector<int>>> adjmats;

    while (std::getline(infile, line))
    {
        if (line.find("Graph") != std::string::npos)
        {
            std::getline(infile, line);
            std::vector<std::vector<int>> adjmat(n, std::vector<int>(n, 0));

            int i = 0;
            do
            {
                for (int j = 0; j < n; ++j)
                {
                    adjmat[i][j] = line[j] - '0';
                }

                i++;
            }
			while (std::getline(infile, line) && !line.empty());

            adjmats.push_back(adjmat);
        }
    }
	infile.close();

	return adjmats;
}