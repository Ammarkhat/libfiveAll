// #include <cmath>
#include <vector>
#include <algorithm>
// #include <cstring>
#include <iostream>

struct PixelData {
    std::vector<int> dims;
    double step;
    std::vector<double> min;
    std::vector<double> max;
    std::vector<std::vector<int>> circles;
    std::vector<std::vector<int>> secondaryCircles;
};

struct Circle{
    double radius;
    double cx;
    double cy;
};

PixelData createPixelData(const std::vector<double>& box, int resolution, double stepFactor = 1.51 / 3) {
    double step = std::max({box[2] - box[0], box[3] - box[1]}) / resolution;
    double stepMin = step * stepFactor;
    double stepMax = step * stepFactor;
    std::vector<double> min = {box[0] - stepMin, box[1] - stepMin};
    std::vector<double> max = {box[2] + stepMax, box[3] + stepMax};

    int rx = static_cast<int>(std::ceil((max[0] - min[0]) / step));
    int ry = static_cast<int>(std::ceil((max[1] - min[1]) / step));

    int datalen = rx * ry;
    std::vector<std::vector<int>> circles(datalen, std::vector<int>());
    std::vector<std::vector<int>> secondaryCircles(datalen, std::vector<int>());

    PixelData voxels;
    voxels.dims = {rx, ry};
    voxels.step = step;
    voxels.min = min;
    voxels.max = max;
    voxels.circles = std::move(circles);
    voxels.secondaryCircles = std::move(secondaryCircles);
    return voxels;
}

void PixalizeCircles(const std::vector<Circle>& inputSpheres, PixelData& voxels, std::vector<std::vector<int>>& circles, double expansionAmount = 2) {
    const std::vector<double>& min = voxels.min;
    const std::vector<double>& max = voxels.max;
    double step = voxels.step;
    const std::vector<int>& dims = voxels.dims;
    double invStep = 1.0 / step;

    double vminx = min[0];
    double vminy = min[1];

    double vmaxx = max[0];
    double vmaxy = max[1];

    int rx = dims[0];
    int ry = dims[1];

    for (int si = 0; si < inputSpheres.size(); ++si) {   
        const Circle& sphere = inputSpheres[si];
        double radius = sphere.radius;

        // bounding box recomputation
        double xmin = sphere.cx - radius;
        double xmax = sphere.cx + radius;
        double ymin = sphere.cy - radius;
        double ymax = sphere.cy + radius;

        // add few units as an offset for the box where we compute the distance
        xmin -= expansionAmount;
        ymin -= expansionAmount;
        xmax += expansionAmount;
        ymax += expansionAmount;

        xmin = std::max(xmin, vminx + 2 * step);
        xmax = std::min(xmax, vmaxx - 2 * step);
        ymin = std::max(ymin, vminy + 2 * step);
        ymax = std::min(ymax, vmaxy - 2 * step);

        int snapMinx = static_cast<int>(std::floor((xmin - vminx) * invStep));
        int snapMiny = static_cast<int>(std::floor((ymin - vminy) * invStep));

        int snapMaxx = static_cast<int>(std::ceil((xmax - vminx) * invStep));
        int snapMaxy = static_cast<int>(std::ceil((ymax - vminy) * invStep));

        for (int j = snapMiny; j <= snapMaxy; ++j) {
            for (int i = snapMinx; i <= snapMaxx; ++i) {
                double x = vminx + i * step;
                double y = vminy + j * step;
                int n = i + j * rx;
                circles[n].push_back(si);
            }
        }
    }
}
