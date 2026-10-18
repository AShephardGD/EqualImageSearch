Set-Location ./opencv-5.0.0
mkdir build
Set-Location build
cmake -G "MinGW Makefiles" -D CMAKE_BUILD_TYPE=Release -D BUILD_opencv_dnn=OFF -D BUILD_TESTS=OFF -D BUILD_PERF_TESTS=OFF -D BUILD_opencv_world=ON -D CMAKE_INSTALL_PREFIX=./install ..
mingw32-make -j4
mingw32-make install
Set-Location ../..