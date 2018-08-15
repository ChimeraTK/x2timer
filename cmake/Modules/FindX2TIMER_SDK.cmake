#######################################################################################################################
#
# cmake module for finding the x2timer SDK
#
# returns:
#   X2TIMER_SDK_FOUND        : true or false, depending on whether the package was found
#   X2TIMER_SDK_VERSION      : the package version
#   X2TIMER_SDK_INCLUDE_DIRS : path to the include directory
#   X2TIMER_SDK_LIBRARY_DIRS : path to the library directory
#   X2TIMER_SDK_LIBRARIES    : list of libraries to link against
#   X2TIMER_SDK_CXX_FLAGS    : Flags needed to be passed to the c++ compiler
#   X2TIMER_SDK_LINK_FLAGS   : Flags needed to be passed to the linker
#
# @author Martin Hierholzer, DESY
#
#######################################################################################################################

SET(X2TIMER_SDK_FOUND 0)


FIND_PATH(X2TIMER_SDK_DIR libsdk.so
  ${CMAKE_CURRENT_LIST_DIR}
  /export/doocs/lib
)

set(X2TIMER_SDK_LIBRARIES sdk)

# now set the required variables based on the determined X2TIMER_SDK_DIR
set(X2TIMER_SDK_INCLUDE_DIRS ${X2TIMER_SDK_DIR}/include)
set(X2TIMER_SDK_LIBRARY_DIRS ${X2TIMER_SDK_DIR}/)

set(X2TIMER_SDK_CXX_FLAGS "")
set(X2TIMER_SDK_LINK_FLAGS "")

# extract version from librar so symlink. Note: This is platform dependent!
execute_process(COMMAND bash -c "readelf -d ${X2TIMER_SDK_DIR}/libsdk.so | grep SONAME | sed -e 's/^.*Library soname: \\[libsdk\\.so\\.//' -e 's/\\]$//'"
                OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE X2TIMER_SDK_VERSION)

# use a macro provided by CMake to check if all the listed arguments are valid and set X2TIMER_SDK_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(X2TIMER_SDK REQUIRED_VARS X2TIMER_SDK_DIR VERSION_VAR X2TIMER_SDK_VERSION )

