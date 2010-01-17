# - Find QtSingleApplication
# This module can be used to find QtSingleApplication
# Once done this will define
#
# QTSINGLEAPPLICATION_FOUND         If false, don't try to use QtSingleApplication
#
# QTSINGLEAPPLICATION_INCLUDE_DIR   Include directory necessary for using the QtSingleApplication headers
# QTSINGLEAPPLICATION_LIBRARY     Link this to use QtSingleApplication

if(QTSINGLEAPPLICATION_INCLUDE_DIR AND QTSINGLEAPPLICATION_LIBRARY)
    set(QTSINGLEAPPLICATION_FOUND TRUE)
else()
    find_path(QTSINGLEAPPLICATION_INCLUDE_DIR NAME QtSingleApplication)
    find_library(QTSINGLEAPPLICATION_LIBRARY QtSolutions_SingleApplication-2.6)

    if(QTSINGLEAPPLICATION_INCLUDE_DIR AND QTSINGLEAPPLICATION_LIBRARY)
        set(QTSINGLEAPPLICATION_FOUND TRUE)
    endif()

    if(QTSINGLEAPPLICATION_FOUND)
        set(QTSINGLEAPPLICATION_LIBRARY ${QTSINGLEAPPLICATION_LIBRARY} CACHE STRING "QtSingleApplication library" FORCE)
        if(NOT QtSingleApplication_FIND_QUIETLY)
            message(STATUS "Found QtSingleApplication: ${QTSINGLEAPPLICATION_LIBRARY} ${QTSINGLEAPPLICATION_INCLUDE_DIR}")
        endif()
    else()
        if(QtSingleApplication_FIND_REQUIRED)
            message(FATAL_ERROR "Could NOT find QtSingleApplication")
        else()
            if(NOT QtSingleApplication_FIND_QUIETLY)
                message(STATUS "Could NOT find QtSingleApplication")
            endif()
        endif()
    endif()
endif()
