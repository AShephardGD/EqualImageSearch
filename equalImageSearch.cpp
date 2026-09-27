#include "opencv/build/include/opencv.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <windows.h> 

namespace fs = std::filesystem;

// Функция для вычисления гистограммы изображения
cv::Mat calculateHistogram(const cv::Mat& image) {
    cv::Mat hsv, hist;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    int h_bins = 50, s_bins = 60;
    int histSize[] = {h_bins, s_bins};
    float h_ranges[] = {0, 180};
    float s_ranges[] = {0, 256};
    const float* ranges[] = {h_ranges, s_ranges};
    int channels[] = {0, 1};
    cv::calcHist(&hsv, 1, channels, cv::Mat(), hist, 2, histSize, ranges, true, false);
    cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
    return hist;
}

// Функция для сравнения двух изображений
double compareImages(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat hist1 = calculateHistogram(img1);
    cv::Mat hist2 = calculateHistogram(img2);
    return cv::compareHist(hist1, hist2, cv::HISTCMP_CORREL);
}

int main() {
    // Включаем UTF-8 для вывода текста в консоли Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::vector<fs::path> imagePaths;

    // Используем обычный cout — в UTF-8 режиме он выведет русский текст без кракозябр
    std::cout << "Введите путь к папке с изображениями: ";

    // Безопасное чтение широких символов (кириллицы) прямо из буфера консоли Windows
    // Это единственный способ, который PowerShell не сможет испортить или обрезать
    wchar_t wBuffer[MAX_PATH];
    DWORD charsRead = 0;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    ReadConsoleW(hInput, wBuffer, MAX_PATH, &charsRead, NULL);
    // Удаляем символы переноса строки (\r\n) в конце
    std::wstring wPath(wBuffer, charsRead);
    while (!wPath.empty() && (wPath.back() == L'\r' || wPath.back() == L'\n')) {
        wPath.pop_back();
    }

    // Создаем объект пути из широкой строки
    fs::path folderPathObj(wPath);
    // 1. Создаем строку из буфера
    std::wstring wPath(wBuffer, charsRead);
    
    // .string() вернет путь в UTF-8, который cout теперь отлично напечатает
    std::cout << "Путь к папке: " << folderPathObj.string() << std::endl;

    // Проверяем, существует ли папка
    if (!fs::exists(folderPathObj) || !fs::is_directory(folderPathObj)) {
        std::cerr << "Ошибка: Указанная папка не существует или это не папка." << std::endl;
        std::cin.get();
        return 1;
    }

    // Считываем все файлы из папки
    for (const auto& entry : fs::directory_iterator(folderPathObj)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".jpg" || entry.path().extension() == ".png")) {
            imagePaths.push_back(entry.path()); 
        }
    }
    std::cout << imagePaths.size() << " изображений найдено в папке." << std::endl;

    // Сравниваем изображения
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        std::cout << "Обрабатывается изображение: " << imagePaths[i].string() << std::endl;
        
        // В Windows cv::imread принимает строку. Чтобы кириллица открылась, 
        // библиотека OpenCV должна получить путь в системной кодировке.
        // Метод .string() в Windows-версии std::filesystem автоматически сделает нужную конвертацию.
        cv::Mat img1 = cv::imread(imagePaths[i].string());
        if (img1.empty()) {
            std::cout << "Не удалось открыть файл (возможно поврежден)." << std::endl;
            continue;
        }

        for (size_t j = i + 1; j < imagePaths.size(); ++j) {
            cv::Mat img2 = cv::imread(imagePaths[j].string());
            if (img2.empty()) continue;

            double similarity = compareImages(img1, img2);
            if (similarity > 0.9) { // Порог похожести
                std::cout << "Похожие изображения: " << imagePaths[i].string() << " и " << imagePaths[j].string() << std::endl;
            }
        }
    }

    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();
    return 0;
}
