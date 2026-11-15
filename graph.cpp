#include "graph.h"

#include <stdexcept>

namespace
{
	const std::string kGraph6Header = ">>graph6<<";

	bool isBlank(const std::string& s)
	{
		return s.find_first_not_of(" \t") == std::string::npos;
	}

	bool isMatrixHeader(const std::string& s)
	{
		return s.find("Graph") != std::string::npos;
	}
}

Graph parseGraph6(const std::string& text)
{
	size_t pos = 0;
	if (text.compare(0, kGraph6Header.size(), kGraph6Header) == 0)
	{
		pos = kGraph6Header.size();
	}
	if (pos >= text.size())
	{
		throw std::runtime_error("empty graph6 string");
	}
	if (text[pos] == ':' || text[pos] == '&')
	{
		throw std::runtime_error("sparse6/digraph6 input is not supported, use graph6");
	}

	auto sextet = [&](size_t i) -> unsigned
	{
		if (i >= text.size())
		{
			throw std::runtime_error("graph6 string is too short");
		}
		const int c = static_cast<unsigned char>(text[i]) - 63;
		if (c < 0 || c > 63)
		{
			throw std::runtime_error("invalid character in graph6 string");
		}
		return static_cast<unsigned>(c);
	};

	// Orders up to 62 take one byte; larger ones are prefixed with '~'.
	unsigned long n = sextet(pos++);
	if (n == 63)
	{
		if (sextet(pos) == 63)
		{
			throw std::runtime_error("graph is too large (at most " + std::to_string(kMaxVertices) + " vertices)");
		}
		n = (sextet(pos) << 12) | (sextet(pos + 1) << 6) | sextet(pos + 2);
		pos += 3;
	}
	if (n > static_cast<unsigned long>(kMaxVertices))
	{
		throw std::runtime_error("graph is too large (at most " + std::to_string(kMaxVertices) + " vertices)");
	}

	Graph g;
	g.n = static_cast<int>(n);
	g.adj.assign(g.n, 0);

	// The upper triangle follows column by column, six bits per character.
	const size_t nBits = static_cast<size_t>(g.n) * (g.n - 1) / 2;
	if (text.size() - pos != (nBits + 5) / 6)
	{
		throw std::runtime_error("graph6 string has the wrong length for " + std::to_string(g.n) + " vertices");
	}

	size_t bit = 0;
	unsigned cur = 0;
	for (int j = 1; j < g.n; ++j)
	{
		for (int i = 0; i < j; ++i, ++bit)
		{
			if (bit % 6 == 0)
			{
				cur = sextet(pos++);
			}
			if ((cur >> (5 - bit % 6)) & 1U)
			{
				g.adj[i] |= std::uint64_t{1} << j;
				g.adj[j] |= std::uint64_t{1} << i;
			}
		}
	}
	return g;
}

bool GraphReader::getLine(std::string& line)
{
	if (hasPending_)
	{
		line = std::move(pending_);
		hasPending_ = false;
		return true;
	}
	if (!std::getline(in_, line))
	{
		return false;
	}
	++lineNo_;
	if (!line.empty() && line.back() == '\r')
	{
		line.pop_back();
	}
	return true;
}

Graph GraphReader::readMatrix()
{
	std::vector<std::string> rows;
	std::string line;
	while (getLine(line))
	{
		if (isBlank(line))
		{
			break;
		}
		if (isMatrixHeader(line))
		{
			// The next graph started without a separating blank line.
			pending_ = std::move(line);
			hasPending_ = true;
			break;
		}

		std::string row;
		for (char c : line)
		{
			if (c == '0' || c == '1')
			{
				row.push_back(c);
			}
			else if (c != ' ' && c != '\t')
			{
				throw std::runtime_error("adjacency matrix rows may only contain 0 and 1");
			}
		}
		rows.push_back(std::move(row));
	}

	if (rows.empty())
	{
		throw std::runtime_error("adjacency matrix has no rows");
	}
	if (rows.size() > static_cast<size_t>(kMaxVertices))
	{
		throw std::runtime_error("graph is too large (at most " + std::to_string(kMaxVertices) + " vertices)");
	}

	Graph g;
	g.n = static_cast<int>(rows.size());
	g.adj.assign(g.n, 0);
	for (int i = 0; i < g.n; ++i)
	{
		if (rows[i].size() != rows.size())
		{
			throw std::runtime_error("adjacency matrix is not square (" + std::to_string(rows.size()) +
				" rows, but row " + std::to_string(i + 1) + " has " + std::to_string(rows[i].size()) + " entries)");
		}
		for (int j = 0; j < g.n; ++j)
		{
			if (rows[i][j] == '1')
			{
				g.adj[i] |= std::uint64_t{1} << j;
			}
		}
	}
	for (int i = 0; i < g.n; ++i)
	{
		if (g.hasEdge(i, i))
		{
			throw std::runtime_error("adjacency matrix has a loop at vertex " + std::to_string(i + 1));
		}
		for (int j = 0; j < i; ++j)
		{
			if (g.hasEdge(i, j) != g.hasEdge(j, i))
			{
				throw std::runtime_error("adjacency matrix is not symmetric");
			}
		}
	}
	return g;
}

bool GraphReader::next(Graph& g)
{
	std::string line;
	try
	{
		while (getLine(line))
		{
			if (isBlank(line))
			{
				continue;
			}
			g = isMatrixHeader(line) ? readMatrix() : parseGraph6(line);
			return true;
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error("line " + std::to_string(lineNo_) + ": " + e.what());
	}
	return false;
}
