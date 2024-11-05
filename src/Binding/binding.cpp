#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "backend.h"
#include "model.h"
#include "coupling.h"
#include "sabre_layout.h"
#include "sabre_routing.h"
#include "DAG/dag.h"
#include "layout.h"
#include "parameter.h"


namespace py = pybind11;
using namespace sabre;

PYBIND11_MODULE(sabre, m) {
    m.doc() = "Sabre Algorithm"; // optional module docstring

    py::class_<SabreLayout>(m, "SabreLayout")
        .def(py::init<const CouplingCircuit&>()) 
        .def("run", &SabreLayout::run)
        .def("get_model", &SabreLayout::get_model)
        .def_readwrite("c_circuit", &SabreLayout::c_circuit)
        .def_readwrite("max_iterations", &SabreLayout::max_iterations)
        .def_readwrite("heuristic", &SabreLayout::heuristic);

     py::class_<SabreRouting>(m, "SabreRouting")
        .def(py::init<const CouplingCircuit&>()) 
        .def("set_model", &SabreRouting::set_model)
        .def("get_model", &SabreRouting::get_model)
        .def("run", &SabreRouting::run)
        .def_readwrite("modify_dag", &SabreRouting::modify_dag);

    py::class_<Backend>(m, "Backend")
        .def(py::init<CouplingList>())
        .def(py::init<>());

    py::class_<Model>(m, "Model")
        .def(py::init<>())
        .def(py::init<Backend>())
        .def_readwrite("init_layout", &Model::init_layout)
        .def_readwrite("final_layout", &Model::final_layout);


    py::class_<CouplingCircuit>(m, "CouplingCircuit")
        .def(py::init<CouplingList>())
        .def("update_num_qubits", &CouplingCircuit::update_num_qubits)
        .def("get_distance_matrix", &CouplingCircuit::get_distance_matrix)
        .def_readwrite("num_qubits", &CouplingCircuit::num_qubits);

    py::class_<Layout>(m, "Layout")
        .def(py::init<>())
        .def(py::init<LayoutStructure>())
        .def("get_v2p", &Layout::get_v2p)
        .def("__getitem__", &Layout::operator[]);

    py::class_<InstructionNode>(m, "InstructionNode")
        .def(py::init<>())
        .def(py::init<const std::string>())

        .def(py::init<const std::string, const std::vector<int>>())
        .def(py::init<const std::string, const int>())


        // for Parameter 
        .def(py::init<const std::string, const int, const std::vector<Parameter>, const int, const std::string>())
        .def(py::init<const std::string, const std::vector<int>, const std::vector<Parameter>, const int, const std::string>())

        // for paras
        .def(py::init<const std::string, const int, const std::vector<double>, const int, const std::string>())
        .def(py::init<const std::string, const std::vector<int>, const std::vector<double>, const int, const std::string>())
        .def(py::init<const std::string, const int, const double, const int, const std::string>())
        .def(py::init<const std::string, const std::vector<int>, const double, const int, const std::string>())

        .def_readwrite("name", &InstructionNode::name)
        .def_readwrite("qubit_pos", &InstructionNode::qubit_pos)
        .def_readwrite("paras", &InstructionNode::paras)
        .def_readwrite("parameter", &InstructionNode::parameters)
        .def_readwrite("duration", &InstructionNode::duration)
        .def_readwrite("unit", &InstructionNode::unit);

    py::class_<MeasureNode, InstructionNode>(m, "MeasureNode")
        .def(py::init<>())
        .def(py::init<std::vector<int>, std::vector<int>>());

    py::class_<EdgeProperties>(m, "EdgeProperties")
        .def(py::init<>())
        .def(py::init<int>())
        .def_readwrite("id", &EdgeProperties::qubit_id);

    py::class_<DagGraph>(m, "DagGraph")
        .def("__getitem__", [](const DagGraph &g, node_pos_t i) {
            if (i<0 || i>= boost::num_vertices(g))
                throw py::index_error("Out of index range!");
            else
                return g[i];
        });


    py::class_<DAGCircuit>(m, "DAGCircuit")
        .def(py::init<>())
        .def("add_node", &DAGCircuit::add_node)
        .def("add_instruction_node_end", &DAGCircuit::add_instruction_node_end)
        .def("add_edge", py::overload_cast<const int, const int, const qubit_t>(&DAGCircuit::add_edge))
        .def("add_edge", py::overload_cast<const int, const int, const EdgeProperties&>(&DAGCircuit::add_edge))
        .def("get_num_nodes", &DAGCircuit::get_num_nodes)
        .def("get_qubits_used", &DAGCircuit::get_qubits_used)
        .def("vertices", [](DAGCircuit &s) {return py::make_iterator(s.vertex_begin(), s.vertex_end());})
        .def("reverse", &DAGCircuit::reverse)
        .def_readwrite("graph", &DAGCircuit::graph)
        .def_readwrite("measure", &DAGCircuit::measure);

    py::enum_<Heuristic>(m, "Heuristic")
        .value("DISTANCE", Heuristic::DISTANCE)
        .value("FIDELITY", Heuristic::FIDELITY)
        .value("MIXTURE", Heuristic::MIXTURE);
    // m.def("reverse_DagGraph", &reverse_DagGraph, "Reverse a DagGraph object");


    // Parameter
    py::enum_<OperatorType>(m, "OperatorType")
        .value("ADD", OperatorType::ADD)
        .value("SUB", OperatorType::SUB)
        .value("MUL", OperatorType::MUL)
        .value("TRUEDIV", OperatorType::TRUEDIV)
        .value("FLOORDIV", OperatorType::FLOORDIV)
        .value("POW", OperatorType::POW)
        .value("SIN", OperatorType::SIN)
        .value("COS", OperatorType::COS)
        .value("TAN", OperatorType::TAN)
        .value("EXP", OperatorType::EXP)
        .value("LOG", OperatorType::LOG)
        .value("SQRT", OperatorType::SQRT)
        .value("ABS", OperatorType::ABS)
        .value("NEG", OperatorType::NEG)
        .value("CONST", OperatorType::CONST);


    py::enum_<ParameterType>(m, "ParameterType")
        .value("None", ParameterType::None)
        .value("DOUBLE", ParameterType::DOUBLE)
        .value("PARAMETER", ParameterType::PARAMETER)
        .value("PARAMETER_EXPRESSION", ParameterType::PARAMETER_EXPRESSION);

    py::class_<ParameterExpression>(m, "ParameterExpression")
        .def(py::init<>())
        .def_readwrite("operands", &ParameterExpression::operands)
        .def_readwrite("funcs", &ParameterExpression::operators);

    py::class_<Parameter>(m, "Parameter")
        .def(py::init<>())
        .def(py::init<ParameterType, double>())
        .def(py::init<ParameterType, const std::string, double>())
        .def_readwrite("operands", &Parameter::operands)
        .def_readwrite("funcs", &Parameter::operators)
        .def_readwrite("type", &Parameter::ptype)
        .def_readwrite("name", &Parameter::name)
        .def_readwrite("value", &Parameter::value)
        .def_readwrite("tunable", &Parameter::tunable);

}