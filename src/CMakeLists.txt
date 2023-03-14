find_package(Qt5 COMPONENTS Core Widgets CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(CURL REQUIRED)
find_package(regulaSdk 6 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON-GLIB REQUIRED json-glib-1.0)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${JSON-GLIB_INCLUDE_DIRS}
)

set(LINK_LIBS
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    regulaSdk::regulaSdk
    ${JSON-GLIB_LIBRARIES}
)

list(APPEND SRC_LIBS
    main.cpp

    mainwindow.cpp
    mainwindow.h

    documentreader.cpp
    documentreader.h

    documentsender.cpp
    documentsender.h

    jsonreader.cpp
    jsonreader.h

    mainwindow.ui
)

add_definitions(-DQT_NO_KEYWORDS)
add_executable(${PROJECT_NAME} ${SRC_LIBS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
