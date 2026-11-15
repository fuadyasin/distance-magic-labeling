// Self-contained tests: known graphs, the input parsers, and a cross-check of
// the search against a brute force over all n! labelings.

#include "labeling.h"

#include <algorithm>
#include <cstdio>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	int failures = 0;

#define CHECK(cond) \
	do { if (!(cond)) { ++failures; std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

	const LabelingKind kAllKinds[] = {
		LabelingKind::DistanceMagic,
		LabelingKind::ClosedDistanceMagic,
		LabelingKind::DistanceAntimagic,
	};

	Graph makeGraph(int n, const std::vector<std::pair<int, int>>& edges)
	{
		Graph g;
		g.n = n;
		g.adj.assign(n, 0);
		for (const auto& e : edges)
		{
			g.adj[e.first] |= std::uint64_t{1} << e.second;
			g.adj[e.second] |= std::uint64_t{1} << e.first;
		}
		return g;
	}

	Graph completeGraph(int n)
	{
		std::vector<std::pair<int, int>> edges;
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < i; ++j)
			{
				edges.push_back({j, i});
			}
		}
		return makeGraph(n, edges);
	}

	// Reference implementation: the first valid labeling in lexicographic
	// order, with the same conventions as findLabeling.
	bool bruteForce(const Graph& g, LabelingKind kind, std::vector<int>& labels)
	{
		if (kind == LabelingKind::DistanceMagic &&
			std::find(g.adj.begin(), g.adj.end(), std::uint64_t{0}) != g.adj.end())
		{
			return false;
		}
		std::vector<int> perm(g.n);
		std::iota(perm.begin(), perm.end(), 1);
		do
		{
			if (isValidLabeling(g, kind, perm))
			{
				labels = perm;
				return true;
			}
		} while (std::next_permutation(perm.begin(), perm.end()));
		return false;
	}

	void checkAgainstBruteForce(const Graph& g)
	{
		for (LabelingKind kind : kAllKinds)
		{
			std::vector<int> expected, actual;
			const bool expectedFound = bruteForce(g, kind, expected);
			const bool actualFound = findLabeling(g, kind, actual);
			CHECK(expectedFound == actualFound);
			if (expectedFound && actualFound)
			{
				CHECK(expected == actual);
			}
		}
	}

	bool readFails(const std::string& text)
	{
		std::istringstream in(text);
		GraphReader reader(in);
		Graph g;
		try
		{
			while (reader.next(g)) {}
		}
		catch (const std::runtime_error&)
		{
			return true;
		}
		return false;
	}

	void testKnownGraphs()
	{
		std::vector<int> labels;

		// C4: opposite vertices share both neighbours, so 1+4 = 2+3 works.
		const Graph c4 = makeGraph(4, {{0, 1}, {1, 2}, {2, 3}, {0, 3}});
		CHECK(findLabeling(c4, LabelingKind::DistanceMagic, labels));
		CHECK((labels == std::vector<int>{1, 2, 4, 3}));
		CHECK((vertexWeights(c4, LabelingKind::DistanceMagic, labels) == std::vector<int>{5, 5, 5, 5}));
		CHECK(!findLabeling(c4, LabelingKind::ClosedDistanceMagic, labels));
		CHECK(!findLabeling(c4, LabelingKind::DistanceAntimagic, labels));

		// K_n is closed distance magic with constant n(n+1)/2 and distance
		// antimagic (weights are total minus own label), but never distance magic.
		for (int n = 1; n <= 9; ++n)
		{
			const Graph k = completeGraph(n);
			CHECK(findLabeling(k, LabelingKind::ClosedDistanceMagic, labels));
			CHECK(vertexWeights(k, LabelingKind::ClosedDistanceMagic, labels)[0] == n * (n + 1) / 2);
			CHECK(!findLabeling(k, LabelingKind::DistanceMagic, labels));
			CHECK(findLabeling(k, LabelingKind::DistanceAntimagic, labels));
		}

		// P3 is distance magic (centre gets 3), P4 is not.
		const Graph p3 = makeGraph(3, {{0, 1}, {1, 2}});
		CHECK(findLabeling(p3, LabelingKind::DistanceMagic, labels));
		CHECK((labels == std::vector<int>{1, 3, 2}));
		const Graph p4 = makeGraph(4, {{0, 1}, {1, 2}, {2, 3}});
		CHECK(!findLabeling(p4, LabelingKind::DistanceMagic, labels));
		CHECK(findLabeling(p4, LabelingKind::DistanceAntimagic, labels));
		CHECK(isValidLabeling(p4, LabelingKind::DistanceAntimagic, labels));

		// Isolated vertices rule out a distance magic labeling.
		CHECK(!findLabeling(makeGraph(1, {}), LabelingKind::DistanceMagic, labels));
		CHECK(!findLabeling(makeGraph(3, {}), LabelingKind::DistanceMagic, labels));
		CHECK(!findLabeling(makeGraph(0, {}), LabelingKind::ClosedDistanceMagic, labels));

		// K_{6,6}: far beyond what enumerating 12! labelings could handle.
		std::vector<std::pair<int, int>> edges;
		for (int i = 0; i < 6; ++i)
		{
			for (int j = 6; j < 12; ++j)
			{
				edges.push_back({i, j});
			}
		}
		const Graph k66 = makeGraph(12, edges);
		CHECK(findLabeling(k66, LabelingKind::DistanceMagic, labels));
		CHECK(isValidLabeling(k66, LabelingKind::DistanceMagic, labels));
		CHECK(vertexWeights(k66, LabelingKind::DistanceMagic, labels)[0] == 39);
	}

	void testIsValidLabeling()
	{
		const Graph c4 = makeGraph(4, {{0, 1}, {1, 2}, {2, 3}, {0, 3}});
		CHECK(isValidLabeling(c4, LabelingKind::DistanceMagic, {1, 2, 4, 3}));
		CHECK(!isValidLabeling(c4, LabelingKind::DistanceMagic, {1, 2, 3, 4}));
		CHECK(!isValidLabeling(c4, LabelingKind::DistanceMagic, {1, 1, 4, 4})); // not a bijection
		CHECK(!isValidLabeling(c4, LabelingKind::DistanceMagic, {1, 2, 4}));
	}

	void testParsers()
	{
		CHECK(parseGraph6("@").n == 1);
		CHECK(parseGraph6("C~").adj == completeGraph(4).adj);
		CHECK(parseGraph6("D~{").adj == completeGraph(5).adj);
		CHECK(parseGraph6(">>graph6<<Cl").adj == makeGraph(4, {{0, 1}, {1, 2}, {2, 3}, {0, 3}}).adj);

		// Mixed formats, CRLF line ends, a missing blank line between
		// matrices, and the spaced rows this program prints itself.
		std::istringstream in(
			"Cl\n"
			"\n"
			"Graph 1, order 3.\r\n"
			"010\r\n"
			"101\r\n"
			"010\r\n"
			"Graph 2, order 2.\n"
			"0 1 \n"
			"1 0 \n"
			"\n"
			"C~\n");
		GraphReader reader(in);
		Graph g;
		CHECK(reader.next(g) && g.n == 4 && g.adj == makeGraph(4, {{0, 1}, {1, 2}, {2, 3}, {0, 3}}).adj);
		CHECK(reader.next(g) && g.n == 3 && g.adj == makeGraph(3, {{0, 1}, {1, 2}}).adj);
		CHECK(reader.next(g) && g.n == 2 && g.adj == completeGraph(2).adj);
		CHECK(reader.next(g) && g.n == 4 && g.adj == completeGraph(4).adj);
		CHECK(!reader.next(g));

		CHECK(readFails("C\n"));                       // graph6 too short
		CHECK(readFails("C~~\n"));                     // graph6 too long
		CHECK(readFails("C \n"));                      // invalid graph6 character
		CHECK(readFails(":Cc\n"));                     // sparse6
		CHECK(readFails("Graph 1\n01\n"));             // not square
		CHECK(readFails("Graph 1\n01\n00\n"));         // not symmetric
		CHECK(readFails("Graph 1\n10\n00\n"));         // loop
		CHECK(readFails("Graph 1\n0x\n10\n"));         // bad digit
		CHECK(readFails("Graph 1\n\n"));               // no rows
		CHECK(!readFails(""));
	}

	void testAgainstBruteForce()
	{
		// Every labelled graph on up to 5 vertices.
		for (int n = 1; n <= 5; ++n)
		{
			const int nPairs = n * (n - 1) / 2;
			for (int mask = 0; mask < (1 << nPairs); ++mask)
			{
				std::vector<std::pair<int, int>> edges;
				int bit = 0;
				for (int j = 1; j < n; ++j)
				{
					for (int i = 0; i < j; ++i, ++bit)
					{
						if ((mask >> bit) & 1)
						{
							edges.push_back({i, j});
						}
					}
				}
				checkAgainstBruteForce(makeGraph(n, edges));
			}
		}

		// Random graphs of varying density on 6 to 8 vertices.
		std::mt19937 rng(12345);
		for (int n = 6; n <= 8; ++n)
		{
			for (int trial = 0; trial < 60; ++trial)
			{
				std::bernoulli_distribution edge(0.2 + 0.1 * (trial % 7));
				std::vector<std::pair<int, int>> edges;
				for (int j = 1; j < n; ++j)
				{
					for (int i = 0; i < j; ++i)
					{
						if (edge(rng))
						{
							edges.push_back({i, j});
						}
					}
				}
				checkAgainstBruteForce(makeGraph(n, edges));
			}
		}
	}
}

int main()
{
	testKnownGraphs();
	testIsValidLabeling();
	testParsers();
	testAgainstBruteForce();

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("all tests passed\n");
	return 0;
}
