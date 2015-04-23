# multiarch support

option(ENABLE_MULTIARCH "Enable support for multi-arch distros (lib64)" OFF)

include(CMakeParseArguments)

IF(NOT DEFINED LIB_SUFFIX)
  message(STATUS "Multiarch option set to ${ENABLE_MULTIARCH}")
  IF(ENABLE_MULTIARCH)
    IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
      SET(LIB_SUFFIX "")
    ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
      SET(LIB_SUFFIX 64)
    ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  ELSE()
    SET(LIB_SUFFIX "")
  ENDIF()
ENDIF(NOT DEFINED LIB_SUFFIX)

set(DST_LIB lib${LIB_SUFFIX})

# testrunner support macros

function(testrunner_project _name)
  set(TESTRUNNER_PROJECT_DIR /opt/tests/${_name} CACHE INTERNAL "TESTRUNNER_PROJECT_DIR")
  configure_install(FILES tests.xml DESTINATION ${TESTRUNNER_PROJECT_DIR})
endfunction(testrunner_project _name)

macro(testrunner_install)
  cmake_parse_arguments(_arg "" "" "TARGETS;FILES;PROGRAMS" ${ARGN})
  if (DEFINED TESTRUNNER_PROJECT_DIR)
    foreach(_arg ${_arg_TARGETS})
      install(TARGETS ${_arg} DESTINATION ${TESTRUNNER_PROJECT_DIR})
    endforeach(_arg)
    foreach(_arg ${_arg_FILES})
      install(FILES ${_arg} DESTINATION ${TESTRUNNER_PROJECT_DIR})
    endforeach(_arg)
    foreach(_arg ${_arg_PROGRAMS})
      install(PROGRAMS ${_arg} DESTINATION ${TESTRUNNER_PROJECT_DIR})
    endforeach(_arg)
  else()
    message(ERROR "Define TESTRUNNER_PROJECT")
  endif()
endmacro(testrunner_install)

# C++

if(CMAKE_COMPILER_IS_GNUCXX)

  set(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -feliminate-unused-debug-types -Wl,--as-needed -Wl,--no-undefined -Wl,-z,now"
    )

  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

  # Cor uses C++11
  if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    message(STATUS "Build with C++11 support")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  elseif(GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6)
    message(STATUS "Build with limited C++11 support")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  else()
    message(FATAL_ERROR "Maybe your version of g++ (${GCC_VERSION}) is not supported for C++11")
  endif()

  if(GCC_VERSION VERSION_LESS 4.7)
    # fix for gcc 4.6 specific compiling/linking issues, no dwarf-4
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-3")
  endif()

  #-Wno-psabi is to remove next g++ warning/note:
  #the mangling of 'va_list' has changed in GCC 4.4
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

else(CMAKE_COMPILER_IS_GNUCXX)
  message(FATAL_ERROR "Tweak Nemo macros for your compiler")
endif(CMAKE_COMPILER_IS_GNUCXX)


# wrappers around configuration macros

function(configure_install)
  cmake_parse_arguments(_arg "" "DESTINATION" "FILES;PROGRAMS" ${ARGN})
  set(NAMES ${_arg_FILES})
  LIST(APPEND NAMES ${_arg_PROGRAMS})

  foreach(_name ${NAMES})
    configure_file(${_name}.in ${CMAKE_BINARY_DIR}/${_name} @ONLY)
  endforeach(_name)

  foreach(_name ${_arg_FILES})
    install(FILES ${CMAKE_BINARY_DIR}/${_name} DESTINATION ${_arg_DESTINATION})
  endforeach(_name)

  foreach(_name ${_arg_PROGRAMS})
    install(PROGRAMS ${CMAKE_BINARY_DIR}/${_name} DESTINATION ${_arg_DESTINATION})
  endforeach(_name)
endfunction(configure_install)


function(configure_install_pkgconfig _name)
  IF(DEFINED DST_LIB)
    CONFIGURE_INSTALL(FILES ${_name}.pc DESTINATION ${DST_LIB}/pkgconfig)
  ELSE()
    message(ERROR "Should define DST_LIB")
  ENDIF()
endfunction(configure_install_pkgconfig)
