# !bin/bash

cd /home/phuckhang/MyWorkspace/MMT/rocketProgramming

mkdir -p build
cd build
cmake ..
cmake --build . -j4

echo "Build status\n"

ls -lh bin/