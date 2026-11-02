# Recursive Graph Collections
It is a C++23 library that lets you create and manage collections of **recursive graphs** written as part of an Advanced C++ course at MIMUW.

Compatible with C and based on STL containers and safe usage of iterators and static variables.

## Authors

* **Maria Kaszewska**
* **Julia Szoszkiewicz**

---

## API

The library has a C-compatible interface. All functions validate their inputs safely by returning `-1`, `false`, or `nullptr` when given data is invalid.

### 1. Collection Management
* `long rgc_new_collection();` — Creates a new isolate collection and returns its unique ID.
* `void rgc_delete_collection(long id);` — Safely deletes the given collection and all graphs inside it, freeing the memory.
* `ssize_t rgc_size(long id);` — Returns the total number of graphs currently stored in the collection with given ID.

### 2. Graph Construction & Info
* `bool rgc_new_graph(long id, char const *name, int n);` — Creates a new graph named `name` with `n` isolated vertices (numbered 0 to n-1).
* `void rgc_delete_graph(long id, char const *name);`
* `ssize_t rgc_count_nodes(long id, char const *name);`
* `ssize_t rgc_count_edges(long id, char const *name);`
* `ssize_t rgc_graph_level(long id, char const *name);` — Returns the current maximum level of the graph.

### 3. Modifying Vertices & Edges
* `int rgc_graph_new_node(long id, char const *name);` — Adds a single, isolated vertex and returns its newly assigned integer ID - the lowest number which not exist in given graph.
* `bool rgc_graph_delete_node(long id, char const *name, int x);`
* `bool rgc_graph_add_edge(long id, char const *name, int x, int y);`
* `bool rgc_graph_delete_edge(long id, char const *name, int x, int y);`
* `ssize_t rgc_edge_level(long id, char const *name, int x, int y);`

### 4. Replacing Edge
- `bool rgc_replace_edge(long id, char const *result_name, char const *recipient_name, int w_1, int w_2, char const *donor_name, int v_1, int v_2);`  
  Creates a new graph (`result_name`) in following steps:
  ** Copies the `recipient_name` graph.
  ** Deletes the edge `{w_1, w_2}` from the new graph.
  ** Identifies vertices `v_1` as `w_1` and `v_2` as `w_2` and adds remaining donor vertices (assigning them the smallest available non-negative IDs).
  ** Adds all donor edges with a new level equal to `max(donor graph level, replaced edge level) + 1` (replaced_edge - edge {w_1, w_2}).

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
