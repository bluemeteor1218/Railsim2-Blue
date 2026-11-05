cmake_minimum_required(VERSION 3.25)

project(RailSim2Modern LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The Direct3D 12 renderer requires Windows.")
endif()

add_subdirectory(renderer/d3d12)
