set(LIBS libfive)

set(SRCS libfive-api.cpp)

add_executable(libfive-api ${SRCS})
target_link_libraries(libfive-api ${LIBS})

target_include_directories(libfive-api SYSTEM PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIR}
    ${EIGEN_INCLUDE_DIRS}
)

target_include_directories(libfive-api PUBLIC
    ../include
)


if (EMSCRIPTEN)
   set_target_properties(libfive-api PROPERTIES
               COMPILE_FLAGS "-O3"
               LINK_FLAGS "-s ENVIRONMENT=web -s SINGLE_FILE=1 -lembind -s MODULARIZE -s EXPORT_NAME=createModule -s EXPORTED_RUNTIME_METHODS=FS"
               SUFFIX ".mjs"
               )
endif()
