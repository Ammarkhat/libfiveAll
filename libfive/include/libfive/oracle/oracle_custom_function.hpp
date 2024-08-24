#pragma once

#include "libfive/eval/oracle_storage.hpp"

using namespace Kernel;

namespace Kernel {

class CustomFunctionOracle : public OracleStorage<>
{
public:
    CustomFunctionOracle(std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd);

    void evalInterval(Interval::I &out) override;
    void evalPoint(float& out, size_t index=0) override;

    void checkAmbiguous(
            Eigen::Block<Eigen::Array<bool, 1, LIBFIVE_EVAL_ARRAY_SIZE>,
                         1, Eigen::Dynamic> /* out */) override
    {
        // Nothing to do here
        // (not strictly correct, but close enough)
    }

    void evalDerivs(
            Eigen::Block<Eigen::Array<float, 3, Eigen::Dynamic>,
                         3, 1, true> out, size_t index=0) override;

    void evalFeatures(
            boost::container::small_vector<Feature, 4>& out) override;

protected:
    std::function<float(float, float, float)> f;
    std::function<Eigen::Vector3f(float, float, float)> fd;
};

}   
