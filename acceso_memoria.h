#include "tipos.h"
#define LAR  0
#define MAR  1
#define MBR  2
#define CANTSEG 6
#define SEG 3
#define COD 4

void getMemoria(VM*,int,int*);
void setMemoria(VM*,int,int);
void modificoLAR_MAR(VM*,int);
int logica_fisica(VM,int);
unsigned int shiftRightLogico(int,int);
void errores(int);
