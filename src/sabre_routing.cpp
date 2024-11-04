#include <sabre_routing.h>
#include <iostream>
#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>
#include <vector>
#include <algorithm>
#include "layout.h"
#include "sabre_routing.h"

/*
    Return a @c DAGCircuit : The original dag or the mapped_dag with added swap gate depending on modify_flag.
*/
DAGCircuit SabreRouting::run(const DAGCircuit& dag) {
    // Precheck
    std::set<int> qubits_used = dag.get_qubits_used();
    if (qubits_used.size() == 1) { 
        std::cerr << "Warning: single qubit circuit no need optimize." << std::endl;
        return dag; 
    }

    if (qubits_used.size() > this->c_circuit.num_qubits) {
        throw std::runtime_error("More virtual qubits than physical qubits.");
    }

    for(const auto& qubit : qubits_used)
        this->qubits_decay[qubit] = 1;
    this->extended_set_size = this->c_circuit.num_qubits;


    // Parameter preparation before iteration.
    DAGCircuit mapped_dag;

    Layout current_layout;
    if (model->init_layout.empty())
        model->init_layout = generate_random_layout(qubits_used.size(), c_circuit.num_qubits);

    current_layout = model->init_layout;

    std::unordered_map<int, int> pre_executed_counts;
    std::vector<int> front_layer; 
    Matrix distance_matrix = c_circuit.get_distance_matrix();      

    // Initialize the front layer.
    DagGraph::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = boost::edges(dag.graph); ei != ei_end; ++ei) {
        if ( boost::source(*ei, dag.graph) == dag.start_node_pos) {
            const auto target = boost::target(*ei, dag.graph);
            pre_executed_counts[target]++;
            if ((pre_executed_counts[target] == 2) || (dag.graph[target].qubit_pos.size() == 1)) {
                front_layer.push_back(target);
            }
        }
    }

    std::vector<std::pair<int, int>> executed_2gate_list;  // The hardware execution order list of executable 2-qubit gates under current_layout.
    std::set<std::pair<int, int>> unavailable_2qubits;

    // Start the algorithm from the front layer and iterate until all gates are completed.    
    int iteration_count = 0;
    while ( !front_layer.empty() ) {
        std::vector<int> execute_gate_list;
        /* 处理 front_layer 中节点, 记录
            executable_gates        -> (单比特的门，barrier，XY门，measure) + (layout后两个qubit直接相连的两比特门) = 直接可执行的门
            unabailable_2quibits    -> 
        */
        for (const auto& node_index : front_layer) {
            const InstructionNode& node = dag.graph[node_index];
            // 如果是两个qubit的门
            if (node.qubit_pos.size() == 2 && node.name != "barrier" && node.name != "XY" && node.name != "measure") {
                int v0 = node.qubit_pos[0];
                int v1 = node.qubit_pos[1];
                int p0 = current_layout[v0];
                int p1 = current_layout[v1];

                // 如果物理上这front_layer中某个门两个qubit直接相连
                if (boost::edge(p0, p1, c_circuit.graph).second) {
                    execute_gate_list.push_back(node_index);

                    std::set<std::pair<int, int>> unavailable_2qubits_new;
                    for (const auto& item : unavailable_2qubits) {
                        if (item.first != p0 && item.second != p0 && item.first != p1 && item.second != p1)
                            unavailable_2qubits_new.insert(item);
                    }  
                    unavailable_2qubits = std::move(unavailable_2qubits_new);
                }
            } 
            else 
                execute_gate_list.push_back(node_index); // Single-qubit gates, barriers, XY-gates and measures are both executable gates.
        }


        if ( !execute_gate_list.empty() ) {
            for (const int& node_index : execute_gate_list) {
                _apply_gate(mapped_dag, dag.graph[node_index], current_layout);
                front_layer.erase(std::remove(front_layer.begin(), front_layer.end(), node_index), front_layer.end());

                for ( int successor : _dag_successors(dag, node_index) ) {
                    pre_executed_counts[successor]++;
                    if (pre_executed_counts[successor] == dag.graph[successor].qubit_pos.size()) {
                        front_layer.push_back(successor);  
                    }
                }
            }
            iteration_count = 0;
            _reset_qubits_decay();

            continue;
        }


        std::set<int> extended_set = _calc_extended_set(dag, front_layer);

        // 添加swap gate
        std::set<SwapPos> swap_candidates = _obtain_swaps(front_layer, current_layout, dag);
        const SwapPos best_swap = _get_best_swap(dag, swap_candidates, current_layout, front_layer, extended_set, unavailable_2qubits); 
        const InstructionNode swap_gate = InstructionNode("swap", std::vector<qubit_t>{best_swap.first, best_swap.second}); 
        _apply_gate(mapped_dag, swap_gate, current_layout);
        this->add_swap_counter++;
        current_layout.swap(best_swap.first, best_swap.second);

        // 更新 excute_gate_list, unavailable_2qubits
        int min_val = std::min(current_layout[best_swap.first], current_layout[best_swap.second]);
        int max_val = std::max(current_layout[best_swap.first], current_layout[best_swap.second]);
        executed_2gate_list.push_back({min_val, max_val});
        unavailable_2qubits.insert({min_val, max_val});


        // 更新 qubits_decay
        iteration_count++;
        if (iteration_count % this->decay_reset_interval == 0) {
            _reset_qubits_decay();
        } 
        else {
            qubits_decay[best_swap.first] += decay_delta;
            qubits_decay[best_swap.second] += decay_delta;
        } 
    }

    // Update model
    this->model->final_layout = current_layout;


    if (this->modify_dag)   {
        return mapped_dag;
    } else {
        return dag;
    }
}


std::set<int> SabreRouting::_calc_extended_set(const DAGCircuit& dag, const std::vector<int>& front_layer) {
    /*Calculate the extended set for lookahead capabilities.
    Args:
        dag (DAGCircuit): a dag
        front_layer (list): The front layer in the dag.
    Returns:
        extended_set (set): Set of expansion gates obtained according to requirements.
    */ 

    std::set<int> extended_set{};
    std::vector<int> new_front_layer(front_layer);

    while ( !new_front_layer.empty() && extended_set.size() < this->extended_set_size ) {
        int node_index = new_front_layer.front();
        new_front_layer.erase(new_front_layer.begin());

        auto successors_nodes = this->_dag_successors(dag, node_index);
        new_front_layer.insert(new_front_layer.end(), successors_nodes.begin(), successors_nodes.end());          

        for (int successor : successors_nodes) {
            const auto& node = dag.graph[successor];
            if ( node.qubit_pos.size() == 2 ) {
                extended_set.insert(successor);
            }
        }
    }
    return extended_set;
}


std::set<SwapPos> SabreRouting::_obtain_swaps(  const std::vector<int>& front_layer, 
                                                const Layout& current_layout, 
                                                const DAGCircuit& dag ) {   
    std::set<SwapPos> candiate_swaps{};
    for ( const auto& node_index : front_layer )
    {
        for ( const auto& virtual_pos : dag.graph[node_index].qubit_pos )
        {
            int physical_pos = current_layout[virtual_pos];
            auto neighbors = boost::adjacent_vertices(physical_pos, c_circuit.graph);

            for ( auto it =  neighbors.first; it != neighbors.second; ++it )
            {
                int virtual_neighbor = current_layout.get_p2v().at(*it);
                SwapPos swap = std::minmax(virtual_pos, virtual_neighbor);
                candiate_swaps.insert(swap);
            }
        }
    }
    return candiate_swaps;
}

SwapPos SabreRouting::_get_best_swap(   const DAGCircuit& dag,
                                        const std::set<SwapPos>& swap_candidates, 
                                        const Layout& current_layout,
                                        const std::vector<int>& front_layer, 
                                        const std::set<int>& extended_set, 
                                        const std::set<std::pair<int, int>>& unavailable_2qubits) const {
    /*Get the best swap based on different heuristics.

    Args:
        swap_candidates (set): The set of all candidate swap gates.
        current_layout (Layout): current layout
        front_layer (list): front layer gates list
        extended_set (set): set of expansion gates
        unavailable_2qubits (set): set of unavailable two-qubits
    Returns:
        best_swap (tuple): the best swap based on different heuristics
    */
    std::map<SwapPos, double> swap_scores;
    for ( const auto& swap : swap_candidates )
        swap_scores[swap] = -1000000; 

    // TODO
    if ( this->heuristic == Heuristic::FIDELITY ) {
        for ( const auto& swap : swap_candidates )  {
            SwapPos physical_swap = std::minmax(physical_swap.first, physical_swap.second);
            if (unavailable_2qubits.find(physical_swap) == unavailable_2qubits.end()) {
                double swap_cost = _swap_score(physical_swap);

            } 
        }
    }

    else if ( this->heuristic == Heuristic::DISTANCE ) {
        for ( const auto& swap : swap_candidates) {
            double score = _score_heuristic(dag, front_layer, extended_set, current_layout, swap);
            swap_scores[swap] = score;
        }
        // get key of swap_scores mini_value
        auto best_swap = std::min_element(swap_scores.begin(), swap_scores.end(), 
            [](const std::pair<SwapPos, double>& a, const std::pair<SwapPos, double>& b) {
                return a.second < b.second;
            }
        );
        return best_swap->first; 
    }

    // TODO
    else if ( this->heuristic == Heuristic::MIXTURE )
    {}

    return {0,0};
}


double SabreRouting::_score_heuristic(  const DAGCircuit& dag, 
                                        const std::vector<int>& front_layer, 
                                        const std::set<int>& extended_set, 
                                        const Layout& current_layout,
                                        const SwapPos& swap_pos) const {
    Layout trial_layout = Layout(current_layout);
    trial_layout.swap(swap_pos.first, swap_pos.second);

    if ( heuristic == Heuristic::DISTANCE) {
        double front_cost = _compute_distance_cost(dag, front_layer, trial_layout) / static_cast<double>(front_layer.size());
        double extended_cost = 0; 
        if ( !extended_set.empty() ) {
            const std::vector<int> extended_vector(extended_set.begin(), extended_set.end());
            extended_cost = _compute_distance_cost(dag, extended_vector , trial_layout) / static_cast<double>(extended_set.size());
        }

        double total_cost = front_cost + extended_cost*extended_set_weight;
        
        return total_cost * std::max(qubits_decay.at(swap_pos.first), qubits_decay.at(swap_pos.second));
    } 
    return 0;
}


double SabreRouting::_compute_distance_cost(    const DAGCircuit& dag, 
                                                const std::vector<int>& layer,
                                                const Layout& layout) const {
    double cost = 0;
    for (auto node_index : layer) 
        cost += distance_matrix.at(layout[dag.graph[node_index].qubit_pos[0]]).at(layout[dag.graph[node_index].qubit_pos[1]]);
    return cost;
}