#pragma once

#include "libfive/oracle/oracle_storage.hpp"

using namespace libfive;

namespace libfive {

class CustomFunctionWrapper{
    public:
        virtual float f(float, float, float) = 0;
        virtual Eigen::Vector3f fd(float, float, float) = 0;
};

class CustomFunctionOracle : public OracleStorage<>
{
public:
    CustomFunctionOracle(CustomFunctionWrapper* cfw);

    void evalInterval(Interval &out) override;
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
    CustomFunctionWrapper* cfw;
};

}   
