#include <String>
#include <fstream>
#include <iostream>
#include "Game.h"
using namespace std;
void Game::ReadFile(int numLine) {
    ifstream file("berserk.txt");
    string line;
    int currentLine = 0;
    
    while (getline(file, line)) {
        if (currentLine == numLine) {
            cout << line << endl;
            break;
        }
        currentLine++;
    }
    file.close();
}

