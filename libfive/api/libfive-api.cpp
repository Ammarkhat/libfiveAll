#include <emscripten/bind.h>
#include <string>
#include <vector>

// #include <iostream>
// #include <iterator>

#include "libfive.h"
#include "libfive/solve/bounds.hpp"
#include "libfive/render/brep/mesh.hpp"

// #include <bits/stdc++.h>
// #include <iostream>
// #include <chrono>
using namespace std;

const char *OUTPUT_FILENAME = "exported.stl";
// const float OUTPUT_RESOLUTION = 15.0;

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

Tree move(Tree t, TreeVec3 offset) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - offset.x, y - offset.y, z - offset.z);
}

Tree move(Tree t, float cx, float cy, float cz) {
    LIBFIVE_DEFINE_XYZ();
    return t.remap(x - cx, y - cy, z - cz);
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
    auto r0 = cx - r;
    auto r1 = cx + r;
    return torus_z(r0, r1, {0,cy,0});
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
    if(word == "union" || word == "intersection" || word == "difference" || word == "blend"){
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
    }else if(word == "t"){
        node.type = "torus";
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
      return tr;
    } else if(root.type == "extend"){
      Tree tr = buildTree(root.children[0]);
      // tr = offset(tr, root.data[0]);
      return tr;
    } else if(root.type == "sphere"){
      return sphere(root.data[0], root.data[1], root.data[2], root.data[3]);
    } else if(root.type == "torus"){
      return torus(root.data[0], root.data[1], root.data[2]);
    }else if(root.children.size() > 0){
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
MeshDto meshImplicitFunction(std::string implicitString, float resolution, float maxError) {

  // std::cout << "Tree: " << libfive_tree_print(&out) << "\n";
  auto out_parse = parseImplicitString(implicitString);
  
  // The value for `max_err` is cargo-culted from its default value.
  Region<3> bds({-150, -80, -150}, {150, 150, 150});
  auto bds_box = box_mitered({-149, -79, -149}, {149, 149, 149});
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
