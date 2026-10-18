if (Test-Path -Path ".\build") {
    Remove-Item -Recurse -Force ".\build"
}
mkdir ".\build"
Set-Location ".\build"
cmake -G "MinGW Makefiles" -DOpenCV_DIR="./opencv-5.0.0/build/install/x64/mingw/lib" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make
Set-Location ..