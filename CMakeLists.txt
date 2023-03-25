cmake_minimum_required(VERSION 3.16)
project(EasyDC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH "/usr/bin/flex")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/branchNum)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/differentialCry)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/preprocessing)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/sbox)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/util)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/data)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/z3/include)

INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)

# s3lab
INCLUDE_DIRECTORIES(/home/pusun/gurobi912/linux64/include)

add_subdirectory(lib)

# find FL_BIBRARY
if(APPLE)
    set(CMAKE_PREFIX_PATH "/usr/local/opt/flex")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fopenmp")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -gdwarf-2")

FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)
message( ${CMAKE_CURRENT_SOURCE_DIR} )
BISON_TARGET(parser lib/language/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
        )
FLEX_TARGET(lexer lib/language/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(lexer parser)

ADD_EXECUTABLE(EasyDC
        ${FLEX_lexer_OUTPUTS}
        ${BISON_parser_OUTPUTS}
        main.cpp
        )

#  libgurobi_c++.a
find_library(gurobi_static NAMES libgurobi_g++5.2.a
        PATHS "$ENV{LD_LIBRARY_PATH}")

find_library(gurobi_dynamic NAMES libgurobi91.so libgurobi.so.9.1.2
        PATHS "$ENV{LD_LIBRARY_PATH}")
if(NOT gurobi_static)
    message(FATAL_ERROR "Failed to find Gurobi lib")
endif()
message(${gurobi_static})
message(${gurobi_dynamic})

TARGET_LINK_LIBRARIES(EasyDC ${FLEX_LIBRARIES} plbench jsoncpp)
TARGET_LINK_LIBRARIES(EasyDC ${gurobi_static} ${gurobi_dynamic} ${PROJECT_SOURCE_DIR}/z3/bin/libz3.so)