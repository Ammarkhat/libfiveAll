set(SRCS example-cxx-libfive-basic)

set(LIBS five)

add_executable(example-cxx-libfive-basic ${SRCS})
target_link_libraries(example-cxx-libfive-basic ${LIBS})
