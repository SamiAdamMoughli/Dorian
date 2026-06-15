cmake_minimum_required(VERSION 3.20)
project(Dorian LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Dorian only builds on Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Dorian must be compiled for x64.")
endif()

add_executable(Dorian
    src/main.cpp
    src/ErrorHandler.cpp
    src/PayloadManager.cpp
    src/FileManager.cpp
    src/SectionManager.cpp
    src/ProcessExecutor.cpp
)

target_include_directories(Dorian PRIVATE include/dorian)

target_compile_definitions(Dorian PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(Dorian PRIVATE ntdll)
