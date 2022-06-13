cmake_minimum_required(VERSION 3.13.4)

set(CMAKE_CXX_STANDARD 17)

project(ash-boot-stage0)

if (UNIX AND NOT APPLE) # unix and linux
	set(LLVM_DIR ./../llvm-project/build-linux/lib/cmake/llvm)
	add_compile_options(-march=x86-64 -m64)
endif()

if (MSVC) # windwos msvc
	set(LLVM_DIR ./../llvm-project/build-msvc/lib/cmake/llvm)
	add_compile_options(/MT)
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(./include)

# Now build our tools
add_executable(ash-boot-stage0 "source/main.cpp" "source/ast/ast.cpp" "source/ast/types.cpp" "source/ast/builder.cpp" "source/ast/parser.cpp")

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(ash-boot-stage0 ${LLVM_AVAILABLE_LIBS})

# Link against LLVM libraries
target_link_libraries(ash-boot-stage0 ${llvm_libs})