set(LIBS five)

set(SRCS libfive-api.cpp)

add_executable(libfive-api ${SRCS})
target_link_libraries(libfive-api ${LIBS})

if (EMSCRIPTEN)
   set_target_properties(libfive-api PROPERTIES
               COMPILE_FLAGS "-O3"
               LINK_FLAGS "-O3"
               SUFFIX ".mjs"
               )

endif()
