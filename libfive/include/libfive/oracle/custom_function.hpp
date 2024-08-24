#pragma once

#include "./oracle_custom_function.hpp"
#include "libfive/tree/tree.hpp"

namespace Kernel {

Tree customFunction(std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd);

}  
