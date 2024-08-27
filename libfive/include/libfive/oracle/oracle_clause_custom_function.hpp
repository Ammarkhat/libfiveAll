#pragma once

#include <Eigen/Eigen>

#include "libfive/oracle/oracle_clause.hpp"
#include "./oracle_custom_function.hpp"

namespace Kernel {

class CustomFunctionOracleClause : public OracleClause
{
public:
    CustomFunctionOracleClause(CustomFunctionWrapper *cfw);
    std::unique_ptr<Oracle> getOracle() const override;
    std::string name() const { return "CustomFunctionOracleClause"; }

protected:
    CustomFunctionWrapper *cfw;
};

}
