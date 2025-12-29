#ifndef SCANNERPC_H
#define SCANNERPC_H

#include "mpi/mpi.h"


class ScannerPC { 
public:
    MPI_Scanner scanner;
    ScannerPC();
    void EndScanner();
private:
    int rank;
    int size;
    bool initialized;
};

#endif