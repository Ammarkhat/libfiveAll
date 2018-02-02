set(SRCS example-cxx-libfive-basic)

set(LIBS five)

add_executable(example-cxx-libfive-basic ${SRCS})
target_link_libraries(example-cxx-libfive-basic ${LIBS})


set(SRCS2 example-cxx-libfive-to-stl)

add_executable(example-cxx-libfive-to-stl ${SRCS2})
target_link_libraries(example-cxx-libfive-to-stl ${LIBS})
