cmake_minimum_required(VERSION 3.6.0)
project(cmake VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

aux_source_directory(. DIR_SRCS)
add_executable(cmake ${DIR_SRCS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
