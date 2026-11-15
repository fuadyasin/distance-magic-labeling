#pragma once

#include "graph.h"

#include <functional>
#include <vector>

// A labeling is a bijection f : V -> {1, ..., n}. The weight of a vertex is
// the sum of the labels over its neighbourhood.
enum class LabelingKind
{
	DistanceMagic,       // all open-neighbourhood weights are equal
	ClosedDistanceMagic, // all closed-neighbourhood weights are equal
	DistanceAntimagic,   // all open-neighbourhood weights are distinct
};

const char* kindName(LabelingKind kind);

// Weight of every vertex under the given labeling.
std::vector<int> vertexWeights(const Graph& g, LabelingKind kind, const std::vector<int>& labels);

// True iff labels is a permutation of 1..n satisfying the given kind.
bool isValidLabeling(const Graph& g, LabelingKind kind, const std::vector<int>& labels);

// Exhaustively searches for a labeling. On success, labels[v] is the label of
// vertex v and the result is the lexicographically smallest valid labeling.
// Returns false if the graph admits none.
//
// A graph with an isolated vertex is never reported as distance magic (the
// weight of an isolated vertex is the empty sum).
bool findLabeling(const Graph& g, LabelingKind kind, std::vector<int>& labels);

// Called with each labeling found; return true to keep searching, false to stop.
using LabelingVisitor = std::function<bool(const std::vector<int>&)>;

// Passes every valid labeling to visit, in lexicographic order, until visit
// returns false. Returns the number of labelings passed to visit.
unsigned long long forEachLabeling(const Graph& g, LabelingKind kind, const LabelingVisitor& visit);

// Number of valid labelings of the graph.
unsigned long long countLabelings(const Graph& g, LabelingKind kind);
