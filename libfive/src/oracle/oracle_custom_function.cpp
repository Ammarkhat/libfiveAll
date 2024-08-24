#include "libfive/oracle/oracle_custom_function.hpp"
#include <iostream>

using namespace Kernel;

CustomFunctionOracle::CustomFunctionOracle(std::function<float(float, float, float)> f, std::function<Eigen::Vector3f(float, float, float)> fd)
    : f(f), fd(fd)
{
}

void CustomFunctionOracle::evalInterval(Interval::I& out)
{
    // out = Interval(0.0f, 1.0f);
    // out = {-10000.0, 10000.0};
    auto center = (lower + upper)/2;    
    float centerValue;

    Eigen::Vector3f before = points.col(0);
    points.col(0) = center;
    evalPoint(centerValue);
    points.col(0) = before;

    auto diagonal = (upper - lower).norm();
    out = {centerValue - diagonal/2, centerValue + diagonal/2};
}

void CustomFunctionOracle::evalPoint(float& out, size_t index)
{
    const auto pt = points.col(index);
    out = f(pt.x(), pt.y(), pt.z());

    // Initialize out to an invalid value, so we can check it afterwards
    // out = -1;
    // float closest = std::numeric_limits<float>::infinity();

    // for (unsigned i=0; i < spheres.size(); ++i)
    // {
    //     auto s = spheres[i];
    //     auto dx = pt[0] - s.cx;
    //     auto dy = pt[1] - s.cy;
    //     auto dz = pt[2] - s.cz;
    //     auto d = (dx * dx + dy * dy + dz * dz) - s.radius * s.radius;
    //     if (d < closest)
    //     {
    //         closest = d;
    //         out = d;
    //     }
    // }

    // // std::cout << " eval pt: "<< pt[0]<< "  " << pt[1] << "  " << pt[2]<< ", "<< closest << std::endl;

    // assert(out >= 0 && out <= 1);
}

void CustomFunctionOracle::evalDerivs(
            Eigen::Block<Eigen::Array<float, 3, Eigen::Dynamic>,
                         3, 1, true> out, size_t index)
{
    const auto pt = points.col(index);
    out = fd(pt.x(), pt.y(), pt.z());

    // out = Eigen::Vector3f(0,0,0);
    // const float EPSILON = 1e-6;
    // float center, dx, dy, dz;

    // Eigen::Vector3f before = points.col(0);
    // evalPoint(center);

    // points.col(0) = before + Eigen::Vector3f(EPSILON, 0.0, 0.0);
    // evalPoint(dx);

    // points.col(0) = before + Eigen::Vector3f(0.0, EPSILON, 0.0);
    // evalPoint(dy);

    // points.col(0) = before + Eigen::Vector3f(0.0, 0.0, EPSILON);
    // evalPoint(dz);

    // points.col(0) = before;

    // out = Eigen::Vector3f(
    //     (dx - center) / EPSILON,
    //     (dy - center) / EPSILON,
    //     (dz - center) / EPSILON);
}

void CustomFunctionOracle::evalFeatures(
        boost::container::small_vector<Feature, 4>& out)
{
}

// Numerically solve for the gradient
// void CustomFunctionOracle::evalDerivs(
//             Eigen::Block<Eigen::Array<float, 3, Eigen::Dynamic>,
//                          3, 1, true> out, size_t index)
// {
//     out = Eigen::Vector3f(0,0,0);
//     // const float EPSILON = 1e-6;
//     // float center, dx, dy, dz;

//     // Eigen::Vector3f before = points.col(0);
//     // evalPoint(center);

//     // points.col(0) = before + Eigen::Vector3f(EPSILON, 0.0, 0.0);
//     // evalPoint(dx);

//     // points.col(0) = before + Eigen::Vector3f(0.0, EPSILON, 0.0);
//     // evalPoint(dy);

//     // points.col(0) = before + Eigen::Vector3f(0.0, 0.0, EPSILON);
//     // evalPoint(dz);

//     // points.col(0) = before;

//     // out = Eigen::Vector3f(
//     //     (dx - center) / EPSILON,
//     //     (dy - center) / EPSILON,
//     //     (dz - center) / EPSILON);
// }

// void CustomFunctionOracle::evalFeatures(
//         boost::container::small_vector<Feature, 4>& out)
// {
//     const float EPSILON = 1e-6;
//     float center, dx, dy, dz;

//     Eigen::Vector3f before = points.col(0);
//     evalPoint(center);

//     points.col(0) = before + Eigen::Vector3f(EPSILON, 0.0, 0.0);
//     evalPoint(dx);

//     points.col(0) = before + Eigen::Vector3f(0.0, EPSILON, 0.0);
//     evalPoint(dy);

//     points.col(0) = before + Eigen::Vector3f(0.0, 0.0, EPSILON);
//     evalPoint(dz);

//     points.col(0) = before;

//     out.push_back(Eigen::Vector3f(
//         (dx - center) / EPSILON,
//         (dy - center) / EPSILON,
//         (dz - center) / EPSILON));
        
//     // Eigen::Array<float, 3, Eigen::Dynamic> ds(3, 1);
//     // evalDerivs(ds.col(0), 0);
//     // out.push_back(Feature(ds.col(0)));
// }
