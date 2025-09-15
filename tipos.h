typedef struct TSR{
    unsigned short int base;
    unsigned short int tamano;
} TSR;

typedef struct VM {
    unsigned char Memoria[16384]; // MODIFIQUE ESTO, POSIBLE CREACION DE ERRORES (IMPORTANTE!!!!!!!)
    TSR tabla_seg[8];
    int Registros[32];
} VM;
