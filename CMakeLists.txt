cmake_minimum_required(VERSION 3.10)

# Название проекта
project(EqualImageSearch)

# Укажите стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Найдите OpenCV
find_package(OpenCV REQUIRED)

# Укажите исходные файлы
add_executable(equalImageSearch equalImageSearch.cpp)

# Укажите директории для заголовочных файлов OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Подключите библиотеки OpenCV
target_link_libraries(equalImageSearch ${OpenCV_LIBS} stdc++fs)