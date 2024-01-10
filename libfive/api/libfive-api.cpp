#include <emscripten/bind.h>

// #include <iostream>
// #include <iterator>

#include "libfive.h"
#include "libfive/solve/bounds.hpp"
#include "libfive/render/brep/mesh.hpp"

// #include <bits/stdc++.h>
#include <iostream>
using namespace std;

const char *OUTPUT_FILENAME = "exported.stl";
const float OUTPUT_RESOLUTION = 15.0;

using namespace emscripten;
using namespace Kernel;

Kernel::Tree sphereOld(float radius, float cx, float cy, float cz){
  auto x = Kernel::Tree::X() - Kernel::Tree(cx);
  auto y = Kernel::Tree::Y() - Kernel::Tree(cy);
  auto z = Kernel::Tree::Z() - Kernel::Tree(cz);

  auto r = Kernel::Tree(radius);
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

#define LIBFIVE_DEFINE_XYZ() const auto x = Tree::X(); (void)x; \
                             const auto y = Tree::Y(); (void)y; \
                             const auto z = Tree::Z(); (void)z; ;


Tree move(Tree t, float cx, float cy, float cz) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - cx, y - cy, z - cz);
}

Tree sphere(float r, float cx, float cy, float cz) {
    LIBFIVE_DEFINE_XYZ();
    return move(sqrt(square(x) + square(y) + square(z)) - r, cx, cy, cz);
}

Tree extrude_z(Tree t, TreeFloat zmin, TreeFloat zmax) {
    LIBFIVE_DEFINE_XYZ();
    return max(t, max(zmin - z, z - zmax));
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

void printTree(const Node& root, int depth = 0) {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
    std::cout << root.type << " " << root.children.size() << std::endl;

    for (const auto& child : root.children) {
        printTree(child, depth + 1);
    }
}

int parseNode(Node* parentNode, vector<string> words, int i){
    int nextPosition = i;
    Node node;
    string word = words[i];
    if(word == "union" || word == "intersection" || word == "difference"){
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
    }else if(word == ")"){
        nextPosition = i + 1;
    }else{
        if(i < words.size()-1){
            nextPosition = parseNode(parentNode, words, i+1);
        }
    }
    return nextPosition;
}

Tree buildTree(Node& root) {
    if(root.type == "union"){
      Tree tr = buildTree(root.children[0]);
      for(int i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = _union(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "difference"){
      Tree tr = buildTree(root.children[0]);
      for(int i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = difference(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "intersection"){
      Tree tr = buildTree(root.children[0]);
      for(int i = 1; i< root.children.size(); i++){
        auto child = root.children[i];
        tr = intersection(tr, buildTree(child));
      }
      return tr;
    } else if(root.type == "offset"){
      Tree tr = buildTree(root.children[0]);
      tr = offset(tr, root.data[0]);
      return tr;
    } else if(root.type == "extend"){
      Tree tr = buildTree(root.children[0]);
      // tr = offset(tr, root.data[0]);
      return tr;
    } else if(root.type == "sphere"){
      return sphere(root.data[0], root.data[1], root.data[2], root.data[3]);
    }else if(root.children.size() > 0){
      return buildTree(root.children[0]);
    }
}

Tree parseImplicitString(std::string str){
    vector<string> words = splitBySpaces(str);
    Node root;
    root.type="root";
    parseNode(&root, words, 0);
    
    printTree(root);

    return buildTree(root);
}


void meshImplicitFunction(std::string implicitString, float resolution, float maxError) {
    
        // std::string implicitString = "union ( s 3.78 -28.19 50.81 0.00 s 7.01 -28.98 47.60 0.00 s 7.07 -28.96 47.54 0.00 s 6.94 -29.03 47.65 0.00 s 5.98 -29.99 47.99 0.00 s 7.61 -28.71 46.94 0.00 s 7.86 -28.58 46.65 0.00 s 8.00 -28.49 46.47 0.00 s 8.13 -28.39 46.28 0.00 s 8.29 -28.24 46.04 0.00 s 8.40 -28.14 45.87 0.00 s 8.68 -27.73 45.28 0.00 s 8.93 -27.40 44.72 0.00 s 9.11 -27.02 44.14 0.00 s 9.34 -26.56 43.38 0.00 s 9.50 -26.08 42.62 0.00 s 9.68 -25.53 41.72 0.00 s 9.91 -24.72 40.46 0.00 s 10.11 -23.88 39.03 0.00 s 10.31 -23.02 37.58 0.00 s 10.57 -22.00 35.87 0.00 s 10.76 -21.04 34.16 0.00 s 10.96 -20.09 32.45 0.00 s 11.12 -19.16 30.72 0.00 s 11.27 -18.25 28.99 0.00 s 11.38 -17.37 27.24 0.00 s 12.24 -14.84 19.93 0.00 s 4.90 -25.45 16.30 0.00 s 7.03 -24.80 14.10 0.00 s 6.94 -25.71 13.54 0.00 s 6.80 -27.21 12.51 0.00 s 6.70 -28.76 11.37 0.00 s 6.69 -30.26 10.17 0.00 s 6.70 -31.75 8.95 0.00 s 6.76 -33.21 7.69 0.00 s 6.83 -34.66 6.43 0.00 s 6.90 -36.11 5.16 0.00 s 6.96 -37.51 3.95 0.00 s 6.98 -38.90 2.75 0.00 s 7.04 -40.32 1.47 0.00 s 7.11 -41.73 0.20 0.00 s 7.17 -43.15 -1.08 0.00 s 7.22 -44.57 -2.34 0.00 s 7.26 -46.01 -3.62 0.00 s 7.25 -47.16 -4.56 0.00 s 7.26 -48.26 -5.49 0.00 s 7.22 -49.24 -6.25 0.00 s 7.11 -49.92 -6.76 0.00 s 7.02 -50.51 -7.17 0.00 s 6.86 -51.04 -7.59 0.00 s 6.44 -51.78 -7.82 0.00 s 6.10 -52.18 -7.82 0.00 s 3.22 -54.88 -6.73 0.00 s 2.79 -55.57 -8.06 0.00 s 5.14 -53.21 -7.85 0.00 s 5.54 -52.80 -7.84 0.00 s 6.04 -52.25 -7.83 0.00 s 6.43 -51.80 -7.82 0.00 s 5.95 -51.67 -8.65 0.00 s 5.50 -51.80 -9.09 0.00 s 5.95 -51.67 -8.65 0.00 s 6.37 -51.55 -8.22 0.00 s 6.79 -51.23 -7.71 0.00 s 7.00 -50.58 -7.23 0.00 s 7.14 -49.69 -6.57 0.00 s 7.24 -48.69 -5.80 0.00 s 7.25 -47.41 -4.76 0.00 s 7.26 -46.09 -3.69 0.00 s 7.23 -44.87 -2.60 0.00 s 7.18 -43.49 -1.37 0.00 s 7.12 -42.11 -0.12 0.00 s 7.06 -40.68 1.17 0.00 s 6.99 -39.18 2.52 0.00 s 6.98 -38.02 3.55 0.00 s 6.95 -36.95 4.48 0.00 s 6.89 -35.93 5.32 0.00 s 6.84 -34.93 6.20 0.00 s 6.81 -33.88 7.15 0.00 s 6.74 -32.72 8.16 0.00 s 6.70 -31.79 8.92 0.00 s 6.70 -30.85 9.74 0.00 s 6.69 -29.87 10.50 0.00 s 6.70 -28.83 11.32 0.00 s 6.78 -27.78 12.11 0.00 s 6.85 -26.65 12.94 0.00 s 6.97 -25.46 13.70 0.00 s 12.18 -13.87 17.64 0.00 s 11.76 -9.68 10.52 0.00 s 12.17 -8.65 8.96 0.00 s 12.64 -7.49 7.26 0.00 s 13.16 -6.48 5.93 0.00 s 13.69 -5.54 4.83 0.00 s 14.51 -4.19 3.40 0.00 s 13.91 -4.31 1.62 0.00 s 9.83 -7.60 -3.84 0.00 s 8.16 -10.15 -7.46 0.00 s 7.45 -11.50 -9.07 0.00 s 7.07 -12.46 -10.56 0.00 s 6.91 -13.25 -12.19 0.00 s 6.92 -13.87 -13.83 0.00 s 7.24 -15.66 -17.90 0.00 s 7.49 -21.76 -20.76 0.00 s 2.85 -23.45 -15.94 0.00 s 4.34 -23.16 -17.50 0.00 s 5.45 -22.96 -18.66 0.00 s 5.99 -22.78 -19.24 0.00 s 5.61 -23.16 -19.09 0.00 s 5.47 -23.67 -19.98 0.00 s 7.82 -21.49 -21.07 0.00 s 8.77 -20.74 -21.94 0.00 s 9.54 -20.09 -22.83 0.00 s 9.43 -20.34 -23.69 0.00 s 9.28 -20.57 -24.60 0.00 s 9.12 -20.81 -25.50 0.00 s 8.83 -21.18 -27.05 0.00 s 8.54 -21.55 -28.79 0.00 s 8.24 -21.92 -30.53 0.00 s 7.87 -22.40 -32.75 0.00 s 7.58 -23.07 -35.10 0.00 s 7.28 -23.67 -36.97 0.00 s 7.09 -23.98 -37.85 0.00 s 6.96 -24.20 -38.38 0.00 s 6.38 -24.85 -39.49 0.00 s 5.57 -25.61 -40.05 0.00 s 4.79 -26.30 -40.46 0.00 s 3.43 -26.65 -41.94 0.00 s 5.86 -25.36 -39.85 0.00 s 6.31 -24.91 -39.55 0.00 s 6.11 -24.49 -39.86 0.00 s 5.68 -24.39 -40.29 0.00 s 6.01 -24.45 -39.95 0.00 s 6.30 -24.53 -39.66 0.00 s 6.67 -24.58 -39.23 0.00 s 6.84 -24.37 -38.78 0.00 s 7.03 -24.09 -38.11 0.00 s 7.21 -23.81 -37.33 0.00 s 7.42 -23.46 -36.18 0.00 s 7.59 -23.04 -35.01 0.00 s 7.75 -22.72 -33.83 0.00 s 7.92 -22.33 -32.44 0.00 s 8.19 -21.99 -30.82 0.00 s 8.48 -21.63 -29.12 0.00 s 8.79 -21.23 -27.26 0.00 s 9.12 -20.82 -25.54 0.00 s 9.38 -20.43 -24.00 0.00 s 9.57 -20.04 -22.64 0.00 s 8.98 -19.04 -21.74 0.00 s 8.32 -17.92 -20.72 0.00 s 7.85 -17.02 -19.76 0.00 s 7.38 -16.07 -18.51 0.00 s 7.07 -15.22 -17.10 0.00 s 6.94 -14.55 -15.62 0.00 s 6.92 -13.98 -14.10 0.00 s 7.17 -12.20 -10.10 0.00 s 6.93 -8.06 -7.98 0.00 s 3.43 -6.14 -11.07 0.00 s 6.79 -7.99 -8.10 0.00 s 10.79 -6.66 -2.25 0.00 s 14.73 -1.68 2.60 0.00 s 14.64 -0.17 2.56 0.00 s 13.13 4.86 0.66 0.00 s 9.65 7.86 -4.62 0.00 s 9.10 8.70 -5.84 0.00 s 2.80 6.27 -11.85 0.00 s 7.21 8.13 -7.77 0.00 s 7.11 12.34 -10.35 0.00 s 6.92 13.97 -14.05 0.00 s 6.93 14.54 -15.57 0.00 s 7.04 15.21 -17.04 0.00 s 7.35 16.07 -18.45 0.00 s 7.82 17.05 -19.74 0.00 s 8.33 17.99 -20.76 0.00 s 9.00 19.12 -21.78 0.00 s 9.54 20.05 -22.69 0.00 s 9.36 20.44 -24.04 0.00 s 9.10 20.83 -25.58 0.00 s 8.78 21.23 -27.29 0.00 s 8.48 21.61 -29.09 0.00 s 8.21 21.96 -30.74 0.00 s 7.94 22.30 -32.32 0.00 s 7.76 22.67 -33.70 0.00 s 7.62 22.99 -34.86 0.00 s 7.42 23.36 -36.04 0.00 s 7.24 23.72 -37.17 0.00 s 7.05 24.00 -37.97 0.00 s 6.86 24.28 -38.67 0.00 s 6.63 24.46 -39.20 0.00 s 6.44 24.55 -39.52 0.00 s 6.15 24.48 -39.82 0.00 s 5.73 24.38 -40.22 0.00 s 6.03 24.46 -39.93 0.00 s 6.28 24.81 -39.63 0.00 s 5.77 25.33 -39.98 0.00 s 5.34 25.76 -40.25 0.00 s 2.88 27.24 -42.24 0.00 s 5.23 25.86 -40.30 0.00 s 4.62 26.51 -40.17 0.00 s 6.55 24.54 -39.38 0.00 s 5.95 25.17 -38.28 0.00 s 7.07 23.98 -37.91 0.00 s 7.27 23.66 -37.01 0.00 s 7.47 23.27 -35.77 0.00 s 7.68 22.85 -34.45 0.00 s 7.89 22.37 -32.66 0.00 s 8.24 21.92 -30.56 0.00 s 8.52 21.56 -28.84 0.00 s 8.81 21.19 -27.12 0.00 s 9.10 20.83 -25.58 0.00 s 9.26 20.58 -24.67 0.00 s 9.40 20.36 -23.75 0.00 s 9.52 20.10 -22.87 0.00 s 9.07 20.44 -22.02 0.00 s 8.22 21.10 -21.19 0.00 s 5.62 23.52 -20.05 0.00 s 4.60 24.02 -18.57 0.00 s 7.01 22.00 -19.90 0.00 s 6.99 22.01 -19.88 0.00 s 6.91 22.05 -19.80 0.00 s 5.11 22.92 -18.22 0.00 s 6.95 22.03 -19.84 0.00 s 4.82 21.85 -17.71 0.00 s 7.31 15.96 -18.31 0.00 s 6.92 13.88 -13.82 0.00 s 6.91 13.26 -12.19 0.00 s 7.07 12.46 -10.56 0.00 s 7.46 11.49 -9.06 0.00 s 8.16 10.17 -7.46 0.00 s 10.03 7.40 -3.88 0.00 s 13.95 4.27 1.63 0.00 s 14.38 4.31 3.37 0.00 s 13.52 5.70 4.78 0.00 s 12.97 6.66 5.87 0.00 s 12.42 7.70 7.19 0.00 s 11.92 8.89 8.89 0.00 s 11.62 9.83 10.49 0.00 s 12.19 13.88 17.66 0.00 s 6.96 25.47 13.70 0.00 s 6.84 26.66 12.94 0.00 s 6.77 27.79 12.11 0.00 s 6.69 28.84 11.32 0.00 s 6.69 29.88 10.50 0.00 s 6.69 30.86 9.74 0.00 s 6.70 31.80 8.92 0.00 s 6.74 32.73 8.15 0.00 s 6.81 33.88 7.15 0.00 s 6.84 34.94 6.20 0.00 s 6.89 35.94 5.32 0.00 s 6.95 36.96 4.47 0.00 s 6.98 38.02 3.55 0.00 s 6.99 39.18 2.52 0.00 s 7.06 40.69 1.16 0.00 s 7.12 42.12 -0.13 0.00 s 7.18 43.51 -1.39 0.00 s 7.23 44.89 -2.61 0.00 s 7.26 46.10 -3.70 0.00 s 7.25 47.41 -4.76 0.00 s 7.24 48.67 -5.79 0.00 s 7.15 49.66 -6.56 0.00 s 7.01 50.54 -7.20 0.00 s 6.80 51.18 -7.68 0.00 s 6.46 51.52 -8.12 0.00 s 6.04 51.63 -8.54 0.00 s 5.58 51.76 -8.99 0.00 s 6.03 51.64 -8.55 0.00 s 6.38 51.86 -7.83 0.00 s 5.97 52.34 -7.84 0.00 s 5.47 52.89 -7.86 0.00 s 5.07 53.30 -7.88 0.00 s 2.76 55.62 -8.11 0.00 s 3.21 54.91 -6.77 0.00 s 6.06 52.23 -7.84 0.00 s 6.41 51.82 -7.83 0.00 s 6.85 51.05 -7.60 0.00 s 7.01 50.51 -7.18 0.00 s 7.11 49.91 -6.76 0.00 s 7.22 49.23 -6.24 0.00 s 7.26 48.26 -5.49 0.00 s 7.25 47.15 -4.56 0.00 s 7.26 46.01 -3.62 0.00 s 7.22 44.57 -2.35 0.00 s 7.17 43.14 -1.08 0.00 s 7.11 41.73 0.20 0.00 s 7.04 40.32 1.47 0.00 s 6.98 38.90 2.75 0.00 s 6.96 37.51 3.95 0.00 s 6.90 36.11 5.16 0.00 s 6.83 34.66 6.43 0.00 s 6.76 33.21 7.69 0.00 s 6.69 31.76 8.95 0.00 s 6.69 30.27 10.17 0.00 s 6.69 28.77 11.38 0.00 s 6.79 27.21 12.51 0.00 s 6.94 25.63 13.60 0.00 s 7.05 24.78 14.16 0.00 s 11.59 16.36 18.48 0.00 s 11.75 17.39 26.51 0.00 s 11.59 18.22 28.29 0.00 s 11.40 19.07 30.06 0.00 s 11.20 19.94 31.82 0.00 s 10.94 20.85 33.57 0.00 s 10.69 21.76 35.31 0.00 s 10.38 22.78 37.20 0.00 s 10.08 23.84 39.02 0.00 s 9.81 24.81 40.69 0.00 s 9.54 25.66 42.04 0.00 s 9.29 26.27 43.17 0.00 s 8.94 27.01 44.33 0.00 s 8.59 27.59 45.26 0.00 s 8.18 28.14 46.15 0.00 s 7.65 28.73 46.88 0.00 s 7.12 29.24 47.45 0.00 s 6.52 29.78 47.87 0.00 s 6.18 30.06 48.10 0.00 s 5.75 30.41 48.36 0.00 s 6.03 30.17 48.20 0.00 s 6.65 29.67 47.78 0.00 s 5.14 29.20 49.48 0.00 s 4.03 29.37 50.58 0.00 s 4.53 28.26 50.06 0.00 s 7.78 28.60 46.69 0.00 s 5.82 27.29 48.37 0.00 s 6.12 26.42 47.50 0.00 s 8.61 27.55 45.20 0.00 s 8.84 27.21 44.66 0.00 s 9.00 26.89 44.13 0.00 s 8.61 26.25 44.08 0.00 s 9.49 25.80 42.27 0.00 s 9.77 24.95 40.92 0.00 s 10.01 24.11 39.46 0.00 s 10.29 23.11 37.73 0.00 s 10.57 22.28 36.25 0.00 s 10.77 21.49 34.75 0.00 s 11.03 20.56 32.97 0.00 s 11.25 19.74 31.40 0.00 s 11.37 19.21 30.34 0.00 s 11.52 18.66 29.17 0.00 s 11.61 18.10 28.01 0.00 s 11.72 17.56 26.85 0.00 s 11.82 17.05 25.69 0.00 s 11.89 16.53 24.55 0.00 s 11.99 16.08 23.38 0.00 s 12.13 15.74 22.32 0.00 s 12.39 15.36 20.74 0.00 s 12.64 14.88 18.88 0.00 s 12.05 13.56 17.22 0.00 s 11.66 12.37 15.47 0.00 s 11.45 11.32 13.57 0.00 s 14.63 0.31 2.56 0.00 s 11.54 -10.43 11.83 0.00 s 11.66 -12.37 15.48 0.00 s 12.05 -13.56 17.22 0.00 s 12.47 -14.73 18.95 0.00 s 12.02 -15.02 20.88 0.00 s 11.75 -15.39 22.47 0.00 s 11.60 -15.74 23.55 0.00 s 11.50 -16.18 24.73 0.00 s 11.44 -16.72 25.86 0.00 s 11.39 -17.26 27.01 0.00 s 11.33 -17.86 28.14 0.00 s 11.25 -18.42 29.31 0.00 s 11.15 -19.03 30.46 0.00 s 11.07 -19.59 31.50 0.00 s 10.89 -20.45 33.04 0.00 s 10.69 -21.42 34.79 0.00 s 10.52 -22.24 36.28 0.00 s 10.29 -23.11 37.73 0.00 s 10.04 -24.14 39.44 0.00 s 9.83 -25.03 40.93 0.00 s 9.60 -25.78 42.11 0.00 s 9.37 -26.49 43.27 0.00 s 9.11 -27.03 44.15 0.00 s 8.82 -27.54 44.96 0.00 s 8.51 -27.97 45.64 0.00 s 8.19 -28.34 46.20 0.00 s 7.71 -28.66 46.82 0.00 s 3.90 -29.09 50.76 0.00 )";
  // std::cout << "libfive Revision: " << libfive_git_branch() << " " << libfive_git_version() << " " << libfive_git_revision() << "\n";

  // std::cout << "received implicit string: "<< implicitString<<"\n"; 

  // auto sphere1 = sphere(2, 0,0,0);
  // auto sphere2 = sphere(2, 1,0,0);
  // libfive_tree libfive_tree_binary(int op, libfive_tree a, libfive_tree b);
// auto spheres = sphere(2.51, -25.26, 22.72, 22.67);
// spheres = min(spheres, sphere(6.85, -25.81, 18.41, 23.85));
// spheres = min(spheres, sphere(6.67, -25.85, 18.58, 23.79));
// spheres = min(spheres, sphere(6.96, -25.79, 18.30, 23.89));
// spheres = min(spheres, sphere(6.67, -25.95, 18.54, 23.79));
// spheres = min(spheres, sphere(6.99, -25.78, 18.27, 23.90));
// spheres = min(spheres, sphere(6.67, -26.05, 18.46, 23.80));
// spheres = min(spheres, sphere(6.97, -25.81, 18.27, 23.89));
// spheres = min(spheres, sphere(6.58, -26.18, 18.41, 23.79));
// spheres = min(spheres, sphere(6.84, -25.93, 18.31, 23.86));
// spheres = min(spheres, sphere(7.02, -25.77, 18.23, 23.91));
// spheres = min(spheres, sphere(7.10, -25.68, 18.10, 23.97));
// spheres = min(spheres, sphere(7.94, -24.62, 16.62, 24.63));
// spheres = min(spheres, sphere(9.20, -23.11, 14.87, 25.30));
// spheres = min(spheres, sphere(10.37, -21.68, 13.11, 25.95));
// spheres = min(spheres, sphere(11.16, -20.63, 11.29, 26.43));
// spheres = min(spheres, sphere(11.77, -19.83, 9.89, 26.81));
// spheres = min(spheres, sphere(12.21, -19.13, 8.78, 27.20));
// spheres = min(spheres, sphere(12.81, -18.17, 7.28, 27.84));
// spheres = min(spheres, sphere(13.29, -17.40, 6.06, 28.44));
// spheres = min(spheres, sphere(13.80, -16.57, 4.75, 29.06));
// spheres = min(spheres, sphere(14.13, -15.93, 3.41, 29.59));
// spheres = min(spheres, sphere(14.35, -15.49, 2.49, 29.94));
// spheres = min(spheres, sphere(14.55, -15.04, 1.67, 30.33));
// spheres = min(spheres, sphere(14.86, -14.41, 0.61, 30.92));
// spheres = min(spheres, sphere(15.20, -13.72, -0.42, 31.53));
// spheres = min(spheres, sphere(15.64, -12.89, -1.57, 32.20));
// spheres = min(spheres, sphere(16.17, -11.85, -3.11, 32.94));
// spheres = min(spheres, sphere(16.73, -10.78, -4.90, 33.63));
// spheres = min(spheres, sphere(14.39, -12.78, -6.55, 33.30));
// spheres = min(spheres, sphere(17.22, -9.86, -6.23, 34.08));
// spheres = min(spheres, sphere(17.70, -8.94, -7.38, 34.43));
// spheres = min(spheres, sphere(18.29, -7.86, -8.62, 34.75));
// spheres = min(spheres, sphere(19.02, -6.62, -9.94, 35.02));
// spheres = min(spheres, sphere(19.77, -5.41, -11.12, 35.30));
// spheres = min(spheres, sphere(10.35, -13.92, -15.35, 32.94));
// spheres = min(spheres, sphere(7.85, -16.19, -16.41, 31.84));
// spheres = min(spheres, sphere(16.97, -7.92, -12.45, 34.88));
// spheres = min(spheres, sphere(20.58, -3.81, -12.33, 35.56));
// spheres = min(spheres, sphere(21.59, -2.03, -13.60, 35.72));
// spheres = min(spheres, sphere(22.17, -1.05, -14.26, 35.74));
// spheres = min(spheres, sphere(22.70, -0.00, -15.17, 35.72));
// spheres = min(spheres, sphere(21.84, 0.00, -17.09, 35.55));
// spheres = min(spheres, sphere(21.06, -0.00, -18.96, 35.30));
// spheres = min(spheres, sphere(20.65, 0.00, -19.96, 35.12));
// spheres = min(spheres, sphere(19.15, -0.66, -21.99, 34.67));
// spheres = min(spheres, sphere(14.31, -4.07, -26.02, 33.13));
// spheres = min(spheres, sphere(8.46, -8.67, -30.05, 30.32));
// spheres = min(spheres, sphere(6.05, -10.63, -31.52, 28.81));
// spheres = min(spheres, sphere(7.84, -9.13, -30.50, 29.92));
// spheres = min(spheres, sphere(6.37, -10.12, -31.59, 28.95));
// spheres = min(spheres, sphere(8.60, -8.57, -29.96, 30.40));
// spheres = min(spheres, sphere(11.94, -5.94, -27.64, 32.17));
// spheres = min(spheres, sphere(16.40, -2.47, -24.54, 33.82));
// spheres = min(spheres, sphere(12.14, -2.97, -29.24, 31.81));
// spheres = min(spheres, sphere(7.19, -4.66, -34.18, 28.83));
// spheres = min(spheres, sphere(6.77, -4.82, -34.59, 28.56));
// spheres = min(spheres, sphere(6.86, -4.78, -34.50, 28.63));
// spheres = min(spheres, sphere(8.00, -4.36, -33.41, 29.31));
// spheres = min(spheres, sphere(8.11, -3.14, -33.45, 29.45));
// spheres = min(spheres, sphere(6.87, -2.98, -34.71, 28.74));
// spheres = min(spheres, sphere(6.66, -2.93, -34.92, 28.62));
// spheres = min(spheres, sphere(14.54, -2.45, -26.72, 33.00));
// spheres = min(spheres, sphere(13.89, 2.57, -27.42, 32.71));
// spheres = min(spheres, sphere(6.96, 2.99, -34.63, 28.79));
// spheres = min(spheres, sphere(6.62, 2.99, -34.97, 28.59));
// spheres = min(spheres, sphere(8.04, 3.11, -33.53, 29.41));
// spheres = min(spheres, sphere(8.17, 4.30, -33.25, 29.42));
// spheres = min(spheres, sphere(6.30, 4.99, -35.02, 28.28));
// spheres = min(spheres, sphere(6.96, 4.75, -34.40, 28.69));
// spheres = min(spheres, sphere(7.44, 4.57, -33.95, 28.98));
// spheres = min(spheres, sphere(12.17, 2.95, -29.22, 31.82));
// spheres = min(spheres, sphere(16.40, 2.47, -24.54, 33.82));
// spheres = min(spheres, sphere(11.94, 5.94, -27.64, 32.17));
// spheres = min(spheres, sphere(8.49, 8.65, -30.03, 30.34));
// spheres = min(spheres, sphere(6.61, 9.93, -31.44, 29.11));
// spheres = min(spheres, sphere(7.41, 9.41, -30.83, 29.64));
// spheres = min(spheres, sphere(5.83, 10.90, -31.51, 28.71));
// spheres = min(spheres, sphere(8.22, 8.86, -30.22, 30.17));
// spheres = min(spheres, sphere(14.30, 4.07, -26.03, 33.13));
// spheres = min(spheres, sphere(19.04, 0.72, -22.12, 34.63));
// spheres = min(spheres, sphere(20.53, -0.00, -20.22, 35.06));
// spheres = min(spheres, sphere(21.02, -0.00, -19.05, 35.28));
// spheres = min(spheres, sphere(21.83, -0.00, -17.11, 35.55));
// spheres = min(spheres, sphere(22.68, -0.00, -15.22, 35.71));
// spheres = min(spheres, sphere(22.18, 1.03, -14.26, 35.74));
// spheres = min(spheres, sphere(21.56, 2.06, -13.58, 35.71));
// spheres = min(spheres, sphere(20.57, 3.81, -12.33, 35.56));
// spheres = min(spheres, sphere(17.36, 7.54, -12.35, 34.96));
// spheres = min(spheres, sphere(11.53, 12.89, -14.78, 33.37));
// spheres = min(spheres, sphere(6.72, 17.23, -16.88, 31.27));
// spheres = min(spheres, sphere(19.79, 5.32, -11.32, 35.33));
// spheres = min(spheres, sphere(19.06, 6.55, -10.01, 35.04));
// spheres = min(spheres, sphere(18.25, 7.92, -8.55, 34.73));
// spheres = min(spheres, sphere(17.53, 9.27, -6.99, 34.31));
// spheres = min(spheres, sphere(13.40, 13.59, -7.27, 33.11));
// spheres = min(spheres, sphere(17.00, 10.28, -5.66, 33.89));
// spheres = min(spheres, sphere(16.58, 11.04, -4.48, 33.48));
// spheres = min(spheres, sphere(16.13, 11.93, -2.96, 32.88));
// spheres = min(spheres, sphere(15.43, 13.27, -1.05, 31.90));
// spheres = min(spheres, sphere(14.85, 14.40, 0.60, 30.92));
// spheres = min(spheres, sphere(14.59, 14.97, 1.54, 30.41));
// spheres = min(spheres, sphere(14.46, 15.28, 2.08, 30.12));
// spheres = min(spheres, sphere(14.17, 15.85, 3.26, 29.65));
// spheres = min(spheres, sphere(13.79, 16.59, 4.77, 29.05));
// spheres = min(spheres, sphere(13.30, 17.39, 6.05, 28.45));
// spheres = min(spheres, sphere(12.80, 18.20, 7.31, 27.82));
// spheres = min(spheres, sphere(12.31, 18.99, 8.58, 27.28));
// spheres = min(spheres, sphere(12.00, 19.50, 9.34, 27.00));
// spheres = min(spheres, sphere(11.36, 20.34, 10.81, 26.57));
// spheres = min(spheres, sphere(10.57, 21.41, 12.66, 26.07));
// spheres = min(spheres, sphere(9.52, 22.73, 14.48, 25.45));
// spheres = min(spheres, sphere(8.17, 24.34, 16.26, 24.79));
// spheres = min(spheres, sphere(7.23, 25.53, 17.88, 24.07));
// spheres = min(spheres, sphere(7.02, 25.76, 18.23, 23.93));
// spheres = min(spheres, sphere(6.79, 25.99, 18.25, 23.88));
// spheres = min(spheres, sphere(6.46, 26.29, 18.42, 23.76));
// spheres = min(spheres, sphere(6.99, 25.79, 18.26, 23.91));
// spheres = min(spheres, sphere(6.75, 25.97, 18.42, 23.83));
// spheres = min(spheres, sphere(6.79, 25.94, 18.40, 23.85));
// spheres = min(spheres, sphere(6.97, 25.80, 18.28, 23.90));
// spheres = min(spheres, sphere(6.84, 25.83, 18.41, 23.86));
// spheres = min(spheres, sphere(6.63, 25.90, 18.61, 23.79));
// spheres = min(spheres, sphere(6.99, 25.78, 18.27, 23.91));
// spheres = min(spheres, sphere(6.88, 25.76, 18.38, 23.88));
// spheres = min(spheres, sphere(6.79, 25.73, 18.47, 23.86));
// spheres = min(spheres, sphere(6.94, 25.76, 18.32, 23.90));
// spheres = min(spheres, sphere(7.22, 25.53, 17.89, 24.07));
// spheres = min(spheres, sphere(9.03, 23.33, 15.08, 25.23));
// spheres = min(spheres, sphere(6.58, 20.63, 17.40, 24.64));
// spheres = min(spheres, sphere(7.05, 20.64, 16.92, 24.80));
// spheres = min(spheres, sphere(6.63, 20.48, 17.32, 24.66));
// spheres = min(spheres, sphere(6.86, 20.56, 17.10, 24.74));
// spheres = min(spheres, sphere(7.04, 20.60, 16.91, 24.80));
// spheres = min(spheres, sphere(7.28, 20.72, 16.68, 24.88));
// spheres = min(spheres, sphere(10.35, 21.73, 13.23, 25.89));
// spheres = min(spheres, sphere(11.36, 20.34, 10.81, 26.57));
// spheres = min(spheres, sphere(12.07, 19.38, 9.16, 27.07));
// spheres = min(spheres, sphere(12.69, 18.38, 7.59, 27.70));
// spheres = min(spheres, sphere(13.37, 17.28, 5.87, 28.53));
// spheres = min(spheres, sphere(11.15, 14.69, 6.65, 28.66));
// spheres = min(spheres, sphere(7.72, 12.32, 9.15, 27.73));
// spheres = min(spheres, sphere(13.93, 16.33, 4.20, 29.28));
// spheres = min(spheres, sphere(14.74, 14.65, 0.99, 30.71));
// spheres = min(spheres, sphere(15.85, 12.50, -2.14, 32.49));
// spheres = min(spheres, sphere(13.18, 10.13, -0.55, 32.71));
// spheres = min(spheres, sphere(13.48, 10.32, -0.78, 32.73));
// spheres = min(spheres, sphere(22.81, 0.00, -14.92, 35.73));
// spheres = min(spheres, sphere(13.48, -10.32, -0.78, 32.73));
// spheres = min(spheres, sphere(13.18, -10.13, -0.55, 32.71));
// spheres = min(spheres, sphere(15.85, -12.50, -2.14, 32.49));
// spheres = min(spheres, sphere(14.76, -14.61, 0.92, 30.73));
// spheres = min(spheres, sphere(13.97, -16.27, 4.08, 29.32));
// spheres = min(spheres, sphere(10.60, -14.37, 7.11, 28.51));
// spheres = min(spheres, sphere(8.30, -12.76, 8.77, 27.91));
// spheres = min(spheres, sphere(13.68, -16.78, 5.07, 28.91));
// spheres = min(spheres, sphere(13.19, -17.57, 6.32, 28.30));
// spheres = min(spheres, sphere(12.72, -18.32, 7.50, 27.74));
// spheres = min(spheres, sphere(12.11, -19.28, 9.01, 27.11));
// spheres = min(spheres, sphere(11.46, -20.26, 10.60, 26.60));
// spheres = min(spheres, sphere(10.36, -21.70, 13.15, 25.94));
// spheres = min(spheres, sphere(7.17, -20.68, 16.80, 24.80));
// spheres = min(spheres, sphere(7.10, -20.65, 16.87, 24.78));
// spheres = min(spheres, sphere(6.60, -20.41, 17.32, 24.63));
// spheres = min(spheres, sphere(6.99, -20.61, 16.98, 24.75));
// spheres = min(spheres, sphere(6.85, -20.61, 17.13, 24.70));
// spheres = min(spheres, sphere(6.91, -20.62, 17.07, 24.72));
// spheres = min(spheres, sphere(8.82, -23.58, 15.35, 25.10));
// spheres = min(spheres, sphere(7.11, -25.66, 18.08, 23.98));
// spheres = min(spheres, sphere(6.63, -25.66, 18.61, 23.81));
// spheres = min(spheres, sphere(2.31, -25.24, 22.92, 22.61));

// auto parentSpheres = sphere(8.92, 18.85, 32.44, 0.00);
// parentSpheres = min(parentSpheres, sphere(6.41, 18.05, 34.96, 0.00));
// parentSpheres = min(parentSpheres, sphere(15.04, 18.93, 26.28, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.14, 16.34, 29.66, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.52, 14.05, 32.83, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.46, 13.54, 33.80, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.87, 13.69, 33.42, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.83, 13.64, 33.44, 0.00));
// parentSpheres = min(parentSpheres, sphere(4.99, 12.31, 34.72, 0.00));
// parentSpheres = min(parentSpheres, sphere(10.09, 15.71, 30.60, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.86, 18.65, 26.39, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.30, 20.47, 22.92, 0.00));
// parentSpheres = min(parentSpheres, sphere(21.57, 22.15, 19.54, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.38, 22.61, 16.97, 0.00));
// parentSpheres = min(parentSpheres, sphere(12.35, 12.72, 22.04, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.76, 22.55, 16.14, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.27, 22.30, 14.54, 0.00));
// parentSpheres = min(parentSpheres, sphere(32.88, -6.73, -7.94, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.09, -22.39, 15.16, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.57, -16.07, 19.37, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.69, -22.57, 16.28, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.61, -22.70, 18.45, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.33, -20.99, 21.83, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.07, -19.30, 25.20, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.75, -16.71, 29.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.57, -14.10, 32.80, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.08, -13.81, 33.23, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.56, -13.49, 33.66, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.75, -13.62, 33.52, 0.00));
// parentSpheres = min(parentSpheres, sphere(5.19, -12.97, 34.94, 0.00));
// parentSpheres = min(parentSpheres, sphere(9.85, -15.46, 30.76, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.30, -18.25, 26.88, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.91, -18.18, 32.43, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.30, -17.95, 35.07, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.77, -18.59, 27.53, 0.00));
// parentSpheres = min(parentSpheres, sphere(17.48, -20.00, 23.73, 0.00));
// parentSpheres = min(parentSpheres, sphere(20.25, -21.46, 20.88, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.30, -22.96, 18.75, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.50, -24.63, 29.49, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.68, -24.29, 27.33, 0.00));
// parentSpheres = min(parentSpheres, sphere(21.97, -23.17, 19.05, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.47, -25.93, 24.03, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.78, -28.95, 31.21, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.72, -29.71, 33.13, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.83, -28.93, 31.16, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.38, -25.16, 22.26, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.83, -29.06, 25.20, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.25, -33.23, 29.19, 0.00));
// parentSpheres = min(parentSpheres, sphere(5.81, -35.03, 30.90, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.91, -34.22, 30.14, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.01, -33.41, 29.36, 0.00));
// parentSpheres = min(parentSpheres, sphere(12.92, -29.73, 25.85, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.88, -25.28, 21.60, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.62, -22.76, 18.44, 0.00));
// parentSpheres = min(parentSpheres, sphere(12.38, -32.10, 22.76, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.21, -22.64, 17.29, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.50, -26.83, 16.91, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.90, -37.24, 19.50, 0.00));
// parentSpheres = min(parentSpheres, sphere(5.38, -40.65, 20.46, 0.00));
// parentSpheres = min(parentSpheres, sphere(15.90, -30.34, 17.87, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.10, -22.39, 15.12, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.65, -22.04, 13.20, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.24, -21.65, 11.27, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.91, -21.19, 9.33, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.88, -21.42, 7.47, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.40, -31.05, 6.63, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.86, -38.72, 5.90, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.75, -40.84, 5.72, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.55, -41.04, 5.70, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.52, -32.96, 6.49, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.77, -23.57, 7.30, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.86, -20.09, 6.41, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.79, -26.66, 3.56, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.13, -19.43, 5.31, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.55, -18.36, 3.70, 0.00));
// parentSpheres = min(parentSpheres, sphere(28.03, -17.22, 2.12, 0.00));
// parentSpheres = min(parentSpheres, sphere(28.58, -16.02, 0.57, 0.00));
// parentSpheres = min(parentSpheres, sphere(29.12, -14.92, -0.77, 0.00));
// parentSpheres = min(parentSpheres, sphere(29.58, -14.00, -1.81, 0.00));
// parentSpheres = min(parentSpheres, sphere(30.18, -12.84, -3.07, 0.00));
// parentSpheres = min(parentSpheres, sphere(30.82, -11.63, -4.30, 0.00));
// parentSpheres = min(parentSpheres, sphere(20.95, -20.15, -9.55, 0.00));
// parentSpheres = min(parentSpheres, sphere(10.97, -28.92, -14.46, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.43, -32.05, -16.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.52, -22.30, -10.71, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.08, -11.10, -4.79, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.60, -9.74, -5.82, 0.00));
// parentSpheres = min(parentSpheres, sphere(32.49, -7.58, -7.35, 0.00));
// parentSpheres = min(parentSpheres, sphere(33.60, -5.24, -8.95, 0.00));
// parentSpheres = min(parentSpheres, sphere(34.45, -3.55, -10.03, 0.00));
// parentSpheres = min(parentSpheres, sphere(35.36, -1.80, -11.08, 0.00));
// parentSpheres = min(parentSpheres, sphere(35.77, 0.00, -13.18, 0.00));
// parentSpheres = min(parentSpheres, sphere(34.31, -0.00, -16.14, 0.00));
// parentSpheres = min(parentSpheres, sphere(32.72, -0.72, -18.12, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.48, -17.24, -28.05, 0.00));
// parentSpheres = min(parentSpheres, sphere(33.27, 0.01, -18.23, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.93, 0.01, -20.62, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.64, -2.44, -24.65, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.44, -15.02, -35.00, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.92, -14.65, -34.69, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.37, -4.21, -26.10, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.90, 0.00, -28.36, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.80, -0.00, -33.79, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.69, -2.92, -36.88, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.71, 0.01, -35.69, 0.00));
// parentSpheres = min(parentSpheres, sphere(21.75, -0.00, -37.25, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.45, 0.00, -40.91, 0.00));
// parentSpheres = min(parentSpheres, sphere(17.43, -0.01, -44.87, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.42, -5.01, -48.27, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.35, 0.00, -46.92, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.21, -6.70, -53.36, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.56, -7.20, -53.78, 0.00));
// parentSpheres = min(parentSpheres, sphere(15.58, 0.03, -48.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.48, 0.05, -50.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.38, 0.07, -53.83, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.76, 0.07, -57.37, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.35, -0.87, -59.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.01, -0.43, -59.50, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.84, -0.48, -59.66, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.63, -0.55, -59.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.99, -0.43, -59.52, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.59, -0.13, -58.92, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.11, 0.38, -59.40, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.67, 0.50, -59.83, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.99, 0.43, -59.52, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.67, 0.60, -59.79, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.01, 0.42, -59.50, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.35, 0.04, -57.95, 0.00));
// parentSpheres = min(parentSpheres, sphere(10.62, 0.04, -54.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(12.76, 0.03, -51.97, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.89, 0.01, -49.07, 0.00));
// parentSpheres = min(parentSpheres, sphere(9.73, 4.82, -51.66, 0.00));
// parentSpheres = min(parentSpheres, sphere(4.38, 8.78, -55.29, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.12, -0.01, -47.32, 0.00));
// parentSpheres = min(parentSpheres, sphere(15.77, 1.03, -46.45, 0.00));
// parentSpheres = min(parentSpheres, sphere(17.37, -0.01, -44.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.24, 0.00, -41.28, 0.00));
// parentSpheres = min(parentSpheres, sphere(21.66, -0.00, -37.39, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.58, 0.01, -35.90, 0.00));
// parentSpheres = min(parentSpheres, sphere(15.17, 6.33, -39.90, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.21, -0.01, -34.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.15, -0.01, -33.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.23, -0.00, -27.84, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.71, 3.97, -25.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.92, 14.65, -34.69, 0.00));
// parentSpheres = min(parentSpheres, sphere(12.33, 14.33, -34.43, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.63, 3.26, -25.27, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.79, 0.01, -20.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(33.02, 0.01, -18.67, 0.00));
// parentSpheres = min(parentSpheres, sphere(17.51, 13.48, -26.46, 0.00));
// parentSpheres = min(parentSpheres, sphere(33.40, 0.01, -17.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(34.30, -0.00, -16.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(35.78, 0.00, -13.15, 0.00));
// parentSpheres = min(parentSpheres, sphere(35.36, 1.80, -11.08, 0.00));
// parentSpheres = min(parentSpheres, sphere(34.45, 3.55, -10.03, 0.00));
// parentSpheres = min(parentSpheres, sphere(33.60, 5.24, -8.95, 0.00));
// parentSpheres = min(parentSpheres, sphere(32.49, 7.57, -7.36, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.62, 9.69, -5.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(31.13, 10.98, -4.88, 0.00));
// parentSpheres = min(parentSpheres, sphere(20.25, 20.74, -9.92, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.12, 28.84, -14.30, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.52, 31.99, -16.07, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.79, 22.02, -10.64, 0.00));
// parentSpheres = min(parentSpheres, sphere(30.92, 11.45, -4.48, 0.00));
// parentSpheres = min(parentSpheres, sphere(30.24, 12.70, -3.20, 0.00));
// parentSpheres = min(parentSpheres, sphere(29.62, 13.91, -1.91, 0.00));
// parentSpheres = min(parentSpheres, sphere(29.13, 14.91, -0.79, 0.00));
// parentSpheres = min(parentSpheres, sphere(28.62, 15.95, 0.47, 0.00));
// parentSpheres = min(parentSpheres, sphere(28.10, 17.09, 1.93, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.64, 18.17, 3.41, 0.00));
// parentSpheres = min(parentSpheres, sphere(27.24, 19.19, 4.92, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.11, 27.10, 2.75, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.96, 19.85, 5.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.00, 21.30, 7.54, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.42, 31.03, 6.66, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.60, 38.99, 5.99, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.26, 41.34, 5.77, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.83, 40.77, 5.79, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.52, 32.96, 6.46, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.76, 23.58, 7.32, 0.00));
// parentSpheres = min(parentSpheres, sphere(26.08, 21.08, 8.89, 0.00));
// parentSpheres = min(parentSpheres, sphere(25.41, 21.55, 10.78, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.82, 21.95, 12.67, 0.00));
// parentSpheres = min(parentSpheres, sphere(24.27, 22.30, 14.54, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.15, 27.20, 16.90, 0.00));
// parentSpheres = min(parentSpheres, sphere(9.44, 36.72, 19.39, 0.00));
// parentSpheres = min(parentSpheres, sphere(5.38, 40.70, 20.30, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.80, 29.51, 17.45, 0.00));
// parentSpheres = min(parentSpheres, sphere(23.48, 22.59, 16.76, 0.00));
// parentSpheres = min(parentSpheres, sphere(14.13, 30.85, 21.35, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.94, 22.67, 17.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.92, 24.48, 20.85, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.42, 29.34, 25.51, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.08, 33.36, 29.31, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.03, 34.84, 30.78, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.54, 34.48, 30.42, 0.00));
// parentSpheres = min(parentSpheres, sphere(7.88, 33.51, 29.45, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.71, 29.12, 25.30, 0.00));
// parentSpheres = min(parentSpheres, sphere(18.38, 25.16, 22.26, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.41, 29.09, 31.56, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.15, 30.01, 33.64, 0.00));
// parentSpheres = min(parentSpheres, sphere(8.79, 28.94, 31.21, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.47, 25.93, 24.03, 0.00));
// parentSpheres = min(parentSpheres, sphere(21.91, 23.18, 19.11, 0.00));
// parentSpheres = min(parentSpheres, sphere(13.68, 24.29, 27.33, 0.00));
// parentSpheres = min(parentSpheres, sphere(11.51, 24.63, 29.47, 0.00));
// parentSpheres = min(parentSpheres, sphere(22.49, 22.61, 18.59, 0.00));
// parentSpheres = min(parentSpheres, sphere(19.31, 20.97, 21.86, 0.00));
// parentSpheres = min(parentSpheres, sphere(16.14, 19.34, 25.14, 0.00));
// parentSpheres = min(parentSpheres, sphere(6.69, 18.07, 34.68, 0.00));


// auto firstDifference = difference(spheres, parentSpheres);
// auto parentOffset = offset(parentSpheres, 3);
// auto out = intersection(firstDifference,parentOffset);

  // std::cout << "Tree: " << libfive_tree_print(&out) << "\n";
  auto out_parse = parseImplicitString(implicitString);
  
  // The value for `max_err` is cargo-culted from its default value.
  Region<3> bds({-150, -80, -150}, {150, 150, 150});
  auto bds_box = box_mitered({-149, -79, -149}, {149, 149, 149});
  auto out = intersection(bds_box, out_parse);


  //findBounds(out)
  Kernel::Mesh::render(out, bds, 1.0/resolution, maxError, false)->saveSTL(OUTPUT_FILENAME);

  // std::cout << "Exported file: " << OUTPUT_FILENAME << "\n";
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("meshImplicitFunction", &meshImplicitFunction);
}
