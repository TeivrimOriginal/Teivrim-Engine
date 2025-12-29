#include "ScannerPC.h"
#include <iostream>

using namespace std;
ScannerPC::ScannerPC() : rank(-1), size(0), initialized(false) {
    cout << "система сканирования запущенна" << endl;
    
}
void ScannerPC::EndScanner() {
    cout << "конец сканирования" << endl;
    
}


