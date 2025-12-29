#include <iostream>
#include <locale>  
#include <string>
#include <cstdlib>
#include <random>
#include <String>
#include "Game.h"

using namespace std;

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    cout << "ДОБРО ПОЖАЛОВАТЬ В ИГРУ TEIVRIM QUEST" << endl;
    cout << "=====================================" << endl;
    cout << "ВЫБЕРИТЕ ДЕЙСТВИЕ " << endl;
    cout << "0 - НАЧАТЬ ИГРАТЬ" << endl;
    cout << "1 - ВЫЙТИ ИЗ ИГРЫ" << endl;
    int choose;
    cin >> choose;
    Game game;
    bool isStop;
    int numLine = 0;
    if(choose == 0) {

        cout << "Начинаем игру" << endl;
        cout << "Выберите сценарий" << endl;
        cout << "(0 - Рыцарь )" << endl;
        cout << "(0 - Рыцарь )" << endl;
            
        while(true) {
            game.ReadFile(numLine);
            
            cout << "Продолжить? (1-да, 0-нет): ";
            int choice;
            cin >> choice;
            if(choice == 0) break;
            numLine++;
        }
    }
    if(choose == 1) {
        return 0;        
    }
    system("pause");
    return 0;
}
