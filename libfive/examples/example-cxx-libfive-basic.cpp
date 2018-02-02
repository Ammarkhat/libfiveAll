/*

  Basic libfive C++ API example

  (via <https://libfive.com/examples/>)

 */

#include <iostream>

#include "libfive.h"
#include "libfive/solve/bounds.hpp"


int main() {

  std::cout << "libfive Revision: " << libfive_git_branch() << " " << libfive_git_version() << " " << libfive_git_revision() << "\n";

  auto x = Kernel::Tree::X();
  auto y = Kernel::Tree::Y();
  auto z = Kernel::Tree::Z();

  auto out = (x * x) + (y * y) + (z * z) - 1;

  auto bounds = findBounds(out);

  std::cout << "Tree: " << libfive_tree_print(&out) << "\n";

  std::cout << "Region volume: " << bounds.volume() << "\n";

  return 0;
}