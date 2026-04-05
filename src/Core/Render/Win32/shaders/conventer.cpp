#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

void convertSpvToHexArray(const std::string& inputFilename, const std::string& arrayName, std::ostream& output) {
    // Открываем SPV файл в бинарном режиме
    std::ifstream file(inputFilename, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << inputFilename << std::endl;
        return;
    }
    
    // Получаем размер файла
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Читаем все байты
    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Ошибка: не удалось прочитать файл " << inputFilename << std::endl;
        return;
    }
    
    // Выводим массив
    output << "// Сгенерировано из " << inputFilename << "\n";
    output << "const unsigned char " << arrayName << "[] = {\n    ";
    
    for (size_t i = 0; i < buffer.size(); ++i) {
        // Выводим hex значение
        output << "0x" << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(buffer[i]);
        
        // Добавляем запятую, если не последний элемент
        if (i != buffer.size() - 1) {
            output << ", ";
        }
        
        // Перенос строки каждые 12 байт для читаемости
        if ((i + 1) % 12 == 0 && i != buffer.size() - 1) {
            output << "\n    ";
        }
    }
    
    output << "\n};\n";
    output << "const size_t " << arrayName << "_size = sizeof(" << arrayName << ");\n\n";
}

int main() {
    // Создаем выходной файл
    std::ofstream outputFile("shader_arrays.cpp");
    
    if (!outputFile.is_open()) {
        std::cerr << "Ошибка: не удалось создать выходной файл" << std::endl;
        return 1;
    }
    
    // Добавляем заголовочные комментарии
    outputFile << "// Автоматически сгенерированный файл с шейдерными массивами\n";
    outputFile << "// Не редактируйте вручную!\n\n";
    
    // Добавляем необходимые include
    outputFile << "#include <cstddef>\n\n";
    
    // Конвертируем ui_vert.spv
    convertSpvToHexArray("ui_vert.spv", "ui_vert_spv", outputFile);
    
    // Конвертируем ui_frag.spv
    convertSpvToHexArray("ui_frag.spv", "ui_frag_spv", outputFile);
    
    // Опционально: создаем заголовочный файл
    std::ofstream headerFile("shader_arrays.h");
    
    if (headerFile.is_open()) {
        headerFile << "#ifndef SHADER_ARRAYS_H\n";
        headerFile << "#define SHADER_ARRAYS_H\n\n";
        headerFile << "#include <cstddef>\n\n";
        headerFile << "extern const unsigned char ui_vert_spv[];\n";
        headerFile << "extern const size_t ui_vert_spv_size;\n\n";
        headerFile << "extern const unsigned char ui_frag_spv[];\n";
        headerFile << "extern const size_t ui_frag_spv_size;\n\n";
        headerFile << "#endif // SHADER_ARRAYS_H\n";
        headerFile.close();
    }
    
    outputFile.close();
    
    std::cout << "Готово! Созданы файлы:\n";
    std::cout << "- shader_arrays.cpp (с массивами байтов)\n";
    std::cout << "- shader_arrays.h (заголовочный файл)\n";
    std::cout << "\nРазмер ui_vert.spv: " << std::ifstream("ui_vert.spv", std::ios::binary | std::ios::ate).tellg() << " байт\n";
    std::cout << "Размер ui_frag.spv: " << std::ifstream("ui_frag.spv", std::ios::binary | std::ios::ate).tellg() << " байт\n";
    
    return 0;
}