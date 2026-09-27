cmake_minimum_required(VERSION 3.10)

# Название проекта
project(EqualImageSearch)

# Укажите стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Укажите путь к OpenCV
set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv/build") # Путь к папке build OpenCV

# Найдите OpenCV
find_package(OpenCV REQUIRED)

# Укажите исходные файлы
add_executable(equalImageSearch equalImageSearch.cpp)

# Укажите директории для заголовочных файлов OpenCV
include_directories(${OpenCV_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/opencv/build/include")

# Подключите библиотеки OpenCV
target_link_libraries(equalImageSearch ${OpenCV_LIBS})