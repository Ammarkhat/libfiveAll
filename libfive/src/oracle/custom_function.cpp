#include <array>

#include "libfive/oracle/custom_function.hpp"
#include "libfive/oracle/oracle_clause_custom_function.hpp"

namespace Kernel {

Tree customFunction(std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd){
    Tree t(std::unique_ptr<CustomFunctionOracleClause>(
            new CustomFunctionOracleClause(f, fd)));
    return t;
}

}   

