find_package(PkgConfig REQUIRED)
pkg_check_modules(COAP3 "libcoap-3-notls >= 4.0")
set(COAP3_DEFINITIONS ${PC_COAP3_CFLAGS_OTHER})

find_path(COAP3_INCLUDE_DIR 
            NAMES coap.h
            HINTS ${COAP3_INCLUDEDIR} ${COAP3_INCLUDE_DIRS}
            PATHS /usr/local/include/coap3 
                  /usr/include/coap3 )

# find_library(COAP3_STATIC_LIBRARY
#             NAMES coap.a libcoap.a libcoap-3.a
#             HINTS ${PC_COAP3_LIBDIR} ${PC_COAP3_LIBRARY_DIRS} $ENV{FFTW3_DIR}/lib
#             PATHS /usr/local/lib
#                   /usr/lib)

# find_library(COAP3_LIBRARY 
#             NAMES coap libcoap libcoap-3
#             HINTS ${PC_COAP3_LIBDIR} ${PC_COAP3_LIBRARY_DIRS} $ENV{FFTW3_DIR}/lib
#             PATHS /usr/local/lib
#                   /usr/lib)

# set(COAP3_LIBRARIES ${COAP3_LIBRARY} )
# set(COAP3_STATIC_LIBRARIES ${COAP3_STATIC_LIBRARY} )
# set(COAP3_INCLUDE_DIRS ${COAP3_INCLUDE_DIR} )

message(STATUS "COAP3 LINK LIBRARIES: " ${COAP3_LINK_LIBRARIES})
message(STATUS "COAP3 INCLUDE DIRS: " ${COAP3_INCLUDEDIR})

# include(FindPackageHandleStandardArgs)
# # handle the QUIETLY and REQUIRED arguments and set COAP3_FOUND to TRUE
# # if all listed variables are TRUE
# find_package_handle_standard_args(COAP3 DEFAULT_MSG COAP3_LIBRARY COAP3_INCLUDE_DIR)

# mark_as_advanced(COAP3_INCLUDE_DIR COAP3_STATIC_LIBRARY COAP3_LIBRARY )