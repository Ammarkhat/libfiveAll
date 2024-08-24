# Sanity-checking to make sure CMake was run against the correct directory
if (NOT DEFINED LIBFIVE_BUILD_FROM_ROOT)
    message(FATAL_ERROR "CMake must be called against top-level directory")
endif()

add_subdirectory(src)
add_subdirectory(api)

