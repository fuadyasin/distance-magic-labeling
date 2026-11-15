# Distance Magic Labeling

Exhaustive search for distance magic, closed distance magic and distance
antimagic labelings of small graphs.

A labeling of a graph on `n` vertices is a bijection from the vertices to
`{1, ..., n}`. The weight of a vertex is the sum of the labels in its
neighbourhood.

| Program | Labeling               | Condition                                        |
| ------- | ---------------------- | ------------------------------------------------ |
| `DML`   | distance magic         | all open-neighbourhood weights are equal         |
| `CDML`  | closed distance magic  | all closed-neighbourhood weights are equal (the vertex's own label is included) |
| `DAML`  | distance antimagic     | all open-neighbourhood weights are distinct      |

## Build

Requires CMake 3.10+ and a C++17 compiler.

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

## Usage

```
./DML [-j threads] <file | -> [n]
```

`CDML` and `DAML` take the same arguments.

- `file` holds the graphs to check; `-` reads standard input. Two formats are
  accepted:
  - [graph6](https://users.cecs.anu.edu.au/~bdm/data/formats.html), one graph
    per line, as produced by `geng` from [nauty](https://pallini.di.uniroma1.it/)
  - adjacency matrices as printed by `showg -a`
- `n` is optional. The order of each graph is read from the input; when `n` is
  given the program stops with an error if a graph has a different order.
- `-j` sets the number of worker threads (default: all cores).

For example, to check every connected graph on 8 vertices:

```
geng -c 8 | ./DML -
```

or, going through a file of adjacency matrices:

```
geng -c 8 graphs.g6
showg -a graphs.g6 graphs.txt
./DML graphs.txt
```

## Output

Each graph that has a labeling is printed to standard output, in input order,
with its adjacency matrix and the labeling found (`label[i]` is the label of
vertex `i`). `CDML` also prints the magic constant `k` first.

```
adjmat: 
0 1 0 1 
1 0 1 0 
0 1 0 1 
1 0 1 0 
label: [ 1 2 4 3 ]
```

A summary line (`3 of 64 graphs have a distance magic labeling`) goes to
standard error, so redirecting standard output captures only the results.

## Notes

- The labeling reported is the lexicographically smallest one.
- The search is a backtracking search that abandons a partial labeling as soon
  as the vertex weights can no longer come out equal (or distinct), so it does
  not enumerate all `n!` labelings. It is still exponential in the worst case.
- Graphs may have at most 64 vertices.
- `DML` never reports a graph with an isolated vertex.
