#include <iostream>
#include "sabre_layout.h"
#include "DAG/dag.h"
#include "coupling.h"
#include "visualization.h"
#include "layout.h"

#include <boost/graph/graph_utility.hpp>
#include <iomanip>

DAGCircuit test_dag() {
    DagGraph test_graph;
    DAGCircuit dag_circuit{test_graph};

    dag_circuit.add_instruction_node_end(InstructionNode{"a", 0});
    dag_circuit.add_instruction_node_end(InstructionNode{"b", 1});
    dag_circuit.add_instruction_node_end(InstructionNode{"c", 1});

    // dag_circuit.draw_self();

    return dag_circuit;
}

CouplingCircuit test_c_ciruit() {
    CouplingList c_list = {
        {0, 1, 0.99},
        {2, 3, 0.67},
        {2, 4, 0.89},
        {1, 4, 0.89},
        {3, 4, 0.89},
    };
    CouplingCircuit c_circuit{c_list};  

    CouplingGraph graph;
    auto v1 = boost::add_vertex(CouplingNode{20}, graph);
    auto v2 = boost::add_vertex(CouplingNode{10}, graph);


    std::cout << "c_circuit num of qubits: " <<  c_circuit.num_qubits << std::endl;
    print_graph(c_circuit.graph);

    //c_circuit.draw_self();
    return c_circuit;
}

void test_sabre_routing() {
    CouplingCircuit c_circuit = test_c_ciruit();
    DAGCircuit dag = test_dag();

    // c_circuit.draw_self();
    SabreRouting sabre_routing{c_circuit};
    sabre_routing.run(dag);
}


void test_sabre_layout() {
    CouplingCircuit c_circuit = test_c_ciruit();
    SabreLayout sabre_layout{c_circuit};
    DAGCircuit dag = test_dag();
    Vis::draw_graph(dag.graph);
    sabre_layout.run(dag);

}


int main() {
    std::cout << "---- main function ----" << std::endl;
    // test_c_ciruit();
    // test_sabre_routing();
    // test_dag();
    test_sabre_layout();
}