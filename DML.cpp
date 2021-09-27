// DML.cpp : Defines the entry point for the application.
//

#include "DML.h"
#include "graph.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <numeric>

void runDistanceMagicLabeling(int n, std::vector<std::vector<int>> gr, std::vector<std::vector<int>> labelPerms, std::vector<int> vlabel);
bool validateGraph(std::vector<std::vector<int>> adjmat);
double evalDistanceMagic(std::vector<std::vector<int>> gr, std::vector<int> vlabel);
double stdDev(std::vector<int> val);
void distanceMagicLabeling(std::vector<std::vector<int>> adjmat, std::vector<std::vector<int>> labelPerms, std::vector<int>& rvlabel, double& rval);

int main(int argc, char* argv[])
{
	bool multithreadMode = false;
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
		runDistanceMagicLabeling(n, adjmats[i], labelPerms, vlabel);
	}

	return 0;
}

void runDistanceMagicLabeling(int n, std::vector<std::vector<int>> gr, std::vector<std::vector<int>> labelPerms, std::vector<int> vlabel)
{
	double val = 1;

	if (!(validateGraph(gr)))
	{
		return;
	}

	distanceMagicLabeling(gr, labelPerms, vlabel, val);

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

bool validateGraph(std::vector<std::vector<int>> adjmat)
{
	size_t n = adjmat.size();
	std::vector<int> w;
	for (int i = 0; i < n; i++)
	{
		int sum = 0;
		for (int j = 0; j < n; j++)
		{
			sum += adjmat[i][j];
		}
		w.push_back(sum);
	}

	int pval = std::accumulate(std::begin(w), std::end(w), 1, std::multiplies<int>());
	return pval != 0;
}

double evalDistanceMagic(std::vector<std::vector<int>> g, std::vector<int> vlabel)
{
	size_t n = g.size();
	std::vector<std::vector<int>> wgr(n, std::vector<int>(n, 0));

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			wgr[i][j] = vlabel[j] * g[i][j];
		}
	}

	std::vector<int> w;
	for (int i = 0; i < n; i++)
	{
		int sum = 0;
		for (int j = 0; j < n; j++)
		{
			sum += wgr[i][j];
		}
		w.push_back(sum);
	}

	return stdDev(w);
}

double stdDev(std::vector<int> val)
{
	size_t n = val.size();

	double sum = 0.0;
	double mean;
	double sd = 0.0;

	for (int i = 0; i < n; ++i)
	{
		sum += val[i];
	}

	mean = sum / n;

	for (int i = 0; i < n; ++i)
	{
		sd += pow(val[i] - mean, 2);
	}

	return sqrt(sd / n);
}

void distanceMagicLabeling(std::vector<std::vector<int>> adjmat, std::vector<std::vector<int>> labelPerms, std::vector<int>& rvlabel, double& rval)
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
		val = evalDistanceMagic(adjmat, vlabel);

		iter++;
	}

	rvlabel = vlabel;
	rval = val;
}