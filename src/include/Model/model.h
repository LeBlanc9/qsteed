#pragma once
#include "backend.h"
#include "layout.h"
#include <map> 


class Model
{
private:
    Backend _backend;

public:
    Layout init_layout;
    Layout final_layout;

    Model();
    Model(Backend backend);

    Backend get_backend() const; 
};