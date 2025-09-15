#include "tipos.h"
#define LAR  0
#define MAR  1
#define MBR  2

void getMemoria(VM*,int,int*);
void setMemoria(VM*,int,int);
void modificoLAR_MAR(VM*,int);
int logica_fisica(VM,int);
