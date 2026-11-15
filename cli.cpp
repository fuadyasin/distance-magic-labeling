#include "cli.h"

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
	// memory use stays flat however long the input is.
	constexpr size_t kBatchSize = 4096;

	void printUsage(const char* prog, LabelingKind kind)
	{
		std::fprintf(stderr,
			"Usage: %s [-j threads] <file | -> [n]\n"
			"\n"
			"Searches every graph in the input for a %s labeling and prints\n"
			"the graphs that have one, together with the labeling found.\n"
			"\n"
			"  file         graphs in graph6 format (geng output) or as adjacency\n"
			"               matrices (showg -a output); use - for standard input\n"
			"  n            optional: fail unless every graph has exactly n vertices\n"
			"  -j threads   number of worker threads (default: all cores)\n",
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

	void printResult(LabelingKind kind, const Graph& g, const std::vector<int>& labels)
	{
		if (kind == LabelingKind::ClosedDistanceMagic)
		{
			std::printf("k: %f\n", static_cast<double>(vertexWeights(g, kind, labels)[0]));
		}

		std::printf("adjmat: \n");
		for (int i = 0; i < g.n; ++i)
		{
			for (int j = 0; j < g.n; ++j)
			{
				std::printf("%d ", g.hasEdge(i, j) ? 1 : 0);
			}
			std::printf("\n");
		}

		std::printf("label: [ ");
		for (int label : labels)
		{
			std::printf("%d ", label);
		}
		std::printf("]\n");
	}

	struct Result
	{
		bool found = false;
		std::vector<int> labels;
	};

	void solveBatch(LabelingKind kind, const std::vector<Graph>& graphs, std::vector<Result>& results, int nThreads)
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
				results[i].found = findLabeling(graphs[i], kind, results[i].labels);
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
	std::vector<Graph> batch;
	std::vector<Result> results;

	try
	{
		bool more = true;
		while (more)
		{
			batch.clear();
			Graph g;
			while (batch.size() < kBatchSize && (more = reader.next(g)))
			{
				if (expectedOrder != 0 && g.n != expectedOrder)
				{
					throw std::runtime_error("graph " + std::to_string(nGraphs + batch.size() + 1) + " has " +
						std::to_string(g.n) + " vertices, expected " + std::to_string(expectedOrder));
				}
				batch.push_back(std::move(g));
			}

			solveBatch(kind, batch, results, nThreads);

			for (size_t i = 0; i < batch.size(); ++i)
			{
				if (results[i].found)
				{
					printResult(kind, batch[i], results[i].labels);
					++nFound;
				}
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

	std::fprintf(stderr, "%llu of %llu graphs have a %s labeling\n", nFound, nGraphs, kindName(kind));
	return 0;
}
