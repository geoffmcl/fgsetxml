# CMakeLists.txt for fgsetxml project - 20170419
# CMakeLists.txt, generated gencmake.pl, on 2017/04/19 15:13:10
cmake_minimum_required( VERSION 2.8.8 )

# CMakeScripts or use the ones that come by default with CMake.
# set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

project( fgsetxml )

# ### NOTE: *** UPDATE 'version.txt' ***
file(READ version.txt versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine libtidy version. version.txt file is missing.")
endif()
string(STRIP "${versionFile}" VERSION_TEXT)
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\1" FGSETX_VERSION ${VERSION_TEXT})
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\2" FGSETX_DATE ${VERSION_TEXT})
# establish version number
if (FGSETX_VERSION AND FGSETX_DATE)
    add_definitions( -DFGSETX_VERSION="${FGSETX_VERSION}")
    add_definitions( -DFGSETX_DATE="${FGSETX_DATE}")
else ()
    message(FATAL_ERROR "*** FAILED to get a VERSION or DATE from version.txt!")
endif ()    

# Allow developer to select is Dynamic or static library built - default STATIC
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Set ON to build shared Library (DLL)" OFF )

if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32 AND MSVC)
    # turn off various warnings - none needed in this compile
    set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
    # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
    #     set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    # endforeach(warning)
    set( MSVC_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
    #set( NOMINMAX 1 )
    # to distinguish between debug and release libs
    set( CMAKE_DEBUG_POSTFIX "d" )
else()
    # items for unix
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}" )

# configuration file, if needed
# configure_file( ${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h )
# add_definitions( -DHAVE_CONFIG_H )
# include_directories( ${CMAKE_BINARY_DIR} )

if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else()
   message(STATUS "*** Option BUILD_SHARED_LIB is OFF ${LIB_TYPE}")
endif()

# Total libs 2, exes 3
#################################################
##### LIBRARIES 2 #####
#################################################
#### Utility library
set(name utils_lib)
set(dir utils)
include_directories( ${dir} )
set(${name}_SRCS
    ${dir}/sprtf.cxx
    ${dir}/fgs_utils.cxx
    )
set(${name}_HDRS
    ${dir}/sprtf.hxx
    ${dir}/fgs_utils.hxx
    )
add_library( ${name} ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib )
#install(FILES ${${name}_HDRS} DESTINATION include)

#################################################
#### xmlParser library
set(name xmlParser)
set(dir xmlParser)
include_directories( ${dir} )
set(${name}_SRCS
    ${dir}/xmlParser.cpp
    )
set(${name}_HDRS
    ${dir}/xmlParser.h
    )
add_library( ${name} ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib )
#install(FILES ${${name}_HDRS} DESTINATION include)

#################################################
##### EXECUTABLES #####
#################################################

#################################################
# xmlTest EXECUTABLE from xmlParser
set(name xmlTest)
set(dir xmlParser)
set(${name}_SRCS
    ${dir}/xmlTest.cpp
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
#install( TARGETS ${name} DESTINATION bin )

#################################################
# xmlEnum EXECUTABLE
set(name xmlEnum)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

#################################################
# fgsetxml EXECUTABLE
set(name fgsetxml)
set(dir src)
set(${name}_SRCS
    ${dir}/${name}.cxx
    )
add_executable( ${name} ${${name}_SRCS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

# eof
