#include "libfive/oracle/oracle_clause_custom_function.hpp"
#include "libfive/oracle/oracle_custom_function.hpp"

using namespace Kernel;

CustomFunctionOracleClause::CustomFunctionOracleClause(
        std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd)
    : f(f), fd(fd)
{
    // Nothing to do here
}

std::unique_ptr<Oracle> CustomFunctionOracleClause::getOracle() const
{
    auto o = new CustomFunctionOracle(f, fd);
    return std::unique_ptr<Oracle>(o);
}
