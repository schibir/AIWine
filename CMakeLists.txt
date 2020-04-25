cmake_minimum_required (VERSION 2.8)
set(TARGET wine)
project(${TARGET})

aux_source_directory(./src SrcFiles)

add_executable(${TARGET} ${SrcFiles})