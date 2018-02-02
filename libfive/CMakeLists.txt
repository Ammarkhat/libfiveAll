add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)

if (GUILE_FOUND)
    add_subdirectory(bind)
endif(GUILE_FOUND)
