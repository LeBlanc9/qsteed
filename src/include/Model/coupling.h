#pragma once
#include <vector>
#include <tuple>
#include <memory>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>

using CouplingList = std::vector<std::tuple<int, int, float>>;
const CouplingList EMPTY_COUPLING_LIST = {std::make_tuple(0, 0, 0)};

using Matrix = std::vector<std::vector<int>>;

struct CouplingNode {
    int id;
};
struct CouplingEdge {
    double fidelity;
};

using CouplingGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, CouplingNode, CouplingEdge>;


class CouplingCircuit {
public:     
    CouplingGraph graph;
    CouplingList c_list;
    bool is_bidirectional = true;    
    unsigned int num_qubits = 0;


public:
    CouplingCircuit(CouplingList c_list);


    void update_num_qubits() { 
        this->num_qubits = boost::num_vertices(graph); 
    }

    Matrix get_distance_matrix() const {
        Matrix distance_matrix(boost::num_vertices(graph), std::vector<int>(boost::num_vertices(graph)));
        boost::constant_property_map<CouplingGraph::edge_descriptor, int> weight_map(1);

        bool success = boost::floyd_warshall_all_pairs_shortest_paths(graph, distance_matrix, boost::weight_map(weight_map));
        return distance_matrix;
    }


};