cd "D:\Study\2026.09.16\images"

if (Test-Path -Path ".\build") {
    Remove-Item -Recurse -Force ".\build"
}
mkdir ".\build"
cd ".\build"
cmake .. -G "Visual Studio 16 2019" -A x64
start EqualImageSearch.sln
cd ..