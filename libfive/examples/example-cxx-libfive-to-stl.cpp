/*

  libfive C++ API example of STL file export.

  Usage:

      echo 2 | ./example-cxx-libfive-to-stl

 */

#include <iostream>
#include <iterator>

#include "libfive.h"

const char *OUTPUT_FILENAME = "exported.stl";

int main() {

  std::cout << "libfive Revision: " << libfive_git_branch() << " " << libfive_git_version() << " " << libfive_git_revision() << "\n";

  auto x = Kernel::Tree::X();
  auto y = Kernel::Tree::Y();
  auto z = Kernel::Tree::Z();

  auto r = Kernel::Tree(*std::istream_iterator<float>(std::cin));

  auto out = (x * x) + (y * y) + (z * z) - r;

  std::cout << "Tree: " << libfive_tree_print(&out) << "\n";

  libfive_tree_save_mesh(&out, libfive_tree_bounds(&out), 15, OUTPUT_FILENAME);

  std::cout << "Exported file: " << OUTPUT_FILENAME << "\n";

  return 0;
}