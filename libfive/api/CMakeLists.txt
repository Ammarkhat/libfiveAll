set(LIBS five)

set(SRCS libfive-api.cpp)

add_executable(libfive-api ${SRCS})
target_link_libraries(libfive-api ${LIBS})

if (EMSCRIPTEN)
   set_target_properties(libfive-api PROPERTIES
               COMPILE_FLAGS "-O3"
               LINK_FLAGS "-s ENVIRONMENT=web -s SINGLE_FILE=1 -lembind -s MODULARIZE -s TOTAL_STACK=128mb -s TOTAL_MEMORY=256mb -s EXPORT_NAME=createModule -s EXPORTED_RUNTIME_METHODS=FS"
               SUFFIX ".mjs"
               )
endif()
