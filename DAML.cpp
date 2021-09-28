// DAML.cpp : Defines the entry point for the application.
//

#include "DML.h"
#include "graph.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <numeric>

void runDistanceAntiMagicLabeling(int n, std::vector<std::vector<int>> gr, std::vector<std::vector<int>> labelPerms, std::vector<int> vlabel);
bool validateGraph(std::vector<std::vector<int>> adjmat);
bool validateDAMGraph(std::vector<std::vector<int>> adjmat);
double evalDistanceAntiMagic(std::vector<std::vector<int>> g, std::vector<int> vlabel, std::vector<std::vector<int>>& wg);
void distanceAntiMagicLabeling(std::vector<std::vector<int>> adjmat, std::vector<std::vector<int>> labelPerms, std::vector<int>& rvlabel, double& rval);

int main(int argc, char* argv[])
{
	char* infilename;

	if (argc < 1)
	{
		return -1;
	}
	infilename = argv[1];

	int n;

	std::vector<std::vector<std::vector<int>>> adjmats = loadAdjMat(infilename, n);

	size_t ng = adjmats.size();

	std::vector<int> vlabel;
	for (int i = 0; i < n; i++)
	{
		vlabel.push_back(i + 1);
	}

	std::vector<std::vector<int>> labelPerms;
	int idx = 0;
	do
	{
		labelPerms.push_back(vlabel);
	} while (std::next_permutation(vlabel.begin(), vlabel.end()));

	for (int i = 0; i < ng; i++)
	{
		runDistanceAntiMagicLabeling(n, adjmats[i], labelPerms, vlabel);
	}

	return 0;
}

void runDistanceAntiMagicLabeling(int n, std::vector<std::vector<int>> gr, std::vector<std::vector<int>> labelPerms, std::vector<int> vlabel)
{
	double val = 1;

	if (!validateDAMGraph(gr))
	{
		return;
	}

	distanceAntiMagicLabeling(gr, labelPerms, vlabel, val);

	// val == 0 => can be labelled
	if (val == 0)
	{
		printf("adjmat: \n");
		for (int j = 0; j < n; j++)
		{
			for (int k = 0; k < n; k++)
			{
				printf("%d ", gr[j][k]);
			}
			printf("\n");
		}

		printf("label: [ ");
		for (int j = 0; j < n; j++)
		{
			printf("%d ", vlabel[j]);
		}
		printf("]");
		printf("\n");
	}
}

bool validateDAMGraph(std::vector<std::vector<int>> adjmat)
{
	size_t n = adjmat.size();
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			if (i != j)
			{
				if (adjmat[i] == adjmat[j])
				{
					return false;
				}
			}
		}
	}
	return true;
}

double evalDistanceAntiMagic(std::vector<std::vector<int>> g, std::vector<int> vlabel, std::vector<std::vector<int>>& wg)
{
	size_t n = g.size();
	std::vector<std::vector<int>> wgr(n, std::vector<int>(n, 0));
	std::vector<int> w(n);

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			wgr[i][j] = vlabel[j] * g[i][j];
			w[i] += vlabel[j] * g[i][j];
		}
	}

	std::sort(w.begin(), w.end());
	auto it = std::unique(w.begin(), w.end());

	wg = wgr;
	return !(it == w.end());
}

void distanceAntiMagicLabeling(std::vector<std::vector<int>> adjmat, std::vector<std::vector<int>> labelPerms, std::vector<int>& rvlabel, double& rval)
{
	size_t nVerts = adjmat.size();

	int nAcrs = 0;
	std::vector<int> vlabel(nVerts);
	for (int i = 0; i < nVerts; i++)
	{
		vlabel[i] = i + 1;

		int sum = 0;
		for (int j = 0; j < nVerts; j++)
		{
			sum += adjmat[i][j];
		}
	}

	size_t maxIter = labelPerms.size();

	double val = 1;
	int iter = 0;
	while (val != 0 && iter < maxIter)
	{
		vlabel = labelPerms[iter];

		std::vector<std::vector<int>> wg;
		val = evalDistanceAntiMagic(adjmat, vlabel, wg);

		if (val == 0)
		{
			std::vector<std::vector<int>> dam = wg;
		}

		iter++;
	}

	rvlabel = vlabel;
	rval = val;
}