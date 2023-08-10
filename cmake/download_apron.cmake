if (CRAB_USE_APRON)
if (GIT_FOUND)
  ExternalProject_Add(apron
    GIT_REPOSITORY https://github.com/antoinemine/apron.git    
    INSTALL_DIR ${CMAKE_BINARY_DIR}/run/apron
    CONFIGURE_COMMAND 
    ./configure -prefix <INSTALL_DIR> -no-cxx -no-java -no-ocaml -no-ppl -gmp-prefix ${GMP_SEARCH_PATH} -mpfr-prefix ${MPFR_SEARCH_PATH} -glpk-prefix /usr/include # fixme (JR): Hardcoding path to glpk
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make    
    INSTALL_COMMAND make install
    LOG_CONFIGURE 1
    LOG_INSTALL 1
    LOG_BUILD 1)
else ()
  message (STATUS "Crab: could not find git. Not downloading apron")
endif()
endif()
