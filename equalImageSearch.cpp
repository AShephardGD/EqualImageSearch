#include "opencv/build/include/opencv.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream> // ОБЯЗАТЕЛЬНО: для чтения файлов с диска
#include <windows.h> 

namespace fs = std::filesystem;

struct ImageData {
    fs::path path;
    cv::Mat hist;
};

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

// Функция для надежной конвертации wstring в правильный UTF-8 string без скрытого мусора
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// РЕШЕНИЕ ПРОБЛЕМЫ: Безопасное чтение изображения с кириллицей в пути через std::ifstream
cv::Mat imread_unicode(const fs::path& path) {
    // В Windows std::ifstream нативно поддерживает fs::path с любыми русскими буквами
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return cv::Mat();

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return cv::Mat();

    // Декодируем изображение из буфера памяти (OpenCV это делает без привязки к путям файловой системы)
    return cv::imdecode(cv::Mat(buffer), cv::IMREAD_COLOR);
}

// Вспомогательная функция для безопасного вывода пути в std::cout
std::string path_to_utf8_string(const fs::path& p) {
    std::wstring w = p.wstring();
    return WStringToUTF8(w);
}

void returnOldConsoleCodePage(uint oldInputCP, uint oldOutputCP) {
    // Восстанавливаем старую кодовую страницу консоли
    SetConsoleOutputCP(oldOutputCP);
    SetConsoleCP(oldInputCP);
}

int main() {
    uint oldInputCP = GetConsoleCP();
    uint oldOutputCP = GetConsoleOutputCP();
    // Включаем UTF-8 для вывода текста в консоли Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::vector<fs::path> rawPaths;
    std::vector<ImageData> processedImages;

    std::cout << "Введите путь к папке с изображениями: ";

    wchar_t wBuffer[MAX_PATH];
    DWORD charsRead = 0;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    ReadConsoleW(hInput, wBuffer, MAX_PATH, &charsRead, NULL);

    std::wstring wPath(wBuffer, charsRead);
    
    while (!wPath.empty() && (wPath.back() == L'\r' || wPath.back() == L'\n')) {
        wPath.pop_back();
    }
    
    std::string utf8Path = WStringToUTF8(wPath);
    fs::path folderPathObj = fs::u8path(utf8Path);
    folderPathObj = folderPathObj.lexically_normal();
    
    std::cout << "Путь к папке: " << path_to_utf8_string(folderPathObj) << std::endl;

    if (!fs::exists(folderPathObj) || !fs::is_directory(folderPathObj)) {
        std::cerr << "Ошибка: Указанная папка не существует или это не папка." << std::endl;
        std::cin.get();
        returnOldConsoleCodePage(oldInputCP, oldOutputCP);
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(folderPathObj)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".jpg" || entry.path().extension() == ".png")) {
            rawPaths.push_back(entry.path().lexically_normal()); 
        }
    }
    std::cout << rawPaths.size() << " изображений найдено в папке. Началась загрузка картинок..." << std::endl;

    for (size_t i = 0; i < rawPaths.size(); ++i) {
        //std::cout << "Обрабатывается изображение: " << path_to_utf8_string(rawPaths[i]) << std::endl;
        if (i % 100 == 0) std::cout << "Обработано изображений: " << i << " из " << rawPaths.size() << std::endl;

        // ИСПРАВЛЕНИЕ: Читаем файл через буфер памяти вместо cv::imread
        cv::Mat img = imread_unicode(rawPaths[i]);
        
        if (img.empty()) {
            std::cout << "Не удалось открыть файл (возможно поврежден)." << std::endl;
            continue;
        }

        ImageData data;
        data.path = rawPaths[i];
        data.hist = calculateHistogram(img);
        processedImages.push_back(data);
    }

    std::cout << "\nВсе гистограммы в памяти. Начинаем мгновенное сравнение..." << std::endl;

    long long totalComparisons = 0;
    long long similarImagesCount = 0;
    for (size_t i = 0; i < processedImages.size(); ++i) {
        for (size_t j = i + 1; j < processedImages.size(); ++j) {
            totalComparisons++;
            double similarity = cv::compareHist(processedImages[i].hist, processedImages[j].hist, cv::HISTCMP_CORREL);
            if (similarity > 0.989182) { 
                std::cout << "Похожие изображения: " << path_to_utf8_string(processedImages[i].path) 
                << " и " << path_to_utf8_string(processedImages[j].path) << " с коэффициентом: " << similarity << std::endl;
                similarImagesCount++;
            }
        }
    }

    std::cout << "Проверка завершена! Всего выполнено сравнений: " << totalComparisons << std::endl;
    std::cout << "Проверка завершена! Всего похожих картинок: " << similarImagesCount << std::endl;
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    
    FlushConsoleInputBuffer(hInput);
    std::cin.get();
    returnOldConsoleCodePage(oldInputCP, oldOutputCP);
    return 0;
}
