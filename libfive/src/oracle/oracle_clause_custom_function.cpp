#include "libfive/oracle/oracle_clause_custom_function.hpp"
#include "libfive/oracle/oracle_custom_function.hpp"

using namespace libfive;

CustomFunctionOracleClause::CustomFunctionOracleClause(
        CustomFunctionWrapper *cfw)
    : cfw(cfw)
{
    // Nothing to do here
}

std::unique_ptr<Oracle> CustomFunctionOracleClause::getOracle() const
{
    auto o = new CustomFunctionOracle(cfw);
    return std::unique_ptr<Oracle>(o);
}
