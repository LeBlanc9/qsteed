# This code is part of QSteed.
#
# (C) Copyright 2024 Beijing Academy of Quantum Information Sciences
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from typing import Union

from quafu import QuantumCircuit

from qsteed.dag.circuit_dag_convert import dag_to_circuit
from qsteed.dag.dagcircuit import DAGCircuit
from qsteed.passes.datadict import DataDict
from qsteed.passflow.passflow import PassFlow
from qsteed.passflow.preset_passflow import PresetPassflow


class Transpiler:
    def __init__(self, passflow: PassFlow = None, initial_model=None):
        self.passflow = passflow
        if initial_model is None:
            self.model = DataDict()
        else:
            self.model = initial_model
            if self.model.datadict is None:
                self.model.datadict = DataDict()

    def set_model(self, new_model):
        self.model = new_model

    def transpile(self, circuit: Union[QuantumCircuit, DAGCircuit], optimization_level=None):
        # give the parameters of the original circuit only once,be careful!
        self.model.datadict['variables'] = circuit.variables

        if optimization_level is None:
            optimization_level = 0

        if self.passflow is None:
            if optimization_level in [0, 1, 2, 3]:
                self.passflow = PresetPassflow(self.model._backend.get_property('basis_gates'),
                                               optimization_level=optimization_level).get_passflow()
            else:
                raise ValueError("Error: The value of optimization_level is between [0,3].")

        for pass_instance in self.passflow.passes:
            if hasattr(pass_instance, 'set_model'):
                pass_instance.set_model(self.model)

            circuit = pass_instance.run(circuit)

            if hasattr(pass_instance, 'get_model'):
                self.model = pass_instance.get_model()

        if isinstance(circuit, DAGCircuit):
            circuit = dag_to_circuit(circuit, circuit.circuit_qubits)

        return circuit
