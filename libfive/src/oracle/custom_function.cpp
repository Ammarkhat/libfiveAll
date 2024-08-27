#include <array>

#include "libfive/oracle/custom_function.hpp"
#include "libfive/oracle/oracle_clause_custom_function.hpp"

namespace Kernel {

Tree customFunction(CustomFunctionWrapper* cfw){
    Tree t(std::unique_ptr<CustomFunctionOracleClause>(
            new CustomFunctionOracleClause(cfw)));
    return t;
}

}   

