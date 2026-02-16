cmake_minimum_required(VERSION 3.10)

project(TestProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

# add_definitions(-DGLWINDOW_DUMP_GL_INFO)
# add_definitions(-DGLWINDOW_ENABLE_MSAA)
# add_definitions(-DGLWINDOW_ENABLE_HIDPIFACTOR)
# add_definitions(-DIMAGE_ENABLE_PNG_SUPPORT)
# add_definitions(-DIMAGE_ENABLE_JPEG_SUPPORT)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(JPEG REQUIRED zlib)

include_directories(
    ${CMAKE_SOURCE_DIR}/tgl
    ${CMAKE_SOURCE_DIR}/glwindow
    ${CMAKE_SOURCE_DIR}/glad/include
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES_TGL ${CMAKE_SOURCE_DIR}/tgl/*.cpp)
file(GLOB_RECURSE SOURCES_GLWINDOW ${CMAKE_SOURCE_DIR}/glwindow/*.cpp)
file(GLOB_RECURSE SOURCES_GLAD ${CMAKE_SOURCE_DIR}/glad/src/*.c)

add_library(common STATIC
    ${SOURCES_TGL}
    ${SOURCES_GLWINDOW}
    ${SOURCES_GLAD}
)

set_target_properties(common PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

function(create_executable TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_link_libraries(${TARGET_NAME}
        common
        ${GLFW_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${PNG_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

create_executable(test ${CMAKE_SOURCE_DIR}/test.cpp)
create_executable(fpstest ${CMAKE_SOURCE_DIR}/fpstest.cpp)
create_executable(texttest ${CMAKE_SOURCE_DIR}/texttest.cpp)
