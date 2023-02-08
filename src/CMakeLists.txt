find_package(Qt5 COMPONENTS Core Widgets CONFIG REQUIRED)
set(LINK_LIBS Qt5::Core Qt5::Widgets)

find_package(CURL REQUIRED)
list(APPEND LINK_LIBS CURL::libcurl)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON-GLIB REQUIRED json-glib-1.0)

include_directories(${JSON-GLIB_INCLUDE_DIRS})

find_package(regulaSdk 6 CONFIG REQUIRED)
list(APPEND LINK_LIBS regulaSdk::regulaSdk)

list(APPEND SRC_LIBS
    main.cpp

    mainwindow.cpp
    mainwindow.h

    documentreader.cpp
    documentreader.h

    documentsender.cpp
    documentsender.h

    mainwindow.ui
)

add_definitions(-DQT_NO_KEYWORDS)
add_executable(${PROJECT_NAME} ${SRC_LIBS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LINK_LIBS}
    ${JSON-GLIB_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
