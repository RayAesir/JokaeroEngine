cmake_minimum_required(VERSION 3.25)
project(JokaeroEngine VERSION 0.9.9)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

# vcpkg dependencies
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_library(FRUIT_INCLUDE_LIBS "fruit.lib")
find_package(assimp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PalSigslot CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# create source group
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(cmake/sourcelist.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

include(cmake/flags.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${SIMPLEINI_INCLUDE_DIRS}
    ${STB_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mimalloc # must be first
    assimp::assimp
    fmt::fmt
    glad::glad
    glfw
    glm::glm
    imgui::imgui
    imguizmo::imguizmo
    indicators::indicators
    magic_enum::magic_enum
    OpenGL::GL
    Pal::Sigslot
    spdlog::spdlog
    ${FRUIT_INCLUDE_LIBS}
)

# copy /build/Debug or /build/Release to /engine folder
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/build/$<CONFIGURATION>"
    "${CMAKE_SOURCE_DIR}/engine/"
)