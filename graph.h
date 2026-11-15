#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

// Largest supported order: each adjacency row is stored in one 64-bit word.
constexpr int kMaxVertices = 64;

// Simple undirected graph. Bit j of adj[i] is set iff i and j are adjacent.
struct Graph
{
	int n = 0;
	std::vector<std::uint64_t> adj;

	bool hasEdge(int i, int j) const { return (adj[i] >> j) & 1U; }
};

// Decodes one graph6 line (an optional ">>graph6<<" header is accepted).
// Throws std::runtime_error on malformed input.
Graph parseGraph6(const std::string& line);

// Streams graphs from text input, one at a time. Two formats are recognised
// and may be mixed freely:
//   * graph6, one graph per line (the output of nauty's geng)
//   * adjacency matrices as printed by `showg -a`: a line containing "Graph",
//     followed by n rows of n 0/1 digits, terminated by a blank line
class GraphReader
{
public:
	explicit GraphReader(std::istream& in) : in_(in) {}

	// Reads the next graph into g. Returns false at end of input.
	// Throws std::runtime_error (with the line number) on malformed input.
	bool next(Graph& g);

private:
	bool getLine(std::string& line);
	Graph readMatrix();

	std::istream& in_;
	long lineNo_ = 0;
	std::string pending_;
	bool hasPending_ = false;
};
