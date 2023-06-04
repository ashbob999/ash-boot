#!/bin/bash

cd llvm-project
mkdir build-linux
cd build-linux

cmake -G Ninja -DLLVM_ENABLE_PROJECTS=lld -DLLVM_TARGETS_TO_BUILD='X86;' -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install -DLLVM_INCLUDE_BENCHMARKS=OFF ../llvm
ninja
