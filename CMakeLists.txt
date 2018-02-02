cmake_minimum_required(VERSION 3.3)
project(libfive)


# Set libfive configuration defaults
set(LIBFIVE_WITH_MARCH_NATIVE 1)
set(LIBFIVE_WITH_ISOC99_HW_ROUNDING 0)
set(LIBFIVE_FIND_PKG_QT 1)
set(LIBFIVE_FIND_PKG_PNG 1)
set(LIBFIVE_FIND_PKG_GUILE 1)
set(LIBFIVE_FIND_PKG_THREADS 1)


# Set WebAssembly/Emscripten configuration
if (EMSCRIPTEN)

    # Emscripten doesn't support `-march=native` flag.
    set(LIBFIVE_WITH_MARCH_NATIVE 0)

    # `Boost.Numeric.Interval` can't automatically determine
    # rounding control specialisation for WebAssembly
    # so we specify it manually.
    set(LIBFIVE_WITH_ISOC99_HW_ROUNDING 1)


    # We don't (yet?!) attempt to support Qt for WebAssembly
    set(LIBFIVE_FIND_PKG_QT 0)

    # We will use `libpng` from Emscripten ports
    set(LIBFIVE_FIND_PKG_PNG 0)

    # We don't (yet?) support Guile for WebAssembly
    set(LIBFIVE_FIND_PKG_GUILE 0)

    # WebAssembly doesn't currently support threads.
    set(LIBFIVE_FIND_PKG_THREADS 0)


    # Emscripten disables search of "system" include paths
    # but that prevents use of header-only libraries.
    #
    # The functionality from Boost (and Eigen) that libfive
    # uses is in header-only libraries so we re-enable search
    # of system paths to avoid the need to (re-)install the
    # libraries outside of the system paths.
    #
    # Note: In theory this could break something if we
    #       accidently found non-Boost/Eigen libraries.
    #
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

    # This enables the (header-only) Eigen library to be found (as I
    # recall, via a `.pc` file with `pkg-config`) via the same path
    # root used to find Boost, when supplied via executing, e.g.:
    #
    #   emcmake cmake .. -DCMAKE_PREFIX_PATH=/usr/local
    #
    # In theory this setting should already be enabled by default
    # (see "this behavior is enabled by default if CMAKE_MINIMUM_REQUIRED_VERSION is 3.1 or later" on
    # <https://cmake.org/cmake/help/v3.1/module/FindPkgConfig.html#variable:PKG_CONFIG_USE_CMAKE_PREFIX_PATH>)
    # because this `CMakeLists.txt` file specifies:
    #
    #   cmake_minimum_required(VERSION 3.3)
    #
    # But after the call to `project()` the value of `CMAKE_MINIMUM_REQUIRED_VERSION`
    # seems to be overwritten by the value `3.0.0` as specified in
    # `emscripten/1.37.28/cmake/Modules/Platform/Emscripten.cmake`. I
    # thought CMake scoping rules prevented that from happening but it
    # appears this is not the case. Which leads to rather confusing behaviour...
    #
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH 1)


    # Prevents warning from Emscripten that dynamic libraries
    # are "currently not supported".
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".bc")

    # Ensures examples/tests produce HTML + JavaScript files.
    SET(CMAKE_EXECUTABLE_SUFFIX ".html")


    # Note: It's not entirely clear to me what approach is best for
    # specifying the Emscripten `-s` options below. It seems that it's
    # not enough, for example, to use `add_compile_options(-s WASM=1)`
    # but `link_libraries("-s WASM=1")` is sufficient--even though
    # `-s` seems more like a compiler flag.

    # Ensure WebAssembly is generated rather than asm.js.
    link_libraries("-s WASM=1")

    # Improves debug-ability of exceptions.
    link_libraries("-s ASSERTIONS=1")


    # Ensure libfive library uses WebAssembly port of libpng.
    # Note: This should really only be in the library-specific `CMakeLists.txt`.
    add_compile_options(-s USE_LIBPNG=1)

endif()


# Properly distinguish between Apple and upstream Clang
cmake_policy(SET CMP0025 NEW)

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

################################################################################

set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -fPIC -pedantic -Werror=switch -stdlib=libc++")

if (LIBFIVE_WITH_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

set(CMAKE_CXX_FLAGS_RELEASE  "-O3 -DRELEASE -DEIGEN_NO_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG    "-O0")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

# Work around an issue with Boost::Interval on OpenBSD and others
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD" OR LIBFIVE_WITH_ISOC99_HW_ROUNDING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_ISOC99")
endif()

################################################################################
# Find all packages here at the top level so we can print debugging info

if (LIBFIVE_FIND_PKG_QT)
   find_package(Qt5Core)
endif()

find_package(Boost REQUIRED)

if (LIBFIVE_FIND_PKG_PNG)
   find_package(PNG REQUIRED)
endif()


find_package(PkgConfig REQUIRED)

if (LIBFIVE_FIND_PKG_GUILE)
   pkg_check_modules(GUILE guile-2.2)
endif()

pkg_check_modules(EIGEN REQUIRED eigen3)


if (LIBFIVE_FIND_PKG_THREADS AND (UNIX AND NOT(APPLE)))
    find_package(Threads REQUIRED)
endif(LIBFIVE_FIND_PKG_THREADS AND (UNIX AND NOT(APPLE)))

################################################################################
# Inform the user which subsystems will be built and any missing deps
message("Checking dependencies:")
message("  libfive: ✓")

if (GUILE_FOUND)
    message("  libfive-guile:     ✓")

    # Sadly, this is a global setting (there's no target_link_directories)
    link_directories(${GUILE_LIBRARY_DIRS})
else ()
    message("  libfive-guile:     ✘   (needs Guile 2.2 or later)")
endif()

if (Qt5Core_FOUND AND GUILE_FOUND)
    message("  Studio:       ✓")
else ()
    if (Qt5Core_FOUND)
        message("  Studio:       ✘   (needs Guile 2.2 or later)")
    elseif (GUILE_FOUND)
        message("  Studio:       ✘   (Qt 5.7 or later)")
    else()
        message("  Studio:       ✘   (needs Guile 2.2 or later and Qt 5.7 or later)")
    endif()
endif()

################################################################################

# Always build the kernel and test suite
add_subdirectory(libfive)

if(GUILE_FOUND AND Qt5Core_FOUND)
    add_subdirectory(studio)
endif(GUILE_FOUND AND Qt5Core_FOUND)
