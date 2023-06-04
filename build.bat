@echo off
cd llvm-project
mkdir build-msvc
cd build-msvc
@echo on

cmake -G Ninja -DLLVM_ENABLE_PROJECTS=lld -DLLVM_TARGETS_TO_BUILD='X86;' -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_USE_CRT_RELEASE=MD ../llvm
ninja
