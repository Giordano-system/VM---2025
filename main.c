#include <stdio.h>
#include <stdlib.h>


typedef struct TSR{
    unsigned short int base;
    unsigned short int tamano;
} TSR;

typedef struct VM {
    char * Memoria[16384];
    TSR tabla_seg [8];
    int Registros[32];
} VM;

void leerCabecera(char []);
int analizoValidez(char[]);
void iniciabilizarTablaSegmentos(VM*,char[]);


int main(){
    VM MaquinaVirtual;
    char cabecera[8];
    leerCabecera(cabecera);
    if (analizoValidez(cabecera)){
        printf("iuju");
    }
    printf("%X",cabecera[6]);
    return 0;
}

void leerCabecera(char cabecera[8]){
    FILE* arch;
    unsigned char byte;
    int i;
    arch = fopen("sample.vmx","rb");
    if (arch){
        for (i=0;i<8;i++){
            fread(&byte,1,1,arch);
            cabecera[i]=byte;
        }
        fclose(arch);
    }
    else{
        printf("hola no");
    }
}

int analizoValidez(char cabecera[]){
    int i=0;
    char version[6] = {'V', 'M', 'X', '2', '5', 0x01};
    while (i<6 && cabecera[i]==version[i])
        i++;
    return i==6;
}

void iniciabilizarTablaSegmentos(VM *MaquinaVirtual, char cabecera[8]){
    MaquinaVirtual->tabla_seg[0].base=0;
    MaquinaVirtual->tabla_seg[0].tamano=cabecera[6]<<8 | cabecera[7] ;
    MaquinaVirtual->tabla_seg[1].base=MaquinaVirtual->tabla_seg[0].tamano;
    MaquinaVirtual->tabla_seg[1].tamano=16777216-MaquinaVirtual->tabla_seg[0].tamano;
}
