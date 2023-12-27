set(SRCS example-cxx-libfive-basic.cpp)

set(LIBS five)

add_executable(example-cxx-libfive-basic ${SRCS})
target_link_libraries(example-cxx-libfive-basic ${LIBS})


set(SRCS2 example-cxx-libfive-to-stl.cpp)

add_executable(example-cxx-libfive-to-stl ${SRCS2})
target_link_libraries(example-cxx-libfive-to-stl ${LIBS})

if (EMSCRIPTEN)

   # NOTE: The leading space before each flag below is required due to the
   #       "interesting" way CMake handles `LINK_FLAGS`. For details see:
   #
   #          "[CMake] good way to append link flags?"
   #          <https://cmake.org/pipermail/cmake/2014-April/057438.html>
   #

   # TODO: Ensure changes to these files cause a rebuild?

   set_property(TARGET example-cxx-libfive-to-stl
                APPEND_STRING
                PROPERTY LINK_FLAGS " --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/assets/templates/shell_threejs.html"
                )

   set_property(TARGET example-cxx-libfive-to-stl
                APPEND_STRING
                PROPERTY LINK_FLAGS " --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/assets/js/example-cxx-libfive-to-stl.pre-js.js"
                )

   #set_target_properties(example-cxx-libfive-to-stl PROPERTIES
   #            COMPILE_FLAGS "-O3"
   #            LINK_FLAGS "-O3"
   #            SUFFIX ".mjs"
   #            )

endif()
