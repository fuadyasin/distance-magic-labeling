#include "labeling.h"

#include <algorithm>
#include <climits>

const char* kindName(LabelingKind kind)
{
	switch (kind)
	{
	case LabelingKind::DistanceMagic: return "distance magic";
	case LabelingKind::ClosedDistanceMagic: return "closed distance magic";
	case LabelingKind::DistanceAntimagic: return "distance antimagic";
	}
	return "";
}

std::vector<int> vertexWeights(const Graph& g, LabelingKind kind, const std::vector<int>& labels)
{
	std::vector<int> w(g.n, 0);
	for (int i = 0; i < g.n; ++i)
	{
		if (kind == LabelingKind::ClosedDistanceMagic)
		{
			w[i] = labels[i];
		}
		for (int j = 0; j < g.n; ++j)
		{
			if (g.hasEdge(i, j))
			{
				w[i] += labels[j];
			}
		}
	}
	return w;
}

bool isValidLabeling(const Graph& g, LabelingKind kind, const std::vector<int>& labels)
{
	if (labels.size() != static_cast<size_t>(g.n))
	{
		return false;
	}
	std::vector<int> sorted = labels;
	std::sort(sorted.begin(), sorted.end());
	for (int i = 0; i < g.n; ++i)
	{
		if (sorted[i] != i + 1)
		{
			return false;
		}
	}

	std::vector<int> w = vertexWeights(g, kind, labels);
	std::sort(w.begin(), w.end());
	if (kind == LabelingKind::DistanceAntimagic)
	{
		return std::adjacent_find(w.begin(), w.end()) == w.end();
	}
	return w.empty() || w.front() == w.back();
}

namespace
{
	// Depth-first search that labels the vertices in index order, trying the
	// labels in increasing order, so the first labeling found is the
	// lexicographically smallest one. A branch is abandoned as soon as the
	// partial labeling can no longer be completed.
	class Search
	{
	public:
		Search(const Graph& g, LabelingKind kind)
			: kind_(kind), n_(g.n), nbhd_(g.adj), label_(g.n, 0), sum_(g.n, 0), open_(g.n, 0)
		{
			for (int i = 0; i < n_; ++i)
			{
				if (kind_ == LabelingKind::ClosedDistanceMagic)
				{
					nbhd_[i] |= std::uint64_t{1} << i;
				}
				for (int j = 0; j < n_; ++j)
				{
					open_[i] += static_cast<int>((nbhd_[i] >> j) & 1U);
				}
				unlabeled_ |= std::uint64_t{1} << i;
			}
		}

		bool run() { return place(0); }
		const std::vector<int>& labels() const { return label_; }

	private:
		bool place(int v)
		{
			if (v == n_)
			{
				return true;
			}
			for (int l = 1; l <= n_; ++l)
			{
				const std::uint64_t bit = std::uint64_t{1} << (l - 1);
				if (used_ & bit)
				{
					continue;
				}

				used_ |= bit;
				unlabeled_ &= ~(std::uint64_t{1} << v);
				label_[v] = l;
				update(v, l, -1);
				if (feasible() && place(v + 1))
				{
					return true;
				}
				update(v, -l, +1);
				unlabeled_ |= std::uint64_t{1} << v;
				used_ &= ~bit;
			}
			return false;
		}

		// Neighbourhoods are symmetric, so the vertices whose weight depends
		// on v are exactly the members of v's own neighbourhood.
		void update(int v, int labelDelta, int openDelta)
		{
			for (int i = 0; i < n_; ++i)
			{
				if ((nbhd_[v] >> i) & 1U)
				{
					sum_[i] += labelDelta;
					open_[i] += openDelta;
				}
			}
		}

		bool feasible() const
		{
			return kind_ == LabelingKind::DistanceAntimagic ? feasibleAntimagic() : feasibleMagic();
		}

		// minExtra[r] / maxExtra[r]: the sum of the r smallest / largest labels
		// that are still unused.
		void extremeSums(int* minExtra, int* maxExtra) const
		{
			minExtra[0] = maxExtra[0] = 0;
			int r = 0;
			for (int l = 1; l <= n_; ++l)
			{
				if (!((used_ >> (l - 1)) & 1U))
				{
					minExtra[r + 1] = minExtra[r] + l;
					++r;
				}
			}
			r = 0;
			for (int l = n_; l >= 1; --l)
			{
				if (!((used_ >> (l - 1)) & 1U))
				{
					maxExtra[r + 1] = maxExtra[r] + l;
					++r;
				}
			}
		}

		bool feasibleMagic() const
		{
			int minExtra[kMaxVertices + 1];
			int maxExtra[kMaxVertices + 1];
			extremeSums(minExtra, maxExtra);

			// Every vertex can still reach some interval of weights: its
			// partial sum plus the smallest/largest unused labels for its
			// unlabeled neighbours. A common weight needs all the intervals
			// to intersect.
			int lo = INT_MIN;
			int hi = INT_MAX;
			for (int i = 0; i < n_; ++i)
			{
				lo = std::max(lo, sum_[i] + minExtra[open_[i]]);
				hi = std::min(hi, sum_[i] + maxExtra[open_[i]]);
				if (lo > hi)
				{
					return false;
				}
			}

			// Two weights are equal iff the labels on either side of the
			// symmetric difference of the neighbourhoods have the same sum;
			// the shared neighbours cancel out. This is much sharper than the
			// intervals above when neighbourhoods overlap heavily.
			for (int u = 1; u < n_; ++u)
			{
				for (int v = 0; v < u; ++v)
				{
					const int diff = sum_[u] - sum_[v];
					const int onlyU = popCount(nbhd_[u] & ~nbhd_[v] & unlabeled_);
					const int onlyV = popCount(nbhd_[v] & ~nbhd_[u] & unlabeled_);
					if (diff + maxExtra[onlyU] - minExtra[onlyV] < 0 ||
						diff + minExtra[onlyU] - maxExtra[onlyV] > 0)
					{
						return false;
					}
				}
			}
			return true;
		}

		// Two weights are known to collide once the symmetric difference of
		// the neighbourhoods is fully labeled and both sides sum to the same.
		bool feasibleAntimagic() const
		{
			for (int u = 1; u < n_; ++u)
			{
				for (int v = 0; v < u; ++v)
				{
					if (sum_[u] == sum_[v] && ((nbhd_[u] ^ nbhd_[v]) & unlabeled_) == 0)
					{
						return false;
					}
				}
			}
			return true;
		}

		static int popCount(std::uint64_t x)
		{
			int count = 0;
			for (; x != 0; x &= x - 1)
			{
				++count;
			}
			return count;
		}

		LabelingKind kind_;
		int n_;
		std::vector<std::uint64_t> nbhd_; // the vertices that contribute to each weight
		std::vector<int> label_;
		std::vector<int> sum_;            // labels placed so far in each neighbourhood
		std::vector<int> open_;           // neighbourhood members still unlabeled
		std::uint64_t used_ = 0;          // bit l-1 is set once label l is taken
		std::uint64_t unlabeled_ = 0;     // the vertices without a label yet
	};
}

bool findLabeling(const Graph& g, LabelingKind kind, std::vector<int>& labels)
{
	if (g.n == 0)
	{
		return false;
	}

	if (kind == LabelingKind::DistanceMagic)
	{
		for (int i = 0; i < g.n; ++i)
		{
			if (g.adj[i] == 0)
			{
				return false;
			}
		}
	}
	else if (kind == LabelingKind::DistanceAntimagic)
	{
		// Two vertices with the same neighbourhood always get the same weight.
		std::vector<std::uint64_t> rows = g.adj;
		std::sort(rows.begin(), rows.end());
		if (std::adjacent_find(rows.begin(), rows.end()) != rows.end())
		{
			return false;
		}
	}

	Search search(g, kind);
	if (!search.run())
	{
		return false;
	}
	labels = search.labels();
	return true;
}
