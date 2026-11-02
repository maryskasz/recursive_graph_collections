#include "recursive_graph_collections.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef LEVEL_MAX
#define LEVEL_MAX 4
#endif

namespace {
	// Data structure for recursive graph collections.

	// Graph nodes are represented as integers. Free node numbers will also be
	// put in a set: when a node is deleted it will be placed in the free_nodes
	// set.
	using Nodes_Set = std::set<int>;

	// Map with graph edges as keys (contains both edge {x, y} and its inverted
	// edge {y, x}) and edge levels as values. This means that upon edge
	// insertion, deletion and level change both have to be modified and that
	// their levels must be equal at all times since they represent the same
	// edge.
	using Edge_Map = std::map<std::pair<int, int>, ssize_t>;

	// Map with current (existing in the graph) edge levels as keys and the
	// number of edges of that level as values. These are only entered once for
	// each edge, so for example upon inserting an edge {x, y} of level 0 (and
	// consequently {y, x} of level 0) the value for the key 0 is incremented
	// once.
	using Levels_Map = std::map<ssize_t, size_t>;

	// Graph data holds all graph information. The first Nodes_Set contains
	// a graph's currently used nodes, and the second Nodes_Set contains a
	// graph's deleted nodes, so that the sum of the two nodes sets is {0, ...,
	// N-1} where N-1 is the largest node number to have ever been in the graph.
	using Graph_Data = std::tuple<Nodes_Set, Edge_Map, Levels_Map, Nodes_Set>;

	// Map of graph names as keys and graph data as values.
	using Graph_Collection = std::map<std::string, Graph_Data, std::less<>>;

	// Map of ids as keys and graph collections as values.
	using RGC = std::unordered_map<long, Graph_Collection>;

	inline RGC &rgc() {
		static RGC rgc;
		return rgc;
	}

	constexpr ssize_t LEVEL_LIMIT = LEVEL_MAX;

	long LAST_ID = 0;

	// Helper functions.

	// Returns an iterator to the collection with the given id
	// or rgc().end() if there is no such collection.
	RGC::iterator get_collection(long id) {
		return rgc().find(id);
	}

	// Returns an iterator to the graph with the given name in the collection
	// or collection.end() if there is no such graph.
	Graph_Collection::iterator get_graph(Graph_Collection &collection,
										 char const *name) {
		if (!name) {
			return collection.end();
		}

		return collection.find(name);
	}

	// Checks whether a graph with the given name exists in the collection with
	// the given id. If so, returns true and sets graph_it to that graph.
	bool check_if_graph_exists(long id, char const *name,
							   Graph_Collection::iterator &graph_it) {
		auto collection_it = get_collection(id);
		if (collection_it == rgc().end()) {
			return false;
		}

		auto &collection = collection_it->second;
		auto found_it = get_graph(collection, name);
		if (found_it == collection.end()) {
			return false;
		}

		graph_it = found_it;
		return true;
	}

	bool is_valid_char(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			   (c >= '0' && c <= '9') || c == '_';
	}

	bool is_name_valid(char const *name) {
		if (!name || *name == '\0') {
			return false;
		}

		for (char const *c = name; *c != '\0'; ++c) {
			if (!is_valid_char(*c)) {
				return false;
			}
		}

		return true;
	}

	// Checks whether a graph with the given name in a graph collection with
	// the given id contains edge {x, y}. This function should only be called
	// after checking if such a graph exists in this collection.
	bool has_edge(Graph_Data const &graph, int x, int y) {
		auto const &edges = std::get<1>(graph);
		return edges.find({x, y}) != edges.end();
	}

	// Checks whether a graph with the given name in a graph collection with
	// the given id contains node v. This function should only be called after
	// checking if such a graph exists in this collection.
	bool has_node(Graph_Data const &graph, int v) {
		auto const &nodes = std::get<0>(graph);
		return nodes.find(v) != nodes.end();
	}

	// When deleting an edge (of level L) we have to delete the inverted edge as
	// well and decrement the number of edges of level L.
	void delete_edge_and_decrement_levels(Edge_Map &edges, Levels_Map &levels,
										  Edge_Map::iterator &edge_it,
										  Edge_Map::iterator &inv_edge_it) {
		auto edge_level = edge_it->second;

		auto level_it = levels.find(edge_level);
		auto &number_of_edges_of_level = level_it->second;

		// number_of_edges_of_level can't reach 0 since levels at level_it
		// should have been erased earlier.
		assert(number_of_edges_of_level > 0);

		--number_of_edges_of_level;
		if (number_of_edges_of_level == 0) {
			levels.erase(level_it);
		}

		edges.erase(edge_it);
		edges.erase(inv_edge_it);
	}

	// Checks input validity in rgc_replace_edge(). On success, recipient_it and
	// donor_it point to the recipient and donor graphs in the collection.
	bool check_input_for_replace_edge(
		Graph_Collection &collection, char const *result_name,
		char const *recipient_name, char const *donor_name, int v_1, int v_2,
		int w_1, int w_2, Graph_Collection::iterator &recipient_final_it,
		Graph_Collection::iterator &donor_final_it,
		Edge_Map::iterator &edge_final_it) {
		if (!is_name_valid(result_name) ||
			get_graph(collection, result_name) != collection.end()) {
			return false;
		}

		auto recipient_it = get_graph(collection, recipient_name);
		auto donor_it = get_graph(collection, donor_name);

		if (recipient_it == collection.end() || donor_it == collection.end()) {
			return false;
		}

		if (w_1 == w_2 || v_1 == v_2 || !has_node(donor_it->second, v_1) ||
			!has_node(donor_it->second, v_2)) {
			return false;
		}

		auto &edges = std::get<1>(recipient_it->second);
		auto edge_it = edges.find({w_1, w_2});

		if (edge_it == edges.end()) {
			return false;
		}

		edge_final_it = edge_it;
		donor_final_it = donor_it;
		recipient_final_it = recipient_it;
		return true;
	}

	// Checks whether the result graph will stay within level and node count
	// limits. If so, returns the level of the result graph, else -1.
	ssize_t check_nodes_and_find_level(Graph_Data const &recipient_graph,
									   Graph_Data const &donor_graph,
									   Edge_Map::iterator const &edge_it) {
		// Checking if the number of nodes in result_graph is less than the
		// limit.
		auto const &donor_nodes = std::get<0>(donor_graph);
		auto const &recipient_nodes = std::get<0>(recipient_graph);

		size_t donor_size = donor_nodes.size();
		size_t recipient_size = recipient_nodes.size();

		// recipient_size + donor_size - 2 > INT_MAX
		if (recipient_size - 2 > static_cast<size_t>(INT_MAX) - donor_size) {
			return -1;
		}

		// Calculating maximum edge level.
		ssize_t edge_max_level = edge_it->second;
		ssize_t donor_max_level = 0;

		auto const &donor_levels = std::get<2>(donor_graph);
		auto const &donor_level_it = donor_levels.rbegin();
		if (donor_level_it != donor_levels.rend()) {
			donor_max_level = donor_level_it->first;
		}

		ssize_t max_level = std::max(edge_max_level, donor_max_level);

		// If donor_graph has no edges (thus has no levels) then result_graph's
		// max_level will not be incremented by 1.
		if (max_level >= LEVEL_LIMIT - 1 && !donor_levels.empty()) {
			return -1;
		}

		return max_level + 1;
	}

	// Assigns each node from the donor graph its new number in the result
	// graph. This function is called after having copied the recipient graph
	// (and its nodes) to the result graph.
	void create_node_translator(std::unordered_map<int, int> &node_translator,
								Nodes_Set const &result_nodes,
								Nodes_Set &result_free_nodes,
								Nodes_Set const &donor_nodes, int v_1, int v_2,
								int w_1, int w_2) {
		node_translator[v_1] = w_1;
		node_translator[v_2] = w_2;

		// Next number which isn't in result_nodes or result_free_nodes.
		int next_new =
			static_cast<int>(result_nodes.size() + result_free_nodes.size());
		auto free_nodes_it = result_free_nodes.begin();

		for (auto node : donor_nodes) {
			if (node == v_1 || node == v_2) {
				continue;
			}

			if (free_nodes_it == result_free_nodes.end()) {
				node_translator[node] = next_new;
				++next_new;
			}
			else {
				node_translator[node] = *free_nodes_it;
				++free_nodes_it;
			}
		}

		result_free_nodes.erase(result_free_nodes.begin(), free_nodes_it);
	}

	// Constructs result graph by inserting nodes and edges corresponding to
	// donor nodes and edges. Result graph contains copied recipient graph data
	// without the {w_1, w_2} edge.
	void add_nodes_from_donor(Graph_Data &result_graph,
							  Graph_Data const &donor_graph, int v_1, int v_2,
							  int w_1, int w_2, ssize_t new_level) {
		auto &result_nodes = std::get<0>(result_graph);
		auto &result_edges = std::get<1>(result_graph);
		auto &result_levels = std::get<2>(result_graph);
		auto &result_free_nodes = std::get<3>(result_graph);

		auto const &donor_nodes = std::get<0>(donor_graph);
		auto const &donor_edges = std::get<1>(donor_graph);

		std::unordered_map<int, int> node_translator;
		create_node_translator(node_translator, result_nodes, result_free_nodes,
							   donor_nodes, v_1, v_2, w_1, w_2);

		for (auto node : donor_nodes) {
			result_nodes.insert(node_translator.at(node));
		}

		for (auto it = donor_edges.begin(); it != donor_edges.end(); ++it) {
			auto old_edge = it->first;
			auto new_edge = std::make_pair(node_translator.at(old_edge.first),
										   node_translator.at(old_edge.second));

			result_edges[new_edge] = new_level;

			// Level of the edge {x, y} is added only if x < y to ensure that
			// the edge level is added only once.
			if (new_edge.first < new_edge.second) {
				++result_levels[new_level];
			}
		}
	}
} // namespace

namespace cxx {
	long rgc_new_collection() {
		if (LAST_ID == LONG_MAX) {
			return -1;
		}
		long id = LAST_ID;
		rgc()[id];

		++LAST_ID;
		return id;
	}

	void rgc_delete_collection(long id) {
		rgc().erase(id);
	}

	bool rgc_new_graph(long id, char const *name, int n) {
		if (n < 0 || !is_name_valid(name)) {
			return false;
		}

		auto collection_it = get_collection(id);
		if (collection_it == rgc().end()) {
			return false;
		}
		auto &collection = collection_it->second;

		if (get_graph(collection, name) != collection.end()) {
			return false;
		}

		Nodes_Set nodes;
		for (int i = 0; i < n; ++i) {
			nodes.insert(nodes.end(), i);
		}

		Edge_Map edges;
		Levels_Map levels;
		Nodes_Set free_nodes;

		collection.emplace(
			name, std::make_tuple(std::move(nodes), std::move(edges),
								  std::move(levels), std::move(free_nodes)));
		return true;
	}

	void rgc_delete_graph(long id, char const *name) {
		auto collection_it = get_collection(id);

		if (collection_it != rgc().end() && name) {
			collection_it->second.erase(name);
		}
	}

	int rgc_graph_new_node(long id, char const *name) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto &graph = graph_it->second;
		auto &nodes = std::get<0>(graph);

		if (nodes.size() >= INT_MAX) {
			return -1;
		}

		auto &free_nodes = std::get<3>(graph);

		int new_node = 0;
		if (free_nodes.empty()) {
			new_node = nodes.size();
			nodes.insert(nodes.end(), new_node); // <- zmiana
		}
		else {
			auto new_node_it = free_nodes.begin();
			new_node = *new_node_it;
			nodes.insert(new_node);
			free_nodes.erase(new_node_it);
		}
		// nodes.insert(new_node);
		return new_node;
	}

	bool rgc_graph_delete_node(long id, char const *name, int x) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return false;
		}

		auto &graph = graph_it->second;
		auto &nodes = std::get<0>(graph);
		auto &edges = std::get<1>(graph);
		auto &levels = std::get<2>(graph);
		auto &free_nodes = std::get<3>(graph);

		auto x_it = nodes.find(x);
		if (x_it == nodes.end()) {
			return false;
		}
		nodes.erase(x_it);

		// Node number x is free, so it goes to the set of available nodes.
		free_nodes.insert(x);

		// Erasing edges adjacent to x.
		std::vector<int> nodes_adjacent_to_x;
		auto first_edge_from_x_it = edges.lower_bound({x, 0});

		while (first_edge_from_x_it != edges.end() &&
			   (first_edge_from_x_it->first).first == x) {
			nodes_adjacent_to_x.push_back((first_edge_from_x_it->first).second);
			++first_edge_from_x_it;
		}

		for (auto y : nodes_adjacent_to_x) {
			auto edge_it = edges.find({x, y});
			auto inv_edge_it = edges.find({y, x});
			delete_edge_and_decrement_levels(edges, levels, edge_it,
											 inv_edge_it);
		}

		return true;
	}

	bool rgc_graph_add_edge(long id, char const *name, int x, int y) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return false;
		}

		auto &graph = graph_it->second;

		if (x == y || !has_node(graph, x) || !has_node(graph, y) ||
			has_edge(graph, x, y)) {
			return false;
		}

		auto &edges = std::get<1>(graph);
		auto &levels = std::get<2>(graph);

		std::pair<int, int> edge = {x, y};
		std::pair<int, int> inverted_edge = {y, x};
		ssize_t level_of_new_edge = 0;

		edges[edge] = level_of_new_edge;
		edges[inverted_edge] = level_of_new_edge;

		++levels[level_of_new_edge];

		return true;
	}

	bool rgc_graph_delete_edge(long id, char const *name, int x, int y) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return false;
		}

		auto &graph = graph_it->second;
		auto &edges = std::get<1>(graph);
		auto &levels = std::get<2>(graph);

		auto edge_it = edges.find({x, y});

		// Check if edge {x, y} exists.
		if (edge_it == edges.end()) {
			return false;
		}

		auto inv_edge_it = edges.find({y, x});
		delete_edge_and_decrement_levels(edges, levels, edge_it, inv_edge_it);

		return true;
	}

	char const *rgc_first_graph(long id) {
		auto collection_it = get_collection(id);
		if (collection_it == rgc().end()) {
			return nullptr;
		}
		auto &collection = collection_it->second;

		if (collection.empty()) {
			return nullptr;
		}
		auto const &graph_it = collection.begin();

		return graph_it->first.c_str();
	}

	char const *rgc_next_graph(long id, char const *name) {
		auto collection_it = get_collection(id);
		if (collection_it == rgc().end() || !name) {
			return nullptr;
		}
		auto &collection = collection_it->second;

		auto graph_it = collection.upper_bound(name);

		if (graph_it == collection.end()) {
			return nullptr;
		}

		return graph_it->first.c_str();
	}

	int rgc_first_node(long id, char const *name) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &nodes = std::get<0>(graph_it->second);

		if (nodes.empty()) {
			return -1;
		}

		// Node with the lowest number.
		return *nodes.begin();
	}

	int rgc_next_node(long id, char const *name, int x) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &nodes = std::get<0>(graph_it->second);
		if (nodes.empty()) {
			return -1;
		}

		auto greater_than_x_it = nodes.upper_bound(x);

		// There are no greater elements than x.
		if (greater_than_x_it == nodes.end()) {
			return -1;
		}

		return *greater_than_x_it;
	}

	int rgc_first_edge(long id, char const *name, int x) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &edges = std::get<1>(graph_it->second);
		if (edges.empty()) {
			return -1;
		}

		auto first_edge_from_x_it = edges.lower_bound({x, 0});

		// There are no edges starting from x.
		if (first_edge_from_x_it == edges.end() ||
			(first_edge_from_x_it->first).first > x) {
			return -1;
		}

		return (first_edge_from_x_it->first).second;
	}

	int rgc_next_edge(long id, char const *name, int x, int y) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &edges = std::get<1>(graph_it->second);
		if (edges.empty()) {
			return -1;
		}

		auto greater_edge_it = edges.upper_bound({x, y});

		// There are no edges {x, y'} starting from x where y' > y.
		if (greater_edge_it == edges.end() ||
			(greater_edge_it->first).first > x) {
			return -1;
		}

		return (greater_edge_it->first).second;
	}

	bool rgc_replace_edge(long id, char const *result_name,
						  char const *recipient_name, int w_1, int w_2,
						  char const *donor_name, int v_1, int v_2) {
		auto collection_it = get_collection(id);
		if (collection_it == rgc().end()) {
			return false;
		}
		auto &collection = collection_it->second;

		auto donor_it = collection.end();
		auto recipient_it = collection.end();

		// If edge {w_1, w_2} exists in the recipient after checking other input
		// components edge_it will be set to valid direction.
		Edge_Map::iterator replaced_edge_it;

		// Checking if input is valid.
		if (!check_input_for_replace_edge(
				collection, result_name, recipient_name, donor_name, v_1, v_2,
				w_1, w_2, recipient_it, donor_it, replaced_edge_it)) {
			return false;
		}

		auto &donor_graph = donor_it->second;
		auto &recipient_graph = recipient_it->second;

		// Checking if the result graph's level and node count stay within
		// limits.
		ssize_t new_level = check_nodes_and_find_level(
			recipient_graph, donor_graph, replaced_edge_it);
		if (new_level == -1) {
			return false;
		}

		// Making result graph as copy of the recipient and adding result to the
		// collection.
		auto result_graph = recipient_graph;

		// Deleting {w_1, w_2} edge from result_graph.
		auto &result_edges = std::get<1>(result_graph);
		auto &result_level = std::get<2>(result_graph);
		auto edge_it = result_edges.find({w_1, w_2});
		auto inv_edge_it = result_edges.find({w_2, w_1});

		delete_edge_and_decrement_levels(result_edges, result_level, edge_it,
										 inv_edge_it);

		// Adding updated nodes and edges from donor_graph to result_graph.
		add_nodes_from_donor(result_graph, donor_graph, v_1, v_2, w_1, w_2,
							 new_level);

		// Saving the result graph to the collection.
		collection.emplace(result_name, std::move(result_graph));

		return true;
	}

	ssize_t rgc_edge_level(long id, char const *name, int x, int y) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &edges = std::get<1>(graph_it->second);

		if (edges.empty()) {
			return -1;
		}

		auto edge_it = edges.find({x, y});

		if (edge_it == edges.end()) {
			return -1;
		}

		return edge_it->second;
	}

	ssize_t rgc_graph_level(long id, char const *name) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &levels = std::get<2>(graph_it->second);

		// Graph has no edges.
		if (levels.empty()) {
			return 0;
		}

		return levels.rbegin()->first;
	}

	ssize_t rgc_max_level() {
		return LEVEL_LIMIT - 1;
	}

	ssize_t rgc_count_nodes(long id, char const *name) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &nodes = std::get<0>(graph_it->second);

		return nodes.size();
	}

	ssize_t rgc_count_edges(long id, char const *name) {
		Graph_Collection::iterator graph_it;
		if (!check_if_graph_exists(id, name, graph_it)) {
			return -1;
		}

		auto const &edges = std::get<1>(graph_it->second);

		return edges.size() / 2;
	}

	ssize_t rgc_size(long id) {
		auto collection_it = get_collection(id);
		if (collection_it == rgc().end()) {
			return -1;
		}

		return collection_it->second.size();
	}
} // namespace cxx
