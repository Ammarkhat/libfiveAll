// #include <cmath>
#include <vector>
#include <algorithm>
// #include <cstring>
#include <iostream>
#include <unordered_map>
//export const bbox = [-150, -130, -150, 150, 150, 150];

struct VoxelData {
    std::vector<int> dims;
    double step;
    std::vector<double> min;
    std::vector<double> max;
    std::unordered_map<int, std::vector<short>> spheres;
    std::unordered_map<int, std::vector<short>> secondarySpheres;
};

struct Sphere{
    double radius;
    double cx;
    double cy;
    double cz;
};

VoxelData createVoxelData(const std::vector<double>& box, int resolution, double stepFactor = 1.51 / 3) {
    double step = std::max({box[3] - box[0], box[4] - box[1], box[5] - box[2]}) / resolution;
    double stepMin = step * stepFactor;
    double stepMax = step * stepFactor;
    std::vector<double> min = {box[0] - stepMin, box[1] - stepMin, box[2] - stepMin};
    std::vector<double> max = {box[3] + stepMax, box[4] + stepMax, box[5] + stepMax};

    int rx = static_cast<int>(std::ceil((max[0] - min[0]) / step));
    int ry = static_cast<int>(std::ceil((max[1] - min[1]) / step));
    int rz = static_cast<int>(std::ceil((max[2] - min[2]) / step));
    int rxy = rx * ry;

    int datalen = rx * ry * rz;
    std::unordered_map<int, std::vector<short>> spheres;
    std::unordered_map<int, std::vector<short>> secondarySpheres;

    VoxelData voxels;
    voxels.dims = {rx, ry, rz};
    voxels.step = step;
    voxels.min = min;
    voxels.max = max;
    voxels.spheres = std::move(spheres);
    voxels.secondarySpheres = std::move(secondarySpheres);
    return voxels;
}

void voxelizeSpheres(const std::vector<Sphere>& inputSpheres, VoxelData& voxels, std::unordered_map<int, std::vector<short>>& spheres, double expansionAmount = 2) {
    const std::vector<double>& min = voxels.min;
    const std::vector<double>& max = voxels.max;
    double step = voxels.step;
    const std::vector<int>& dims = voxels.dims;
    double invStep = 1.0 / step;

    double vminx = min[0];
    double vminy = min[1];
    double vminz = min[2];

    double vmaxx = max[0];
    double vmaxy = max[1];
    double vmaxz = max[2];

    int rx = dims[0];
    int ry = dims[1];
    int rxy = rx * ry;

    for (int si = 0; si < inputSpheres.size(); ++si) {   
        const Sphere& sphere = inputSpheres[si];
        double radius = sphere.radius;

        // bounding box recomputation
        double xmin = sphere.cx - radius;
        double xmax = sphere.cx + radius;
        double ymin = sphere.cy - radius;
        double ymax = sphere.cy + radius;
        double zmin = sphere.cz - radius;
        double zmax = sphere.cz + radius;

        // add few units as an offset for the box where we compute the distance
        xmin -= expansionAmount;
        ymin -= expansionAmount;
        zmin -= expansionAmount;
        xmax += expansionAmount;
        ymax += expansionAmount;
        zmax += expansionAmount;

        xmin = std::max(xmin, vminx + 2 * step);
        xmax = std::min(xmax, vmaxx - 2 * step);
        ymin = std::max(ymin, vminy + 2 * step);
        ymax = std::min(ymax, vmaxy - 2 * step);
        zmin = std::max(zmin, vminz + 2 * step);
        zmax = std::min(zmax, vmaxz - 2 * step);

        int snapMinx = static_cast<int>(std::floor((xmin - vminx) * invStep));
        int snapMiny = static_cast<int>(std::floor((ymin - vminy) * invStep));
        int snapMinz = static_cast<int>(std::floor((zmin - vminz) * invStep));

        int snapMaxx = static_cast<int>(std::ceil((xmax - vminx) * invStep));
        int snapMaxy = static_cast<int>(std::ceil((ymax - vminy) * invStep));
        int snapMaxz = static_cast<int>(std::ceil((zmax - vminz) * invStep));

        for (int k = snapMinz; k <= snapMaxz; ++k) {
            for (int j = snapMiny; j <= snapMaxy; ++j) {
                for (int i = snapMinx; i <= snapMaxx; ++i) {
                    double x = vminx + i * step;
                    double y = vminy + j * step;
                    double z = vminz + k * step;
                    int n = i + j * rx + k * rxy;
                    if (spheres.find(n) == spheres.end()) {
                        spheres[n] = std::vector<short>();
                    }
                    spheres[n].push_back(si);
                }
            }
        }
    }
}
