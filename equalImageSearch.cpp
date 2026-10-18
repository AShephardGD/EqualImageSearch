#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream> // ОБЯЗАТЕЛЬНО: для чтения файлов с диска
#include <windows.h>
#include <string_view>
#include <stdexcept>

#include "opencv-5.0.0/build/install/include/opencv2/opencv.hpp"
#include "opencv-5.0.0/build/install/include/opencv2/features2d.hpp"
#include "opencv-5.0.0/build/install/include/opencv2/highgui.hpp"
#include "opencv-5.0.0/build/install/include/opencv2/imgproc.hpp"
#include "opencv-5.0.0/build/install/include/opencv2/calib3d.hpp"

namespace fs = std::filesystem;

// Структура для хранения информации об изображении: путь и его гистограмма
struct ImageData {
    fs::path path;
    cv::Mat hist;
};

// Перечисление для режима вывода результатов
enum class OutputMode {
    console,
    file,
    both,
    unknown
};

// Расширенное перечисление для булевых значений с возможностью неопределённого состояния
enum class extendedBool {
    trueValue,
    falseValue,
    unknown
};

// Функция для надежной конвертации wstring в правильный UTF-8 string без скрытого мусора
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Функция получения корректного double значения из консоли с проверкой на ошибки ввода
double getDoubleFromConsole(const std::string& prompt, const std::string& errorMessage) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear(); // Сброс состояния ошибки
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера ввода
            std::cout << errorMessage << std::endl;
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера ввода
            return value;
        }
    }
}

// Функция для вычисления хэша строки на этапе компиляции
constexpr uint64_t hashString(std::string_view str) {
    uint64 hash = 0;
    for (char c: str) hash = (hash * 131) + static_cast<uint64>(c);
    return hash;
}

// Пользовательский литерал для хэширования строк на этапе компиляции
constexpr uint64_t operator"" _hash(const char* str, size_t len) {
    return hashString(std::string_view(str, len));
}

// Функция для безопасного чтения строки из консоли с поддержкой кириллицы
std::string getStringFromConsole(const std::string& prompt) {
    std::cout << prompt;
    wchar_t wBuffer[MAX_PATH];
    DWORD charsRead = 0;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    ReadConsoleW(hInput, wBuffer, MAX_PATH, &charsRead, NULL);
    std::wstring wPath(wBuffer, charsRead);
    // Удаляем возможные символы перевода строки в конце
    while (!wPath.empty() && (wPath.back() == L'\r' || wPath.back() == L'\n')) wPath.pop_back();
    // Очищаем буфер ввода, чтобы избежать проблем с последующими вводами
    FlushConsoleInputBuffer(hInput);
    return WStringToUTF8(wPath);
}

// Функция для установки параметров из аргументов командной строки
void setParamsFromArgs(double& histCoeff, double& eqCoeff, extendedBool& showPairs, OutputMode& pairsOutput, fs::path& folder, fs::path& outputFile, int argc, char* argv[]) {
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            switch (hashString(arg)) {
                case "--histCoeff"_hash:
                case "-h"_hash:
                    if (i + 1 < argc) {
                        try {
                            histCoeff = std::stod(argv[++i]);
                            if (histCoeff < 0.0 || histCoeff > 1.0) {
                                std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromArgs): Коэффициент гистограммы должен быть в диапазоне [0.0, 1.0]." << std::endl;
                                histCoeff = -1;
                            }
                        } catch (const std::exception&) {
                            std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromArgs): Некорректное значение для коэффициента гистограммы." << std::endl;
                        }
                    }
                    break;
                case "--eqCoeff"_hash:
                case "-e"_hash:
                    if (i + 1 < argc) {
                        try {
                            eqCoeff = std::stod(argv[++i]);
                            if (eqCoeff < 0.0 || eqCoeff > 1.0) {
                                std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromArgs): Коэффициент похожести должен быть в диапазоне [0.0, 1.0]." << std::endl;
                                eqCoeff = -1;
                            }
                        } catch (const std::exception&) {
                            std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromArgs): Некорректное значение для коэффициента похожести." << std::endl;
                        }
                    }
                    break;
                case "--showPairs"_hash:
                case "-s"_hash:
                    showPairs = extendedBool::trueValue;
                    break;
                case "--console"_hash:
                case "-c"_hash:
                    pairsOutput = OutputMode::console;
                    break;
                case "--file"_hash:
                case "-f"_hash:
                    pairsOutput = OutputMode::file;
                    break;
                case "--both"_hash:
                case "-b"_hash:
                    pairsOutput = OutputMode::both;
                    break;
                case "--folder"_hash:
                case "--directory"_hash:
                case "-d"_hash:
                    if (i + 1 < argc)
                        folder = fs::u8path(argv[++i]).lexically_normal();
                    break;
                case "--outputFile"_hash:
                case "-o"_hash:
                    if (i + 1 < argc)
                        outputFile = fs::u8path(argv[++i]).lexically_normal();
                    break;
            }
        }
    }
}

// Функция для установки параметров из файла params.txt
void setParamsFromFile(double& histCoeff, double& eqCoeff, extendedBool& showPairs, OutputMode& pairsOutput, fs::path& folder, fs::path& outputFile) {
    std::ifstream paramFile("params.txt");
    if (paramFile.is_open()) {
        std::string line;
        while (std::getline(paramFile, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                switch (hashString(key)) {
                    case "histCoeff"_hash:
                        if (histCoeff == -1) {
                            try {
                                histCoeff = std::stod(value);
                                if (histCoeff > 1.0 || histCoeff < 0.0) {
                                    std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromFile): Коэффициент гистограммы в файле params.txt должен быть в диапазоне [0.0, 1.0]." << std::endl;
                                    histCoeff = -1;
                                }
                            } catch (const std::exception&) {
                                std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromFile): Некорректное значение для коэффициента гистограммы в файле params.txt." << std::endl;
                            }
                        }
                        break;
                    case "eqCoeff"_hash:
                        if (eqCoeff == -1) {
                            try {
                                eqCoeff = std::stod(value);
                                if (eqCoeff > 1.0 || eqCoeff < 0.0) {
                                    std::cerr << "Ошбика (equalImageSearch.cpp - setParamsFromFile): Коэффициент похожести в файле params.txt должен быть в диапазоне [0.0, 1.0]." << std::endl;
                                    eqCoeff = -1;
                                }
                            } catch (const std::exception&) {
                                std::cerr << "Ошибка (equalImageSearch.cpp - setParamsFromFile): Некорректное значение для коэффициента похожести в файле params.txt." << std::endl;
                            }
                        }
                        break;
                    case "showPairs"_hash:
                        if (showPairs == extendedBool::unknown) showPairs = (value != "false" && value != "0")? extendedBool::trueValue : extendedBool::falseValue;
                        break;
                    case "pairsOutput"_hash:
                        if (pairsOutput == OutputMode::unknown) {
                            switch (hashString(value)) {
                                case "console"_hash:
                                    pairsOutput = OutputMode::console;
                                    break;
                                case "file"_hash:
                                    pairsOutput = OutputMode::file;
                                    break;
                                case "both"_hash:
                                    pairsOutput = OutputMode::both;
                                    break;
                            }
                        }
                        break;
                    case "dir"_hash:
                        if (folder.empty()) folder = fs::u8path(value).lexically_normal();
                        break;
                    case "outputFile"_hash:
                        if (outputFile.empty()) outputFile = fs::u8path(value).lexically_normal();
                        break;
                }
            }
        }
    }
}

// Функция для установки параметров из консоли
void setParamsFromConsole(double& histCoeff, double& eqCoeff, extendedBool& showPairs, OutputMode& pairsOutput, fs::path& folder, fs::path& outputFile) {
    while (histCoeff > 1.0 || histCoeff < 0.0) histCoeff = getDoubleFromConsole("Введите коэффициент гистограммы (0.0 - 1.0): ", "Вы ввели некорректное значение. Попробуйте снова.");
    while (eqCoeff > 1.0 || eqCoeff < 0.0) eqCoeff = getDoubleFromConsole("Введите коэффициент похожести (0.0 - 1.0): ", "Вы ввели некорректное значение. Попробуйте снова.");
    if (showPairs == extendedBool::unknown) {
        std::string input;
        std::cout << "Показывать пары похожих изображений в консоли (0/false - нет, иначе - да): ";
        std::getline(std::cin, input);
        showPairs = (input != "false" && input != "0") ? extendedBool::trueValue : extendedBool::falseValue;
    }
    while (pairsOutput == OutputMode::unknown) {
        std::string input;
        std::cout << "Выводить пары похожих изображений в консоль, файл или оба варианта? (console/file/both): ";
        std::getline(std::cin, input);
        switch (hashString(input)) {
            case "console"_hash:
                pairsOutput = OutputMode::console;
                break;
            case "file"_hash:
                pairsOutput = OutputMode::file;
                break;
            case "both"_hash:
                pairsOutput = OutputMode::both;
                break;
        }
    }
    if (folder.empty()) folder = fs::u8path(getStringFromConsole("Введите путь к папке с изображениями: ")).lexically_normal();
    if (outputFile.empty() && (pairsOutput == OutputMode::file || pairsOutput == OutputMode::both)) 
        outputFile = fs::u8path(getStringFromConsole("Введите путь к файлу для вывод результатов: ")).lexically_normal();
}

// Функция для установки всех параметров, учитывая приоритет: аргументы командной строки > файл params.txt > ввод с консоли
void setParams(double& histCoeff, double& eqCoeff, extendedBool& showPairs, OutputMode& pairsOutput, fs::path& folder, fs::path& outputFile, int argc, char* argv[]) {
    setParamsFromArgs(histCoeff, eqCoeff, showPairs, pairsOutput, folder, outputFile, argc, argv);
    setParamsFromFile(histCoeff, eqCoeff, showPairs, pairsOutput, folder, outputFile);
    setParamsFromConsole(histCoeff, eqCoeff, showPairs, pairsOutput, folder, outputFile);
}

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
std::string pathToUTF8String(const fs::path& p) {
    std::wstring w = p.wstring();
    return WStringToUTF8(w);
}

// Функция для возврата старой кодовой страницы консоли
void returnOldConsoleCodePage(uint oldInputCP, uint oldOutputCP) {
    SetConsoleOutputCP(oldOutputCP);
    SetConsoleCP(oldInputCP);
}

// Функция для сравнения изображений с использованием ORB
double compareImagesORB(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    std::vector<cv::KeyPoint> keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    // Найти ключевые точки и дескрипторы для обоих изображений
    orb->detectAndCompute(img1, cv::Mat(), keypoints1, descriptors1);
    orb->detectAndCompute(img2, cv::Mat(), keypoints2, descriptors2);

    // Сопоставить дескрипторы с помощью BFMatcher
    cv::BFMatcher matcher(cv::NORM_HAMMING, true);
    std::vector<cv::DMatch> matches;
    matcher.match(descriptors1, descriptors2, matches);

    // Рассчитать среднее расстояние между совпадениями
    double totalDistance = 0;
    for (const auto& match : matches) {
        totalDistance += match.distance;
    }
    return totalDistance / matches.size(); // Чем меньше значение, тем больше схожесть
}

double compareImages(const ImageData& img1, const ImageData& img2, long long& similarImagesCount, double histCoeff, double eqCoeff, bool showPairs) {
    // Сравнение гистограмм
    double histSimilarity = compareHist(img1.hist, img2.hist, cv::HISTCMP_CORREL);
    // Если гистограммы схожи, используем ORB для уточнения
    if (histSimilarity > histCoeff) {
        cv::Mat img1Mat = imread_unicode(img1.path);
        cv::Mat img2Mat = imread_unicode(img2.path);

        if (img1Mat.empty()) {
            std::cout << "Не удалось открыть изображение для ORB сравнения: " << img1.path << std::endl;
            return 0.0;
        }
        if (img2Mat.empty()) {
            std::cout << "Не удалось открыть изображение для ORB сравнения: " << img2.path << std::endl;
            return 0.0;
        }
        double equality = 1.0 / (1.0 + compareImagesORB(img1Mat, img2Mat)); // Чем меньше расстояние, тем больше схожесть
        if (equality > eqCoeff) {
            if (showPairs)
                std::cout << "Похожие изображения: " << pathToUTF8String(img1.path) << " и " << pathToUTF8String(img2.path)
                          << " с коэффициентом гистограммы: " << histSimilarity
                          << " и с коэффициентом похожести: " << equality << std::endl;
            similarImagesCount++;
        }
        return equality;
    }
    return 0.0; // Если гистограммы не схожи, возвращаем 0
}

void iterateAndCompareImages(std::list<ImageData>& processedImages, std::vector<std::vector<fs::path>>& similarImagesGroups, double histCoeff, double eqCoeff, bool showPairs) {
    long long totalComparisons = 0;
    long long similarImagesCount = 0;
    for (auto it1 = processedImages.begin(); it1 != processedImages.end(); ++it1) {
        auto distanceFromStart = std::distance(processedImages.begin(), it1);
        if (distanceFromStart % 100 == 0)
            std::cout << "Обработано изображений: " << distanceFromStart << " из " << processedImages.size() << std::endl;
        std::vector<fs::path> imageGroup;
        imageGroup.push_back(it1->path);
        auto it2 = std::next(it1);
        while (it2 != processedImages.end()) {
            ++totalComparisons;
            double eqRes = compareImages(*it1, *it2, similarImagesCount, histCoeff, eqCoeff, showPairs);
            if (eqRes > eqCoeff) {
                // Если изображения схожи, можно удалить их из списка, чтобы ускорить последующие сравнения
                imageGroup.push_back(it2->path);
                it2 = processedImages.erase(it2);
            }
            else ++it2;
        }
        if (imageGroup.size() > 1) similarImagesGroups.push_back(imageGroup);
    }
    std::cout << "Проверка завершена! Всего выполнено сравнений: " << totalComparisons << std::endl;
    std::cout << "Проверка завершена! Всего похожих картинок: " << similarImagesCount << std::endl;
}

void loadHistogramsFromFolder(const fs::path& folder, std::list<ImageData>& processedImages) {
    std::vector<fs::path> rawPaths;

    std::cout << "Путь к папке: " << pathToUTF8String(folder) << std::endl;

    if      (!fs::exists(folder))
        std::cerr << "Ошибка loadHistogramsFromFolder: Указанная папка не существует." << std::endl;
    else if (!fs::is_directory(folder))
        std::cerr << "Ошибка loadHistogramsFromFolder: Указанный путь не является папкой." << std::endl;;

    for (const auto& entry : fs::directory_iterator(folder))
        if (entry.is_regular_file() && (entry.path().extension() == ".jpg" || entry.path().extension() == ".png"))
            rawPaths.push_back(entry.path().lexically_normal());

    std::cout << rawPaths.size() << " изображений найдено в папке. Началась загрузка картинок..." << std::endl;

    for (size_t i = 0; i < rawPaths.size(); ++i) {
        if (i % 100 == 0)
            std::cout << "Обработано изображений: " << i << " из " << rawPaths.size() << std::endl;

        cv::Mat img = imread_unicode(rawPaths[i]);

        if (img.empty()) {
            std::cerr << "Не удалось открыть файл (возможно повреждён): " << pathToUTF8String(rawPaths[i]) << std::endl;
            continue;
        }

        ImageData data;
        data.path = rawPaths[i];
        data.hist = calculateHistogram(img);
        processedImages.push_back(data);
    }
}

int main(int argc, char* argv[]) {
    uint oldInputCP = GetConsoleCP();
    uint oldOutputCP = GetConsoleOutputCP();
    // Включаем UTF-8 для вывода текста в консоли Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    std::list<ImageData> processedImages; // Используем std::list для хранения изображений, так как часто удаляем элементы
    std::vector<std::vector<fs::path>> similarImagesGroups; // Для хранения групп похожих изображений

    // 0.9 0.02
    double histCoeff = -1; // Коэффициент гистограммы (0.0 - 1.0)
    double eqCoeff = -1; // Коэффициент похожести (0.0 - 1.0)
    extendedBool showPairs = extendedBool::unknown; // Показывать ли пары похожих изображений в консоли
    OutputMode pairsOutput = OutputMode::unknown; // Способ вывода пар похожих изображений
    fs::path folderPathObj, outputFile;

    // Устанавливаем параметры из аргументов командной строки, файла params.txt и консоли
    setParams(histCoeff, eqCoeff, showPairs, pairsOutput, folderPathObj, outputFile, argc, argv);

    // Загружаем гистограммы изображений из указанной папки
    loadHistogramsFromFolder(folderPathObj, processedImages);

    std::cout << "\nВсе гистограммы в памяти. Начинаем мгновенное сравнение..." << std::endl;

    while (histCoeff || eqCoeff) {
        // Сравниваем изображения и группируем похожие
        iterateAndCompareImages(processedImages, similarImagesGroups, histCoeff, eqCoeff, showPairs == extendedBool::trueValue);
        if (pairsOutput == OutputMode::file || pairsOutput == OutputMode::both) {
            fs::path parentDir = outputFile.parent_path();
            // Создаём папки, если их нет
            if (!parentDir.empty() && !fs::exists(parentDir)) fs::create_directories(parentDir);
            
            std::ofstream outFile(outputFile);
            if (!outFile.is_open()) std::cerr << "Ошибка: Не удалось открыть файл для записи результатов." << std::endl;
            else {
                for (const auto& group : similarImagesGroups) {
                    outFile << "Группа похожих изображений:" << std::endl;
                    for (const auto& path : group) outFile << " " << pathToUTF8String(path) << std::endl;
                    outFile << std::endl;
                }
                outFile.close();
            }
        }
        if (pairsOutput == OutputMode::console || pairsOutput == OutputMode::both) {
            std::cout << "Результаты следующие: " << std::endl;
            for (const auto& group : similarImagesGroups) {
                std::cout << "Группа похожих изображений: " << std::endl;
                for (const auto& path : group) std::cout << " " << pathToUTF8String(path) << std::endl;
                std::cout << std::endl;
            }
        }

        std::cout << "Введите коэффициент гистограммы (0.0 - 1.0) или 0 для выхода: ";
        std::cin >> histCoeff;
        if (!histCoeff) break;
        std::cout << "Введите коэффициент похожести (0.0 - 1.0) или 0 для выхода: ";
        std::cin >> eqCoeff;
        if (!eqCoeff) break;
    }
    
    std::cout << "Нажмите Enter для выхода..." << std::endl;
    std::cin.get();
    returnOldConsoleCodePage(oldInputCP, oldOutputCP);
    return 0;
}
