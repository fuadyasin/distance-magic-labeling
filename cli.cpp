#include "cli.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
	// Graphs are read, solved in parallel and printed one batch at a time, so
	// memory use stays flat however long the input is. Listing every labeling
	// keeps them all in memory until the batch is printed, so use smaller
	// batches then.
	constexpr size_t kBatchSize = 4096;
	constexpr size_t kListAllBatchSize = 64;

	enum class Mode
	{
		First,   // the lexicographically smallest labeling of each graph
		All,     // every labeling of each graph
		Count,   // the number of labelings of each graph
		Without, // the graphs that have no labeling
	};

	struct Options
	{
		Mode mode = Mode::First;
		bool graph6 = false;
	};

	void printUsage(const char* prog, LabelingKind kind)
	{
		std::fprintf(stderr,
			"Usage: %s [-j threads] [-a | -c | -v] [-g] <file | -> [n]\n"
			"\n"
			"Searches every graph in the input for a %s labeling and prints\n"
			"the graphs that have one, together with the labeling found.\n"
			"\n"
			"  file         graphs in graph6 format (geng output) or as adjacency\n"
			"               matrices (showg -a output); use - for standard input\n"
			"  n            optional: fail unless every graph has exactly n vertices\n"
			"  -j threads   number of worker threads (default: all cores)\n"
			"  -a           print every labeling, not just the smallest\n"
			"  -c           print the number of labelings instead of a labeling\n"
			"  -v           print the graphs that have no labeling instead\n"
			"  -g           print one line per result, starting with the graph in\n"
			"               graph6, instead of the adjacency matrix\n",
			prog, kindName(kind));
	}

	bool parsePositiveInt(const std::string& text, int& value)
	{
		try
		{
			size_t used = 0;
			value = std::stoi(text, &used);
			return used == text.size() && value > 0;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	bool isMagic(LabelingKind kind)
	{
		return kind != LabelingKind::DistanceAntimagic;
	}

	void printAdjacencyMatrix(const Graph& g)
	{
		std::printf("adjmat: \n");
		for (int i = 0; i < g.n; ++i)
		{
			for (int j = 0; j < g.n; ++j)
			{
				std::printf("%d ", g.hasEdge(i, j) ? 1 : 0);
			}
			std::printf("\n");
		}
	}

	// One labeling: "k: K" and "label: [ ... ]" lines, or with -g a single
	// tab-separated line "graph6 [k] labels".
	void printLabeling(LabelingKind kind, const Options& opts, const Graph& g, const std::string& g6, const int* labels)
	{
		const std::vector<int> labelVec(labels, labels + g.n);
		const int k = isMagic(kind) ? vertexWeights(g, kind, labelVec)[0] : 0;
		if (opts.graph6)
		{
			std::printf("%s\t", g6.c_str());
			if (isMagic(kind))
			{
				std::printf("%d\t", k);
			}
			for (int i = 0; i < g.n; ++i)
			{
				std::printf(i == 0 ? "%d" : " %d", labels[i]);
			}
			std::printf("\n");
			return;
		}

		if (isMagic(kind))
		{
			std::printf("k: %d\n", k);
		}
		std::printf("label: [ ");
		for (int i = 0; i < g.n; ++i)
		{
			std::printf("%d ", labels[i]);
		}
		std::printf("]\n");
	}

	struct Result
	{
		unsigned long long count = 0;
		std::vector<int> labels; // the labelings found, n entries each
	};

	Result solve(LabelingKind kind, Mode mode, const Graph& g)
	{
		Result r;
		switch (mode)
		{
		case Mode::First:
			r.count = findLabeling(g, kind, r.labels) ? 1 : 0;
			break;
		case Mode::All:
			r.count = forEachLabeling(g, kind, [&](const std::vector<int>& labels)
			{
				r.labels.insert(r.labels.end(), labels.begin(), labels.end());
				return true;
			});
			break;
		case Mode::Count:
			r.count = countLabelings(g, kind);
			break;
		case Mode::Without:
		{
			std::vector<int> labels;
			r.count = findLabeling(g, kind, labels) ? 1 : 0;
			break;
		}
		}
		return r;
	}

	// Prints one graph's result; returns true if the graph counts towards the
	// summary (has a labeling, or with -v has none).
	bool printResult(LabelingKind kind, const Options& opts, const Graph& g, const Result& r)
	{
		if (opts.mode == Mode::Without)
		{
			if (r.count != 0)
			{
				return false;
			}
			if (opts.graph6)
			{
				std::printf("%s\n", toGraph6(g).c_str());
			}
			else
			{
				printAdjacencyMatrix(g);
			}
			return true;
		}

		if (r.count == 0)
		{
			return false;
		}
		const std::string g6 = opts.graph6 ? toGraph6(g) : std::string();
		if (!opts.graph6)
		{
			printAdjacencyMatrix(g);
		}
		if (opts.mode == Mode::Count)
		{
			if (opts.graph6)
			{
				std::printf("%s\t%llu\n", g6.c_str(), r.count);
			}
			else
			{
				std::printf("count: %llu\n", r.count);
			}
			return true;
		}
		for (size_t i = 0; i < r.labels.size(); i += static_cast<size_t>(g.n))
		{
			printLabeling(kind, opts, g, g6, &r.labels[i]);
		}
		return true;
	}

	void solveBatch(LabelingKind kind, Mode mode, const std::vector<Graph>& graphs, std::vector<Result>& results, int nThreads)
	{
		results.assign(graphs.size(), Result{});
		if (graphs.empty())
		{
			return;
		}

		std::atomic<size_t> nextIndex{0};
		auto worker = [&]()
		{
			for (size_t i = nextIndex++; i < graphs.size(); i = nextIndex++)
			{
				results[i] = solve(kind, mode, graphs[i]);
			}
		};

		std::vector<std::thread> threads;
		const size_t nExtra = std::min<size_t>(nThreads, graphs.size()) - 1;
		for (size_t t = 0; t < nExtra; ++t)
		{
			threads.emplace_back(worker);
		}
		worker();
		for (std::thread& t : threads)
		{
			t.join();
		}
	}
}

int runCli(LabelingKind kind, int argc, char* argv[])
{
	const char* prog = argc > 0 ? argv[0] : "labeling";

	std::string inputName;
	bool haveInput = false;
	int expectedOrder = 0;
	Options opts;
	int nThreads = static_cast<int>(std::thread::hardware_concurrency());
	if (nThreads < 1)
	{
		nThreads = 1;
	}

	for (int i = 1; i < argc; ++i)
	{
		const std::string arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			printUsage(prog, kind);
			return 0;
		}
		else if (arg == "-j")
		{
			if (i + 1 >= argc || !parsePositiveInt(argv[++i], nThreads))
			{
				std::fprintf(stderr, "%s: -j needs a positive number of threads\n", prog);
				return 2;
			}
		}
		else if (arg == "-a" || arg == "-c" || arg == "-v")
		{
			const Mode mode = arg == "-a" ? Mode::All : arg == "-c" ? Mode::Count : Mode::Without;
			if (opts.mode != Mode::First && opts.mode != mode)
			{
				std::fprintf(stderr, "%s: -a, -c and -v cannot be combined\n", prog);
				return 2;
			}
			opts.mode = mode;
		}
		else if (arg == "-g")
		{
			opts.graph6 = true;
		}
		else if (arg.size() > 1 && arg[0] == '-')
		{
			std::fprintf(stderr, "%s: unknown option %s\n\n", prog, arg.c_str());
			printUsage(prog, kind);
			return 2;
		}
		else if (!haveInput)
		{
			inputName = arg;
			haveInput = true;
		}
		else if (expectedOrder == 0)
		{
			if (!parsePositiveInt(arg, expectedOrder))
			{
				std::fprintf(stderr, "%s: n must be a positive integer, got %s\n", prog, arg.c_str());
				return 2;
			}
		}
		else
		{
			std::fprintf(stderr, "%s: unexpected argument %s\n\n", prog, arg.c_str());
			printUsage(prog, kind);
			return 2;
		}
	}

	if (!haveInput)
	{
		printUsage(prog, kind);
		return 2;
	}

	std::ifstream file;
	if (inputName != "-")
	{
		file.open(inputName);
		if (!file.is_open())
		{
			std::fprintf(stderr, "%s: cannot open %s\n", prog, inputName.c_str());
			return 1;
		}
	}
	GraphReader reader(inputName == "-" ? std::cin : file);

	unsigned long long nGraphs = 0;
	unsigned long long nFound = 0;
	unsigned long long nLabelings = 0;
	const size_t batchSize = opts.mode == Mode::All ? kListAllBatchSize : kBatchSize;
	std::vector<Graph> batch;
	std::vector<Result> results;

	try
	{
		bool more = true;
		while (more)
		{
			batch.clear();
			Graph g;
			while (batch.size() < batchSize && (more = reader.next(g)))
			{
				if (expectedOrder != 0 && g.n != expectedOrder)
				{
					throw std::runtime_error("graph " + std::to_string(nGraphs + batch.size() + 1) + " has " +
						std::to_string(g.n) + " vertices, expected " + std::to_string(expectedOrder));
				}
				batch.push_back(std::move(g));
			}

			solveBatch(kind, opts.mode, batch, results, nThreads);

			for (size_t i = 0; i < batch.size(); ++i)
			{
				if (printResult(kind, opts, batch[i], results[i]))
				{
					++nFound;
				}
				nLabelings += results[i].count;
			}
			std::fflush(stdout);
			nGraphs += batch.size();
		}
	}
	catch (const std::runtime_error& e)
	{
		std::fflush(stdout);
		std::fprintf(stderr, "%s: %s: %s\n", prog, inputName.c_str(), e.what());
		return 1;
	}

	switch (opts.mode)
	{
	case Mode::First:
		std::fprintf(stderr, "%llu of %llu graphs have a %s labeling\n", nFound, nGraphs, kindName(kind));
		break;
	case Mode::All:
	case Mode::Count:
		std::fprintf(stderr, "%llu of %llu graphs have a %s labeling (%llu labelings in total)\n",
			nFound, nGraphs, kindName(kind), nLabelings);
		break;
	case Mode::Without:
		std::fprintf(stderr, "%llu of %llu graphs have no %s labeling\n", nFound, nGraphs, kindName(kind));
		break;
	}
	return 0;
}
