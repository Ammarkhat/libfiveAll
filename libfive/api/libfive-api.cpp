#include <emscripten/bind.h>
#include <string>
#include <vector>

// #include <iostream>
// #include <iterator>

#include "libfive.h"
// #include "libfive/solve/bounds.hpp"
#include "libfive/render/brep/mesh.hpp"
#include "libfive/render/brep/region.hpp"

#include "libfive/oracle/custom_function.hpp"
#include "libfive/oracle/oracle_custom_function.hpp"
#include <Eigen/Eigen>
#include "gridBased.hpp"
#include "gridBasedCircles.hpp"
#include <unordered_map>

// #include <bits/stdc++.h>
// #include <iostream>
// #include <chrono>
using namespace std;

#define BOOST_NO_EXCEPTIONS
#include <boost/throw_exception.hpp>
void boost::throw_exception(std::exception const & e){
//do nothing
}

// Tree _union(Tree, Tree);
// Tree intersection(Tree, Tree);
// Tree inverse(Tree);
// Tree difference(Tree, Tree);
// Tree offset(Tree, TreeFloat);
// Tree sphere(TreeFloat, TreeVec3);

const char *OUTPUT_FILENAME = "exported.stl";
// const float OUTPUT_RESOLUTION = 15.0;

using namespace emscripten;
using namespace libfive;

Tree sphereOld(float radius, float cx, float cy, float cz){
  auto x = Tree::X() - Tree(cx);
  auto y = Tree::Y() - Tree(cy);
  auto z = Tree::Z() - Tree(cz);

  auto r = Tree(radius);
  auto out = (x * x) + (y * y) + (z * z) - r;
  return out;
}
struct TreeVec2 {
    Tree x, y;
};
struct TreeVec3 {
    Tree x, y, z;
};
typedef Tree TreeFloat;

#define LIBFIVE_DEFINE_XYZ() const auto x = Tree::X(); (void)x; \
                             const auto y = Tree::Y(); (void)y; \
                             const auto z = Tree::Z(); (void)z; ;

Tree _union(Tree a, Tree b) {
    return min(a, b);
}
Tree intersection(Tree a, Tree b) {
    return max(a, b);
}
Tree inverse(Tree a) {
    return -a;
}
Tree difference(Tree a, Tree b) {
    return intersection(a, inverse(b));
}
Tree offset(Tree a, float off) {
    return a - off;
}

Tree scale_x(Tree t, TreeFloat sx, TreeFloat x0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x0 + (x - x0) / sx, y, z);
}

Tree scale_y(Tree t, TreeFloat sy, TreeFloat y0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x, y0 + (y - y0) / sy, z);
}

Tree scale_z(Tree t, TreeFloat sz, TreeFloat z0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x, y, z0 + (z - z0) / sz);
}

Tree clamp(Tree x, TreeFloat mn, TreeFloat mx){
  return min(max(x, mn), mx);
}

Tree elongate(Tree t, TreeVec3 offset) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - clamp(x, -offset.x, offset.x), y - clamp(y, -offset.y, offset.y), z - clamp(z, -offset.z, offset.z));
}

Tree move(Tree t, TreeVec3 offset) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - offset.x, y - offset.y, z - offset.z);
}

Tree move(Tree t, float cx, float cy, float cz) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - cx, y - cy, z - cz);
}

Tree circle(TreeFloat r, TreeVec2 center) {
    LIBFIVE_DEFINE_XYZ();
    auto c = sqrt(x * x + y * y) - r;
    return move(c, {center.x, center.y, 0});
}

Tree rotate_x(Tree t, TreeFloat angle, TreeVec3 center) {
    LIBFIVE_DEFINE_XYZ();
    t = move(t, {-center.x, -center.y, -center.z});
    return move(t.remap(x,
                        cos(angle) * y + sin(angle) * z,
                       -sin(angle) * y + cos(angle) * z), center);
}

Tree rotate_y(Tree t, TreeFloat angle, TreeVec3 center) {
    LIBFIVE_DEFINE_XYZ();
    t = move(t, {-center.x, -center.y, -center.z});
    return move(t.remap(cos(angle) * x + sin(angle) * z,
                        y,
                       -sin(angle) * x + cos(angle) * z), center);
}

Tree rotate_z(Tree t, TreeFloat angle, TreeVec3 center) {
    LIBFIVE_DEFINE_XYZ();
    t = move(t, {-center.x, -center.y, -center.z});
    return move(t.remap(cos(angle) * x + sin(angle) * y,
                       -sin(angle) * x + cos(angle) * y, z), center);
}

Tree sphere(float r, float cx, float cy, float cz) {
    LIBFIVE_DEFINE_XYZ();
    return move(sqrt(square(x) + square(y) + square(z)) - r, cx, cy, cz);
}

Tree torus_z(TreeFloat ro, TreeFloat ri, TreeVec3 center) {
    LIBFIVE_DEFINE_XYZ();
    return move(
        sqrt(square(ro - sqrt(square(x) + square(y)))
           + square(z)) - ri,
        center);
}

Tree torus(float r, float cx, float cy) {
    return move(rotate_x(torus_z(cx, r, {0,0,0}), M_PI/2, {0,0,0}), {0,cy,0});
}

Tree capsule(TreeFloat r1, TreeFloat r2, TreeVec3 p1, TreeVec3 p2){
      const auto x = Tree::X(); 
      const auto y = Tree::Y(); 
      const auto z = Tree::Z();

      auto rdiff = r2 - r1;
      auto unitx = p2.x - p1.x;
      auto unity = p2.y - p1.y;
      auto unitz = p2.z - p1.z;
      auto lengthSq = square(unitx) + square(unity) + square(unitz);
      auto length = sqrt(lengthSq);
      unitx = unitx / length;
      unity = unity / length;
      unitz = unitz / length;

      auto vx = x - p1.x;
      auto vy = y - p1.y;
      auto vz = z - p1.z;
      auto p1p_sqrl = square(vx) + square(vy) + square(vz);
      auto x_p_2D = vx * unitx + vy * unity + vz * unitz;
      auto y_p_2D = sqrt(
                max( // Necessary because of rounded errors, pyth result can be <0 and this causes sqrt to return NaN...
                    0.0, p1p_sqrl - square(x_p_2D) // =  y_p_2D² by pythagore
                )
            );
      auto t = -y_p_2D / length;

      auto proj_x = x_p_2D + t * (r1 - r2);

      // Easy way to compute the distance now that we ave the projection on the segment
      auto a = max( 0, min( 1.0, proj_x / length ) );
      auto projx = p1.x + ( p2.x - p1.x ) * a;
		  auto projy = p1.y + ( p2.y - p1.y ) * a;
		  auto projz = p1.z + ( p2.z - p1.z ) * a;

      auto lx = x - projx;
      auto ly = y - projy;
      auto lz = z - projz;
      auto llengthSq = square(lx) + square(ly) + square(lz);
      auto llength = sqrt(llengthSq);
      return llength - (a * r2 + (1.0 - a) * r1);
}

Tree extrude_z(Tree t, TreeFloat zmin, TreeFloat zmax) {
    LIBFIVE_DEFINE_XYZ();
    return max(t, max(zmin - z, z - zmax));
}

Tree revolve_y(Tree shape, TreeFloat x0) {
    LIBFIVE_DEFINE_XYZ();
    const auto r = sqrt(square(x) + square(z));
    TreeVec3 center{-x0, 0, 0};
    shape = move(shape, center);
    return move(_union(shape.remap(r, y, z), shape.remap(-r, y, z)), center);
}

Tree rectangle(TreeVec2 a, TreeVec2 b) {
    LIBFIVE_DEFINE_XYZ();
    return max(
        max(a.x - x, x - b.x),
        max(a.y - y, y - b.y));
}

Tree box_mitered(TreeVec3 a, TreeVec3 b) {
    return extrude_z(rectangle({a.x, a.y}, {b.x, b.y}), a.z, b.z);
}
Tree blend_expt(Tree a, Tree b, TreeFloat m) {
    return -log(exp(-m * a) + exp(-m * b)) / m;
}

Tree blend_expt_unit(Tree a, Tree b, TreeFloat m) {
    return blend_expt(a, b, 2.75 / pow(m, 2));
}

Tree blend_rough(Tree a, Tree b, TreeFloat m) {
    auto c = sqrt(abs(a)) + sqrt(abs(b)) - m;
    return _union(a, _union(b, c));
}


Tree half_plane(TreeVec2 a, TreeVec2 b) {
    LIBFIVE_DEFINE_XYZ();
    return (b.y - a.y) * (x - a.x) - (b.x - a.x) * (y - a.y);
}

Tree half_space(TreeVec3 norm, TreeVec3 point) {
    LIBFIVE_DEFINE_XYZ();
    // dot(pos - point, norm)
    return (x - point.x) * norm.x +
           (y - point.y) * norm.y +
           (z - point.z) * norm.z;
}

Tree triangle(TreeVec2 a, TreeVec2 b, TreeVec2 c) {
    LIBFIVE_DEFINE_XYZ();
    // We don't know which way the triangle is wound, and can't actually
    // know (because it could be parameterized, so we return the union
    // of both possible windings)
    return _union(
        intersection(intersection(
            half_plane(a, b), half_plane(b, c)), half_plane(c, a)),
        intersection(intersection(
            half_plane(a, c), half_plane(c, b)), half_plane(b, a)));
}


Tree reflect_x(Tree t, TreeFloat x0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(2*x0 - x, y, z);
}

Tree reflect_y(Tree t, TreeFloat y0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x, 2*y0 - y, z);
}

Tree reflect_z(Tree t, TreeFloat z0) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x, y, 2*z0 - z);
}

Tree reflect_xy(Tree t) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(y, x, z);
}

Tree reflect_yz(Tree t) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x, z, y);
}

Tree reflect_xz(Tree t) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(z, y, x);
}


Tree array_x(Tree shape, int nx, TreeFloat dx) {
    auto out = shape;
    for (int i=1; i < nx; ++i) {
        out = _union(out, move(shape, {dx * i, 0, 0}));
    }
    return out;
}

Tree array_xy(Tree shape, int nx, int ny, TreeVec2 delta) {
    shape = array_x(shape, nx, delta.x);
    auto out = shape;
    for (int i=1; i < ny; ++i) {
        out = _union(out, move(shape, {0, delta.y * i, 0}));
    }
    return out;
}

Tree array_xyz(Tree shape, int nx, int ny, int nz,
                         TreeVec3 delta) {
    shape = array_xy(shape, nx, ny, {delta.x, delta.y});
    auto out = shape;
    for (int i=1; i < nz; ++i) {
        out = _union(out, move(shape, {0, 0, delta.y * i}));
    }
    return out;
}


Tree cylinder_z(TreeFloat r, TreeFloat h, TreeVec3 base) {
    return extrude_z(circle(r, {base.x, base.y}), base.z, base.z + h);
}
Tree cylinder_y(TreeFloat r, TreeFloat h, TreeVec3 base) {
    return rotate_x(extrude_z(circle(r, {base.x, -base.z}), base.y, base.y + h), -M_PI/2, {0,0,0});
}
Tree cylinder_x(TreeFloat r, TreeFloat h, TreeVec3 base) {
    return rotate_y(extrude_z(circle(r, {-base.z, base.y}), base.x, base.x + h), -M_PI/2, {0,0,0});
}

vector<string> splitBySpaces(string s)
{
  vector<string> result;
	stringstream ss(s);
	string word;
	while (ss >> word) {
		result.push_back(word);
	}
	return result;
}

class Node {
public:
    Node(){}
    string type;
    vector<Node> children;
    vector<double> data;
    Node* parent;
};

// void printTree(const Node& root, int depth = 0) {
//     for (int i = 0; i < depth; ++i) {
//         std::cout << "  ";
//     }
//     std::cout << root.type << " " << root.children.size() << std::endl;

//     for (const auto& child : root.children) {
//         printTree(child, depth + 1);
//     }
// }

int parseNode(Node* parentNode, vector<string> words, int i){
    int nextPosition = i;
    Node node;
    string word = words[i];
    if(word == "union" || word == "intersection" || word == "difference" || word == "blend" || word == "spheres" || word == "revolveCircles"){
        node.type = word;
        nextPosition = parseNode(&node, words, i+2);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
    }else if(word == "offset"){
        node.type = "offset";
        node.data.push_back(stod(words[i+2]));
        nextPosition = parseNode(&node, words, i+3);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
     }else if(word == "extrude"){
        node.type = "extrude";
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        nextPosition = parseNode(&node, words, i+4);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
      }else if(word == "revolve"){
        node.type = "revolve";
        nextPosition = parseNode(&node, words, i+2);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
      }else if(word == "rotate" || word == "scaleX" || word == "scaleY" || word == "scaleZ"){
        node.type = word;
        node.data.push_back(stod(words[i+2]));
        nextPosition = parseNode(&node, words, i+3);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
       }else if(word == "scale" || word == "position"){
        node.type = word;
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        nextPosition = parseNode(&node, words, i+5);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
      }else if(word == "rotation"){
        node.type = word;
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        node.data.push_back(stod(words[i+5]));
        node.data.push_back(stod(words[i+6]));
        node.data.push_back(stod(words[i+7]));
        nextPosition = parseNode(&node, words, i+8);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
      }else if(word == "limit"){
        node.type = "limit";
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        nextPosition = parseNode(&node, words, i+5);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
     }else if(word == "extend"){
        node.type = "extend";
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        nextPosition = parseNode(&node, words, i+5);
        parentNode->children.push_back(node);
        if(nextPosition < words.size()-1){
            nextPosition = parseNode(parentNode, words, nextPosition);
        }
    }else if(word == "s"){
        node.type = "sphere";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+5);
    }else if(word == "c"){
        node.type = "cylinder";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        node.data.push_back(stod(words[i+5]));
        node.data.push_back(stod(words[i+6]));
        node.data.push_back(stod(words[i+7]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+8);
    }else if(word == "sphere"){
        node.type = "sphere";
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        node.data.push_back(stod(words[i+5]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+6);
    }else if(word == "c"){
        node.type = "capsule";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        node.data.push_back(stod(words[i+5]));
        node.data.push_back(stod(words[i+6]));
        node.data.push_back(stod(words[i+7]));
        node.data.push_back(stod(words[i+8]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+9);
    }else if(word == "tr"){
        node.type = "triangle";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        node.data.push_back(stod(words[i+4]));
        node.data.push_back(stod(words[i+5]));
        node.data.push_back(stod(words[i+6]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+7);
    }else if(word == "t"){
        node.type = "torus";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+4);
    }else if(word == "cc"){
        node.type = "circle";
        node.data.push_back(stod(words[i+1]));
        node.data.push_back(stod(words[i+2]));
        node.data.push_back(stod(words[i+3]));
        parentNode->children.push_back(node);
        nextPosition = parseNode(parentNode, words, i+4);
    }else if(word == ")"){
        nextPosition = i + 1;
    }else{
        if(i < words.size()-1){
            nextPosition = parseNode(parentNode, words, i+1);
        }
    }
    return nextPosition;
}

class BruteForceSpheresFunction: public CustomFunctionWrapper{
    public:
      std::vector<Sphere> spheresArray;
      virtual float f(float x, float y, float z) {
         float totalD = 1000000;
        // std::cout << "spheresArray.size: " << this->spheresArray.size() << std::endl;
        for (size_t i = 0; i < this->spheresArray.size(); i++)
        {
          auto s = this->spheresArray[i];
          auto d = sqrt((x-s.cx) * (x-s.cx) + (y-s.cy) * (y-s.cy) + (z-s.cz) * (z-s.cz)) - s.radius;
          if(d < totalD){
            totalD = d;
          }
        }
        return totalD;
      }
      virtual Eigen::Vector3f fd(float x, float y, float z) {
        float totalD = 1000000;
        int minIndex = -1;
        // std::cout << "spheresArray.size: " << this->spheresArray.size() << std::endl;
        for (size_t i = 0; i < this->spheresArray.size(); i++)
        {
          auto s = this->spheresArray[i];
          auto d = sqrt((x-s.cx) * (x-s.cx) + (y-s.cy) * (y-s.cy) + (z-s.cz) * (z-s.cz)) - s.radius;
          if(d < totalD){
            totalD = d;
            minIndex = i;
          }
        }
        auto s = this->spheresArray[minIndex];
        auto v = Eigen::Vector3f{x-s.cx,y-s.cy,z-s.cz};
        return v;
      }

};

class GridBasedCFW: public CustomFunctionWrapper{
    public:
    double vminx;
    double vminy;
    double vminz;
    double step;
    double invStep;
    int rx;
    int ry;
    int rxy;
    VoxelData voxelData;
    std::vector<short> spheresIndices;
    std::vector<Sphere> spheresArray;

    GridBasedCFW(std::vector<Sphere>& spheresArray): spheresArray(spheresArray){
      // auto t1 = std::chrono::high_resolution_clock::now();
      std::vector<double> box {-150, -130, -150, 150, 150, 150};
      voxelData = createVoxelData(box, 150, 1.51);
      std::unordered_map<int, std::vector<short>>& innerSpheres = voxelData.spheres;
      voxelizeSpheres(spheresArray, voxelData, innerSpheres, 2);

      std::unordered_map<int, std::vector<short>>& secondarySpheres = voxelData.secondarySpheres;
      voxelizeSpheres(spheresArray, voxelData, secondarySpheres, 20);
      // auto t2 = std::chrono::high_resolution_clock::now();
      // std::cout << "grid construction time (s): " << (t2 - t1).count() / 1.0e9 << std::endl;

      vminx = voxelData.min[0];
      vminy = voxelData.min[1];
      vminz = voxelData.min[2];
      step = voxelData.step;
      invStep = 1.0 / step;
      rx = voxelData.dims[0];
      ry = voxelData.dims[1];
      rxy = rx * ry;

      for( int i = 0; i < spheresArray.size(); i++ )
        spheresIndices.push_back( (short)i );
    }
      virtual float f(float x, float y, float z) {
        // Precompute the indices and distance bounds
        int i = static_cast<int>(std::floor((x - vminx) * invStep));
        int j = static_cast<int>(std::floor((y - vminy) * invStep));
        int k = static_cast<int>(std::floor((z - vminz) * invStep));
        int n = i + j * rx + k * rxy;

        float totalD = 1000000.0f;  // Large initial value
        const auto& spheres = voxelData.spheres.find(n) != voxelData.spheres.end() ? voxelData.spheres[n] : (
          voxelData.secondarySpheres.find(n) != voxelData.secondarySpheres.end() ? voxelData.secondarySpheres[n] : spheresIndices);

        // Calculate the minimum distance
        for (const auto& si : spheres) {
            const auto& s = spheresArray[si];
            float dx = x - s.cx;
            float dy = y - s.cy;
            float dz = z - s.cz;
            float dist = sqrt(dx * dx + dy * dy + dz * dz) - s.radius;
            totalD = std::min(totalD, dist);
        }

        return totalD;
      }
      virtual Eigen::Vector3f fd(float x, float y, float z) {
                // Precompute the indices and distance bounds
        int i = static_cast<int>(std::floor((x - vminx) * invStep));
        int j = static_cast<int>(std::floor((y - vminy) * invStep));
        int k = static_cast<int>(std::floor((z - vminz) * invStep));
        int n = i + j * rx + k * rxy;

        float minDist = 1000000.0f;  // Large initial value
        int minIndex = -1;

        const auto& spheresToCheck = voxelData.spheres.find(n) != voxelData.spheres.end() ? voxelData.spheres[n] : (
          voxelData.secondarySpheres.find(n) != voxelData.secondarySpheres.end() ? voxelData.secondarySpheres[n] : spheresIndices);

        // Iterate over the relevant spheres
        for (const auto si : spheresToCheck) {
            const auto& s = spheresArray[si];
            float dx = x - s.cx;
            float dy = y - s.cy;
            float dz = z - s.cz;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz) - s.radius;
            if (dist < minDist) {
                minDist = dist;
                minIndex = si;
            }
        }

        // Calculate the vector based on the sphere with the minimum distance
        const auto& s = spheresArray[minIndex];
        Eigen::Vector3f v(x - s.cx, y - s.cy, z - s.cz);
        return v;
      }
};


class GridBasedCirclesCFW: public CustomFunctionWrapper{
    public:
    double vminx;
    double vminy;
    double step;
    double invStep;
    int rx;
    int ry;
    PixelData voxelData;
    std::vector<short> spheresIndices;
    std::vector<Circle> circlesArray;

    GridBasedCirclesCFW(std::vector<Circle>& circlesArray): circlesArray(circlesArray){
      std::vector<double> box {-150, -130, 150, 150};
      voxelData = createPixelData(box, 150, 1.51);
      std::unordered_map<int, std::vector<short>>& innerSpheres = voxelData.circles;
      PixalizeCircles(circlesArray, voxelData, innerSpheres, 2);

      std::unordered_map<int, std::vector<short>>& secondarySpheres = voxelData.secondaryCircles;
      PixalizeCircles(circlesArray, voxelData, secondarySpheres, 20);

      vminx = voxelData.min[0];
      vminy = voxelData.min[1];
      step = voxelData.step;
      invStep = 1.0 / step;
      rx = voxelData.dims[0];
      ry = voxelData.dims[1];

      for( int i = 0; i < circlesArray.size(); i++ )
        spheresIndices.push_back( (short)i );
    }
      virtual float f(float x, float y, float z) {
        // Precompute the indices and distance bounds
        int i = static_cast<int>(std::floor((x - vminx) * invStep));
        int j = static_cast<int>(std::floor((y - vminy) * invStep));
        int n = i + j * rx;

        float totalD = 1000000.0f;  // Large initial value
        const auto& spheres = voxelData.circles.find(n) != voxelData.circles.end() ? voxelData.circles[n] : (
          voxelData.secondaryCircles.find(n) != voxelData.secondaryCircles.end() ? voxelData.secondaryCircles[n] : spheresIndices);

        // Calculate the minimum distance
        for (const auto& si : spheres) {
            const auto& s = circlesArray[si];
            float dx = x - s.cx;
            float dy = y - s.cy;
            float dist = sqrt(dx * dx + dy * dy) - s.radius;
            totalD = std::min(totalD, dist);
        }

        return totalD;
      }
      virtual Eigen::Vector3f fd(float x, float y, float z) {
                // Precompute the indices and distance bounds
        int i = static_cast<int>(std::floor((x - vminx) * invStep));
        int j = static_cast<int>(std::floor((y - vminy) * invStep));
        int n = i + j * rx;

        float minDist = 1000000.0f;  // Large initial value
        int minIndex = -1;

        const auto& spheresToCheck = voxelData.circles.find(n) != voxelData.circles.end() ? voxelData.circles[n] : (
          voxelData.secondaryCircles.find(n) != voxelData.secondaryCircles.end() ? voxelData.secondaryCircles[n] : spheresIndices);

        // Iterate over the relevant spheres
        for (const auto si : spheresToCheck) {
            const auto& s = circlesArray[si];
            float dx = x - s.cx;
            float dy = y - s.cy;
            float dist = std::sqrt(dx * dx + dy * dy) - s.radius;
            if (dist < minDist) {
                minDist = dist;
                minIndex = si;
            }
        }

        // Calculate the vector based on the sphere with the minimum distance
        const auto& s = circlesArray[minIndex];
        Eigen::Vector3f v(x - s.cx, y - s.cy, 0);
        return v;
      }
};


Tree buildTree(Node& root) {
    if(root.type == "union"){
      Tree tr = buildTree(root.children[0]);
      for(size_t i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = _union(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "blend"){
      Tree tr = buildTree(root.children[0]);
      for(size_t i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = blend_expt(tr, buildTree(child), 0.75);
      }
      return tr;
    } else if(root.type == "difference"){
      Tree tr = buildTree(root.children[0]);
      for(size_t i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = difference(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "intersection"){
      Tree tr = buildTree(root.children[0]);
      for(size_t i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = intersection(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "offset"){
      Tree tr = buildTree(root.children[0]);
      tr = offset(tr, root.data[0]);
      // auto offs = root.data[0];
      // tr = elongate(tr, {offs, offs, offs});
      return tr;
    } else if(root.type == "limit"){
      Tree tr = buildTree(root.children[0]);
      auto view = root.data[0];
      auto width = root.data[1];
      auto thirdDimension = root.data[2];
      if (view == 0 || view == 3) { // top
        // y 
        tr = intersection(tr, half_space({0,-1,0}, {0, thirdDimension -width/2, 0})); 
        tr = intersection(tr, half_space({0,1,0}, {0, thirdDimension +width/2, 0})); 
      } else if (view == 1 || view == 4) { // front
        // z
        tr = intersection(tr, half_space({0,0,-1}, {0, 0, thirdDimension -width/2})); 
        tr = intersection(tr, half_space({0,0,1}, {0, 0, thirdDimension +width/2})); 
      } else if (view == 2 || view == 5) { // left
        // x
        tr = intersection(tr, half_space({-1,0,0}, {thirdDimension-width/2,0,0})); 
        tr = intersection(tr, half_space({1,0,0}, {thirdDimension+width/2,0,0})); 
      }
      return tr;
    } else if(root.type == "scaleX"){
      Tree tr = buildTree(root.children[0]);
      tr = scale_x(tr, root.data[0], 0);
      return tr;
    } else if(root.type == "scaleY"){
      Tree tr = buildTree(root.children[0]);
      tr = scale_y(tr, root.data[0], 0);
      return tr;
    } else if(root.type == "scaleZ"){
      Tree tr = buildTree(root.children[0]);
      tr = scale_z(tr, root.data[0], 0);
      return tr;
    } else if(root.type == "extrude"){
      Tree tr = buildTree(root.children[0]);
      tr = extrude_z(tr, root.data[0], root.data[1]);
      return tr;
    } else if(root.type == "revolve"){
      Tree tr = buildTree(root.children[0]);
      tr = revolve_y(tr, 0);
      return tr;
    } else if(root.type == "rotate"){
      Tree tr = buildTree(root.children[0]);
      auto view = root.data[0];
      if (view == 0) { // top
        tr = rotate_x(tr, -M_PI/2, {0,0,0});
      } else if (view == 1) { // front
      } else if (view == 2) { // left
        tr = rotate_y(tr, -M_PI/2, {0,0,0});
      } else if (view == 3) { // bottom
        tr = rotate_x(tr, M_PI/2, {0,0,0});
      } else if (view == 4) { // back
        tr = reflect_z(tr, 0);
      } else if (view == 5) { // right
        tr = rotate_y(tr, M_PI/2, {0,0,0});
      }
      return tr;
    } else if(root.type == "extend"){
      Tree tr = buildTree(root.children[0]);
      // tr = elongate(tr, {root.data[1], root.data[2], root.data[3]});
      float dx = 0;
      float dy = 0;
      float dz = 0;
      int nx = 0;
      int ny = 0;
      int nz = 0;
      if(root.data[1] > 0){
        nx = 4;
        dx = 1;
      }else if(root.data[1] < 0){
        nx = 4;
        dx = -1;
      }
      if(root.data[2] > 0){
        ny = 4;
        dy = 1;
      }else if(root.data[2] < 0){
        ny = 4;
        dy = -1;
      }
      if(root.data[3] > 0){
        nz = 4;
        dy = 1;
      }else if(root.data[3] < 0){
        nz = 4;
        dy = -1;
      }
      tr = array_xyz(tr, nx, ny, nz, {dx, dy, dz});
      return tr;
    } else if(root.type == "sphere"){
      return sphere(root.data[0], root.data[1], root.data[2], root.data[3]);
    } else if(root.type == "cylinder"){
      auto radius = root.data[0];
      auto p1x = root.data[1];
      auto p1y = root.data[2];
      auto p1z = root.data[3];
      auto p2x = root.data[4];
      auto p2y = root.data[5];
      auto p2z = root.data[6];
      if(p1z != p2z){//along z
        return cylinder_z(radius, abs(p1z-p2z), {p1x, p1y, min(p1z, p2z)});
      }else if(p1y != p2y){
        return cylinder_y(radius, abs(p1y-p2y), {p1x, min(p1y, p2y), p1z});
      }else{
        return cylinder_x(radius, abs(p1x-p2x), {min(p1x, p2x), p1y, p1z});
      }
    } else if(root.type == "capsule"){
      return capsule(root.data[0], root.data[1], {root.data[2], root.data[3], root.data[4]}, {root.data[5], root.data[6], root.data[7]});
    } else if(root.type == "torus"){
      return torus(root.data[0], root.data[1], root.data[2]);
    } else if(root.type == "circle"){
      return circle(root.data[0], {root.data[1], root.data[2]});
    } else if(root.type == "triangle"){
      return triangle({root.data[0], root.data[1]}, {root.data[2], root.data[3]}, {root.data[4], root.data[5]});
    } else if(root.type == "scale"){
      Tree tr = buildTree(root.children[0]);
      auto scale_x_value = root.data[0];
      auto scale_y_value = root.data[1];
      auto scale_z_value = root.data[2];
      if(scale_x_value != 1){
        tr = scale_x(tr, scale_x_value, 0);  
      }
      if(scale_y_value != 1){
        tr = scale_y(tr, scale_y_value, 0);  
      }
      if(scale_z_value != 1){
        tr = scale_z(tr, scale_z_value, 0);  
      }
      return tr;
    } else if(root.type == "rotation"){
      Tree tr = buildTree(root.children[0]);
      auto rotation_x_value = root.data[0];
      auto rotation_y_value = root.data[1];
      auto rotation_z_value = root.data[2];

      auto center_x = root.data[3];
      auto center_y = root.data[4];
      auto center_z = root.data[5];
      if(rotation_x_value != 0){
        tr = rotate_x(tr, rotation_x_value, {center_x,center_y,center_z});  
      }
      if(rotation_y_value != 0){
        tr = rotate_y(tr, rotation_y_value, {center_x,center_y,center_z});  
      }
      if(rotation_z_value != 0){
        tr = rotate_z(tr, rotation_z_value, {center_x,center_y,center_z});  
      }
      return tr;
    } else if(root.type == "position"){
      Tree tr = buildTree(root.children[0]);
      tr = move(tr, root.data[0], root.data[1], root.data[2]);  
      return tr;
    } else if(root.type == "spheres"){
      std::vector<Sphere> spheresArray;
      for (size_t i = 0; i < root.children.size(); i++)
      {
        auto sph = root.children[i];
        Sphere sp1{(float)sph.data[0], (float)sph.data[1], (float)sph.data[2], (float)sph.data[3]};
        spheresArray.push_back(sp1);
      }

      auto cf = new GridBasedCFW(spheresArray);
      return Tree(customFunction(cf));
     } else if(root.type == "revolveCircles"){
      std::vector<Circle> circleArray;
      for (size_t i = 0; i < root.children.size(); i++)
      {
        auto sph = root.children[i];
        Circle sp1{(float)sph.data[0], (float)sph.data[1], (float)sph.data[2]};
        circleArray.push_back(sp1);
      }

      auto cf = new GridBasedCirclesCFW(circleArray);
      return revolve_y(Tree(customFunction(cf)), 0);
    } else if(root.children.size() > 0){
      return buildTree(root.children[0]);
    }
}

Tree parseImplicitString(std::string str){
    vector<string> words = splitBySpaces(str);
    Node root;
    root.type="root";
    parseNode(&root, words, 0);
    
    // printTree(root);

    return buildTree(root);
}



struct MeshDto {
  unsigned int vpointer;
  unsigned int vsize;
  unsigned int fpointer;
  unsigned int fsize;
};
MeshDto meshImplicitFunction(std::string implicitString, float resolution, float maxError, double minx, double miny, double minz, double maxx, double maxy, double maxz) {

  // std::cout << "Tree: " << libfive_tree_print(&out) << "\n";
   auto out_parse = parseImplicitString(implicitString);
  // std::vector<Sphere> spheresArray;
  // Sphere sp1{10, 0, 0, 0};
  // spheresArray.push_back(sp1);
  // Sphere sp2{10, 20, 0, 0};
  // spheresArray.push_back(sp2);
  // Sphere sp3{10, 0, 20, 0};
  // spheresArray.push_back(sp3);
  // Sphere sp4{10, 0, 0, 20};
  // spheresArray.push_back(sp4);
  // auto out_parse = buildGridBasedSpheres(spheresArray);

  // vector<string> words = splitBySpaces(implicitString);
  // Node root;
  // root.type="root";
  // parseNode(&root, words, 0);
  // std::vector<Sphere> spheresArray;
  // for (size_t i = 0; i < root.children[0].children.size(); i++)
  // {
  //   auto sph = root.children[0].children[i];
  //   Sphere sp1{(float)sph.data[0], (float)sph.data[1], (float)sph.data[2], (float)sph.data[3]};
  //   spheresArray.push_back(sp1);
  // }
  // // auto out_parse = buildGridBasedSpheres(spheresArray);
  // auto out_parse = buildBruteForceSpheres(spheresArray);

  // The value for `max_err` is cargo-culted from its default value.
  Region<3> bds({minx, miny, minz}, {maxx, maxy, maxz});
  auto bds_box = box_mitered({minx+1, miny+1, minz+1}, {maxx-1, maxy-1, maxz-1});
  auto out = intersection(bds_box, out_parse);

  //findBounds(out)

    // using std::chrono::high_resolution_clock;
    // using std::chrono::duration_cast;
    // using std::chrono::duration;
    // using std::chrono::milliseconds;

    // auto t1 = high_resolution_clock::now();
    
    auto mesh = Kernel::Mesh::render(out, bds, 1.0/resolution, maxError, false); 
  
    // auto t2 = high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    // auto ms_int = duration_cast<milliseconds>(t2 - t1);
    // std::cout << "render time: " << ms_int.count() << "ms\n";
    // mesh->saveSTL(OUTPUT_FILENAME);
    // std::cout << "verts: " << mesh->verts.size() << " "<< mesh->verts[1][0] << ", " << mesh->verts[1][1] << ", " << mesh->verts[1][2]<< "\n";

    MeshDto myMesh;
    unsigned int vsize = mesh->verts.size() * 3;
    float * verts = (float*)malloc(vsize * sizeof(float));
    for(auto i=0;i<mesh->verts.size();i++){
      verts[i * 3 + 0]=mesh->verts[i][0];
      verts[i * 3 + 1]=mesh->verts[i][1];
      verts[i * 3 + 2]=mesh->verts[i][2];
    }
    myMesh.vpointer = (unsigned int) verts; 
    myMesh.vsize = vsize;
    
    unsigned int fsize = mesh->branes.size() * 3;
    unsigned int * faces = (unsigned int*)malloc(fsize * sizeof(unsigned int));
    for(auto i=0;i<mesh->branes.size();i++){
      faces[i * 3 + 0]=mesh->branes[i][0];
      faces[i * 3 + 1]=mesh->branes[i][1];
      faces[i * 3 + 2]=mesh->branes[i][2];
    }
    myMesh.fpointer = (unsigned int) faces; 
    myMesh.fsize = fsize;

    return myMesh;
}

EMSCRIPTEN_BINDINGS(my_module) {
   value_array<MeshDto>("MeshDto")
        .element(&MeshDto::vpointer)
        .element(&MeshDto::vsize)
        .element(&MeshDto::fpointer)
        .element(&MeshDto::fsize)
        ;

    emscripten::function("meshImplicitFunction", &meshImplicitFunction);
}
