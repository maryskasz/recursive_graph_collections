# Recursive Graph Collections

## What is this?
This is a C++23 library that lets you create and manage collections of **recursive graphs** written as part of an Advanced C++ course at MIMUW.

Compatible with C and based on STL containers and safe usage of iterators and static variables.

## Authors

* **Maria Kaszewska**
* **Julia Szoszkiewicz**

---

## Storage & Data Structures

The library is build using advanced C++ STL containers (such as nested structures combining `std::unordered_map`, `std::map`, and `std::set`) to manage data efficiently:
- **Collections:** Each collection is referenced by a unique `long` identifier (ranging from `0` to `LONG_MAX`). 
- **Named Graphs:** Graphs within a collection are uniquely identified by a name—a non-empty string consisting of English letters (upper and lower case), digits, and underscores (`_`).
- **Lexicographical Order:** Graphs inside a collection are automatically sorted lexicographically by their names, induced by standard ASCII character values.
- **Vertices & Edges:** Vertices are represented by non-negative integers. Edges are undirected pairs of distinct vertices `{x, y}`, each associated with an integer **level** ranging from `0` to `LEVEL_MAX - 1` (where `LEVEL_MAX` defaults to 4, configurable via compiler flags).
- **Graph Level:** The level of a graph is defined as the maximum level among all its edges, or `0` if the graph contains no edges.
---

### API

The library has a C-compatible interface. All functions validate their inputs safely by returning `-1`, `false`, or `nullptr` when given data is invalid.

### 1. Collections
- `long rgc_new_collection();` — Creates a new collection and returns its unique ID.
- `void rgc_delete_collection(long id);` — Deletes the given collection and all graphs inside it, freeing the memory.
- `ssize_t rgc_size(long id);` — Returns the total number of graphs currently stored in the collection with the given ID.

### 2. Graph Construction & Info
- `bool rgc_new_graph(long id, char const *name, int n);` — Creates a new graph named `name` with `n` isolated vertices (numbered 0 to n-1).
- `void rgc_delete_graph(long id, char const *name);` — Deletes the graph with the given name from the collection.
- `ssize_t rgc_count_nodes(long id, char const *name);` — Returns the number of vertices in the specified graph.
- `ssize_t rgc_count_edges(long id, char const *name);` — Returns the number of edges in the specified graph.
- `ssize_t rgc_graph_level(long id, char const *name);` — Returns the current maximum level of the graph.
- `ssize_t rgc_max_level();` — Returns the maximum allowed edge level (`LEVEL_MAX - 1`).

### 3. Graph Iterators
- `char const * rgc_first_graph(long id);` — Returns a pointer to the name of the first graph in the collection.
- `char const * rgc_next_graph(long id, char const *name);` — Returns a pointer to the name of the graph following `name` lexicographically.
- `int rgc_first_node(long id, char const *name);` — Returns the smallest vertex number in the graph.
- `int rgc_next_node(long id, char const *name, int x);` — Returns the smallest vertex number greater than `x` in the graph.
- `int rgc_first_edge(long id, char const *name, int x);` — Returns the smallest neighbor of vertex `x`.
- `int rgc_next_edge(long id, char const *name, int x, int y);` — Returns the smallest neighbor of vertex `x` that is greater than `y`.

### 4. Modifying Vertices & Edges
- `int rgc_graph_new_node(long id, char const *name);` — Adds a single, isolated vertex and returns its newly assigned integer ID (the lowest non-negative number that does not exist in the graph).
- `bool rgc_graph_delete_node(long id, char const *name, int x);` — Deletes vertex `x` and all incident edges.
- `bool rgc_graph_add_edge(long id, char const *name, int x, int y);` — Adds an edge between distinct vertices `x` and `y` at level `0`.
- `bool rgc_graph_delete_edge(long id, char const *name, int x, int y);` — Deletes the edge between `x` and `y`.
- `ssize_t rgc_edge_level(long id, char const *name, int x, int y);` — Returns the level of the edge between `x` and `y`.

### 5. Replacing Edge (Recursive Construction)
- `bool rgc_replace_edge(long id, char const *result_name, char const *recipient_name, int w_1, int w_2, char const *donor_name, int v_1, int v_2);`  
  Creates a new graph (`result_name`) in four steps:
  1. Copies the `recipient_name` graph.
  2. Deletes the edge `{w_1, w_2}` from the new graph.
  3. Identifies vertices `v_1` (as `w_1`) and `v_2` (as `w_2`) and adds remaining donor vertices (assigning them the smallest available non-negative IDs).
  4. Adds all donor edges with a new level equal to `max(donor graph level, replaced edge level) + 1`.

---

## How to Compile & Link

You only need two files: `recursive_graph_collections.h` and `recursive_graph_collections.cpp`.

**1. Compile the library module:**
*(By default, `LEVEL_MAX` is 4. You can change it by using the `-D` compiler flag).*
```bash
g++ -c -Wall -Wextra -O2 -std=c++23 -DLEVEL_MAX=16 recursive_graph_collections.cpp -o rgc.o
# If your main program is written in C:
gcc -c -Wall -Wextra -O2 -std=c23 main.c -o main.o
g++ main.o rgc.o -o my_app

# If your main program is written in C++:
g++ -c -Wall -Wextra -O2 -std=c++23 main.cpp -o main.o
g++ main.o rgc.o -o my_app
