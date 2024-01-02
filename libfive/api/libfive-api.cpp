#include <emscripten/bind.h>

// #include <iostream>
// #include <iterator>

#include "libfive.h"
#include "libfive/render/brep/mesh.hpp"
#include "libfive/tree/tree.hpp"
#include "../stdlib/stdlib_impl.hpp"
#include "libfive/render/brep/settings.hpp"

// Tree _union(Tree, Tree);
// Tree intersection(Tree, Tree);
// Tree inverse(Tree);
// Tree difference(Tree, Tree);
// Tree offset(Tree, TreeFloat);
// Tree sphere(TreeFloat, TreeVec3);

const char *OUTPUT_FILENAME = "exported.stl";
const float OUTPUT_RESOLUTION = 15.0;

using namespace emscripten;
using namespace libfive;

Tree sphere(float radius, float cx, float cy, float cz){
  auto x = Tree::X() - Tree(cx);
  auto y = Tree::Y() - Tree(cy);
  auto z = Tree::Z() - Tree(cz);

  auto r = Tree(radius);
  auto out = (x * x) + (y * y) + (z * z) - r;
  return out;
}

void meshImplicitFunction(std::string implicitString) {
    
  // std::cout << "libfive Revision: " << libfive_git_branch() << " " << libfive_git_version() << " " << libfive_git_revision() << "\n";

  // std::cout << "received implicit string: "<< implicitString<<"\n"; 

  auto sphere1 = sphere(2, 0,0,0);
  // auto sphere2 = sphere(2, 1,0,0);
  // auto out = sphere1->union(sphere2);
  auto out = sphere1;

  // std::cout << "Tree: " << libfive_tree_print(&out) << "\n";

  // The value for `max_err` is cargo-culted from its default value.
  // Pick the target region to render
  auto bounds = Region<3>({-5, -5, -5}, {5, 5, 5});

  // Mesh::render returns a unique_ptr, so it cleans up automatically
  BRepSettings settings;
  settings.min_feature = 1.0/OUTPUT_RESOLUTION;
  settings.max_err = 1e-8;
  settings.workers = 1;

  // Mesh::render(out, bounds, 1.0/OUTPUT_RESOLUTION, 1e-8, false)->saveSTL(OUTPUT_FILENAME);
  Mesh::render(out, bounds, settings)->saveSTL(OUTPUT_FILENAME);

  // std::cout << "Exported file: " << OUTPUT_FILENAME << "\n";

}

EMSCRIPTEN_BINDINGS(my_module) {
    function("meshImplicitFunction", &meshImplicitFunction);
}
