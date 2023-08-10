# GMP_USE_STATIC_LIBS   - Set to ON to force the use of static libraries.

if (NOT GLPK_FOUND)
  # Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
  if(GLPK_USE_STATIC_LIBS )
    # save CMAKE_FIND_LIBRARY_SUFFIXES
    set(_GLPK_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()

  set(GLPK_SEARCH_PATH "" CACHE PATH "Search path for glpk.")
  find_path(GLPK_INCLUDE_DIR NAMES glpk.h PATHS ${GLPK_SEARCH_PATH}/include)
  find_library(GLPK_LIB NAMES glpk PATHS ${GLPK_SEARCH_PATH}/lib)

  mark_as_advanced(GLPK_SEARCH_PATH GLPK_INCLUDE_DIR GLPK_LIB)

  include (FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLPK
    REQUIRED_VARS GLPK_INCLUDE_DIR GLPK_LIB)

  if(GLPK_USE_STATIC_LIBS )
    # restore CMAKE_FIND_LIBRARY_SUFFIXES
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_GLPK_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
  endif()
  
endif()
