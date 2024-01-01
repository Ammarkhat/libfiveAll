/*

  libfive C++ API example of STL file export.

  Usage:

      echo 2 | ./example-cxx-libfive-to-stl

 */

#include <iostream>
#include <iterator>

#include "libfive.h"
#include "libfive/solve/bounds.hpp"
#include "libfive/render/brep/mesh.hpp"


const char *OUTPUT_FILENAME = "exported.stl";
const float OUTPUT_RESOLUTION = 15.0;

int main() {

  std::cout << "libfive Revision: " << libfive_git_branch() << " " << libfive_git_version() << " " << libfive_git_revision() << "\n";

  auto x = Kernel::Tree::X();
  auto y = Kernel::Tree::Y();
  auto z = Kernel::Tree::Z();

  // auto r = Kernel::Tree(*std::istream_iterator<float>(std::cin));
  auto r = Kernel::Tree(2.0f);

  auto out = (x * x) + (y * y) + (z * z) - r;

  std::cout << "Tree: " << libfive_tree_print(&out) << "\n";

  // We could use this C API function to export the STL but it is not
  // possible to set `multithread` to false with this approach.
  //
  // libfive_tree_save_mesh(&out, libfive_tree_bounds(&out), OUTPUT_RESOLUTION, OUTPUT_FILENAME);

  // Use the C++ API to export STL file in a single-threaded manner (by setting `multithread` to false).
  //
  // The value for `max_err` is cargo-culted from its default value.
  Kernel::Mesh::render(out, findBounds(out), 1.0/OUTPUT_RESOLUTION, 1e-8, false)->saveSTL(OUTPUT_FILENAME);

  std::cout << "Exported file: " << OUTPUT_FILENAME << "\n";

  return 0;
}