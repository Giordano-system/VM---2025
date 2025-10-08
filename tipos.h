typedef struct TSR{
    unsigned short int base;
    unsigned short int tamano;
} TSR;

typedef struct VM {
    unsigned int tamanoMemoria;
    TSR tabla_seg[8];
    int Registros[32];
    char nombreVMI[100], nombreVMX[100];
    unsigned char Memoria[];
} VM;
