#ifndef CORE_H
#define CORE_H

#include <string>

class Core {
public:
    bool isStart = false;
    
    void settingUpRender();
    void ParserToRender();
    void GameLoop();
};

#endif