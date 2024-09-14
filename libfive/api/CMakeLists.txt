set(LIBS five)

set(SRCS libfive-api.cpp)

add_executable(libfive-api ${SRCS})
target_link_libraries(libfive-api ${LIBS})

if (EMSCRIPTEN)
   set_target_properties(libfive-api PROPERTIES
               COMPILE_FLAGS "-O3 -s ELIMINATE\_DUPLICATE\_FUNCTIONS=1 -s FILESYSTEM=0 --closure 1  -s INITIAL_MEMORY=128mb -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=1024mb"
               LINK_FLAGS "-O3 -s ENVIRONMENT=web -s ELIMINATE\_DUPLICATE\_FUNCTIONS=1 -s SINGLE_FILE=1 -s FILESYSTEM=0 -s MODULARIZE -s INITIAL_MEMORY=128mb -s EXPORT_NAME=createModule -s EXPORTED_FUNCTIONS=_malloc,_free -s USE_ES6_IMPORT_META=0 -s LLD_REPORT_UNDEFINED --bind --closure 1 -s ALLOW_MEMORY_GROWTH=1  -s MAXIMUM_MEMORY=1024mb"
               SUFFIX ".mjs"
               )
#-s EXPORTED_RUNTIME_METHODS=FS
endif()
