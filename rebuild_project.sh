#!/bin/bash
set -e

cd ~/MyWorkspace/MMT/rocketProgramming

rm -rf build

mkdir build && cd build 
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_PREFIX_PATH=/usr/lib/cmake/Qt6 \
      -DCMAKE_BUILD_TYPE=Debug \
      ..

make common -j$(nproc)
if [ ! -f lib/libcommon.a ]; then
    echo "[ERROR]: libcommon.a not found!"
    exit 1
fi

make rtsp_server -j$(nproc)
make rtsp_client -j$(nproc)

cp compile_commands.json ..

echo "[Output] Files:"
ls -lh bin/

echo "[Output] Libraries:"
ls -lh lib/

echo "[System] Run commands:"
echo "  Server: ./bin/server 8554"
echo "  Client: ./bin/client 127.0.0.1 8554 movie.Mjpeg"
echo ""