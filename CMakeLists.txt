cmake_minimum_required(VERSION 3.0.0)
project(M16 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

include(CTest)
add_executable(M16 src/main.cpp src/M16_Emitter.cpp src/M16_MicrAsm.cpp src/M16_CPU.cpp)
target_include_directories(M16 PUBLIC
                           "${PROJECT_BINARY_DIR}"/include
                           )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
