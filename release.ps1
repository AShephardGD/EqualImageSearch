# Пути
$BUILD_DIR = "./build"
$RELEASE_DIR = "./release"
$OPENCV_BIN = "./opencv-5.0.0/build/install/x64/mingw/bin"

# Создаём чистую папку для релиза
if (Test-Path $RELEASE_DIR) {Remove-Item -Recurse -Force $RELEASE_DIR}
New-Item -ItemType Directory -Force -Path $RELEASE_DIR

# 1. Копируем сам компилированный EXE
Copy-Item "$BUILD_DIR/equalImageSearch.exe" $RELEASE_DIR

# 2. Копируем главную DLL от OpenCV 5
Copy-Item "$OPENCV_BIN/libopencv_world500.dll" $RELEASE_DIR

Write-Host "Релиз сборка успешно подготовлена в папке $RELEASE_DIR" -ForegroundColor Green