cmake_minimum_required(VERSION 3.24)

project(NpcGhostFix VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED)

# CommonLibSSE-NG Setup
find_package(CommonLibSSE CONFIG REQUIRED)

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/VisibilityFixer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    CommonLibSSE::CommonLibSSE
)

# SKSE Plugin information
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)

# Version definitions for 1.6.1170
target_compile_definitions(${PROJECT_NAME} PRIVATE
    SKSE_SUPPORT_AE
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive-)
endif()
