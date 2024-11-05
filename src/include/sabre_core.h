#pragma once
#include <utility>

namespace sabre {

    using qubit_t = int;
    using cbit_t = int;
    using node_pos_t = unsigned long;
    using edge_pos_t = std::pair<node_pos_t, node_pos_t>;
    using SwapPos = std::pair<int, int>;

    enum class Heuristic {
        FIDELITY,
        DISTANCE,
        MIXTURE,
    };

};