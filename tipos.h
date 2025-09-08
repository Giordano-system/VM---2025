typedef struct TSR{
    unsigned short int base;
    unsigned short int tamano;
} TSR;

typedef struct VM {
    char * Memoria[16384];
    TSR tabla_seg[8];
    int Registros[32];
} VM;
