cmake_minimum_required(VERSION 3.22)
project(contract_of_consensus)

set(CMAKE_CXX_STANDARD 17)

set(CDT_ROOT "/usr/opt/cdt/4.0.1")

include_directories(${CDT_ROOT}/include)
include_directories(${CDT_ROOT}/include/eosiolib/capi)
include_directories(${CDT_ROOT}/include/eosiolib/contracts)
include_directories(${CDT_ROOT}/include/eosiolib/core)
include_directories(${CDT_ROOT}/include/eosiolib/native)

include_directories(/usr/include)
include_directories(/usr/local/include)

include_directories(${CMAKE_SOURCE_DIR}/contracts)
include_directories(${CMAKE_SOURCE_DIR}/external)

file(GLOB_RECURSE SOURCES
    "contracts/*.cpp"
    "contracts/*.hpp"
    "external/*.cpp"
    "external/*.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${CDT_ROOT}/lib/libeosio_contract.a
    ${CDT_ROOT}/lib/libeosio_system.a
)

target_compile_options(${PROJECT_NAME} PRIVATE -abigen)