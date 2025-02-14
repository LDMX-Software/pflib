# - Try to find cactus

#macro(find_cactus_in_extern arg)
# find_path(CACTUS_ROOT uhal.hpp
#    HINTS ${PROJECT_SOURCE_DIR}/extern/cactus ${arg})
#endmacro()

#find_cactus_in_extern("")

#set(CACTUS_ROOT ${PROJECT_SOURCE_DIR}/extern/cactus )

file(GLOB_RECURSE emp_include /opt/cactus/include/emp/Controller.hpp)
#MESSAGE(STATUS "emp_include = ${emp_include}")
if(emp_include)
    set(EMP_ROOT /opt/cactus)
    MESSAGE(STATUS "Found EMP in ${EMP_ROOT}")
else(emp_include)
    MESSAGE(FATAL_ERROR "Controller.hpp was not found in /opt/cactus/include/emp")
endif(emp_include)

set(EMP_PREFIX ${EMP_ROOT} )
set(EMP_INCLUDE_PREFIX ${EMP_PREFIX}/include/emp )
set(EMP_LIB_PREFIX ${EMP_PREFIX}/lib )

set(EMP_LOG_INCLUDE_PREFIX ${EMP_PREFIX}/include/emp/logger )
set(EMP_LOG_LIB_PREFIX ${EMP_PREFIX}/lib )

# # could not find the package at the usual locations -- try to copy from AFS if accessible
# if (NOT CACTUS_ROOT)
#   MESSAGE(WARNING "Could not find CACTUS package required by miniTLU producer. Please refer to the documentation on how to obtain the software.")
# endif()

# set(EXTERN_ERLANG_PREFIX ${CACTUS_ROOT}/lib/erlang )
# set(EXTERN_ERLANG_BIN_PREFIX ${EXTERN_ERLANG_PREFIX}/bin )

# set(EXTERN_BOOST_PREFIX ${CACTUS_ROOT} )
# set(EXTERN_BOOST_INCLUDE_PREFIX ${EXTERN_BOOST_PREFIX}/include )
# set(EXTERN_BOOST_LIB_PREFIX ${EXTERN_BOOST_PREFIX}/lib )

# set(EXTERN_PUGIXML_PREFIX ${CACTUS_ROOT}  )
# set(EXTERN_PUGIXML_INCLUDE_PREFIX ${EXTERN_PUGIXML_PREFIX}/include/pugixml )
# set(EXTERN_PUGIXML_LIB_PREFIX ${EXTERN_PUGIXML_PREFIX}/lib )

# set(UHAL_GRAMMARS_PREFIX ${CACTUS_ROOT} )
# set(UHAL_GRAMMARS_INCLUDE_PREFIX ${UHAL_GRAMMARS_PREFIX}/include/uhal/grammars )
# set(UHAL_GRAMMARS_LIB_PREFIX ${UHAL_GRAMMARS_PREFIX}/lib )

# set(UHAL_LOG_PREFIX ${CACTUS_ROOT} )
# set(UHAL_LOG_INCLUDE_PREFIX ${UHAL_LOG_PREFIX}/include/uhal )
# set(UHAL_LOG_LIB_PREFIX ${UHAL_LOG_PREFIX}/lib )

# set(UHAL_UHAL_PREFIX ${CACTUS_ROOT} )
# set(UHAL_UHAL_INCLUDE_PREFIX ${UHAL_UHAL_PREFIX}/include )
# set(UHAL_UHAL_LIB_PREFIX ${UHAL_UHAL_PREFIX}/lib )

# include(FindPackageHandleStandardArgs)
# # handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# # if all listed variables are TRUE
# find_package_handle_standard_args(CACTUS  DEFAULT_MSG
#                                   CACTUS_ROOT)

# mark_as_advanced(CACTUS_LIB CACTUS_ROOT )
