cmake_minimum_required(VERSION 3.16)

project(eidolist VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

option(USE_SYSTEM_LIBLCF "Use system install of liblcf" OFF)
if(USE_SYSTEM_LIBLCF)
        find_package(liblcf REQUIRED)
else(USE_SYSTEM_LIBLCF)
        function(add_liblcf)
                set(BUILD_SHARED_LIBS OFF)
                set(LIBLCF_ENABLE_INSTALL OFF)
                add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/liblcf)
        endfunction()
        add_liblcf()
endif(USE_SYSTEM_LIBLCF)

set(PROJECT_SOURCES
        src/main.cpp
        src/directorydialog.cpp
        src/directorydialog.h
        src/directorydialog.ui
)

qt_add_executable(eidolist
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    eidolist.qrc
    src/diffs.h
    src/filemerge_dialog.h src/filemerge_dialog.cpp src/filemerge_dialog.ui
    src/databaseholder.h src/databaseholder.cpp
    src/ce_diff.h src/ce_diff.cpp

)

target_link_libraries(eidolist PRIVATE Qt6::Widgets)
target_link_libraries(eidolist PRIVATE lcf)

set_target_properties(eidolist PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS eidolist
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(eidolist)
endif()
