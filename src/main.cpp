#include "Core/core.h"

int main() {
    Core engine;
    engine.settingUpRender();  // Настройка
    engine.ParserToRender();   // Загрузка данных
    engine.GameLoop();         // Запуск игрового цикла
    return 0;
}