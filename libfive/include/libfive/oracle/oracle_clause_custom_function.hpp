#pragma once

#include <Eigen/Eigen>

#include "libfive/tree/oracle_clause.hpp"
#include "./oracle_custom_function.hpp"

namespace Kernel {

class CustomFunctionOracleClause : public OracleClause
{
public:
    CustomFunctionOracleClause(std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd);
    std::unique_ptr<Oracle> getOracle() const override;
    std::string name() const { return "CustomFunctionOracleClause"; }

protected:
    std::function<float(float, float, float)> f;
    std::function<Eigen::Vector3f(float, float, float)> fd;
};

}
