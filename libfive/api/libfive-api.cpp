#include <emscripten/bind.h>

// #include <iostream>
// #include <iterator>

#include "libfive.h"
#include "libfive/solve/bounds.hpp"
#include "libfive/render/brep/mesh.hpp"


const char *OUTPUT_FILENAME = "exported.stl";
const float OUTPUT_RESOLUTION = 15.0;

using namespace emscripten;

Kernel::Tree sphere(float radius, float cx, float cy, float cz){
  auto x = Kernel::Tree::X() - Kernel::Tree(cx);
  auto y = Kernel::Tree::Y() - Kernel::Tree(cy);
  auto z = Kernel::Tree::Z() - Kernel::Tree(cz);

  auto r = Kernel::Tree(radius);
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
  Kernel::Mesh::render(out, findBounds(out), 1.0/OUTPUT_RESOLUTION, 1e-8, false)->saveSTL(OUTPUT_FILENAME);

  // std::cout << "Exported file: " << OUTPUT_FILENAME << "\n";

}

EMSCRIPTEN_BINDINGS(my_module) {
    function("meshImplicitFunction", &meshImplicitFunction);
}
