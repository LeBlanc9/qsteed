#include "model.h"
#include <iostream>


Model::Model() {
    // std::cout << "Empty model created" << std::endl;
    // generate_random_layout();
}

Model::Model(Backend backend) : _backend(backend) {
    std::cout << "Model with backend created" << std::endl;
}

Backend  Model::get_backend() const {
    return _backend;
}
