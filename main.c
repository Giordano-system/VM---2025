#include <stdio.h>
#include <stdlib.h>


typedef struct TSR{
    unsigned short int base;
    unsigned short int tamano;
} TSR;

typedef struct VM {
    char * Memoria[16384];
    TSR tabla_seg[8];
    int Registros[32];
} VM;

typedef void (*puntFunc)(VM *);

puntFunc operaciones[] {
    sys,
    jmp,
    jz,
    jp,
    jn,
    jnz,
    jnp,
    jnn,
    not,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mov,
    add,
    sub,
    mul,
    div,
    cmp,
    shl,
    shr,
    sar,
    and,
    or,
    xor,
    swap,
    ldl,
    ldh,
    rnd
}

// Funciones de operacion
void sys(VM *);
void jmp(VM *);
void jp(VM *);
void jn(VM *);
void jnz(VM *);
void jnp(VM *);
void jnn(VM *);
void not(VM *);
void mov(VM *);
void add(VM *);
void mul(VM *);
void div(VM *);
void cmp(VM *);
void shl(VM *);
void shr(VM *);
void sar(VM *);
void and(VM *);
void or(VM *);
void xor(VM *);
void swap(VM *);
void ldl(VM *);
void ldh(VM *);
void rnd(VM *);













void leerCabecera(char []);
int analizoValidez(char []);
void iniciabilizarTablaSegmentos(VM *,char []);
void lecturaArchivo(VM *);
void inicializoRegistros(VM *);
int logica_fisica(VM *, int);


int main(){
    VM MaquinaVirtual;
    char cabecera[8];
    leerCabecera(cabecera);
    if (analizoValidez(cabecera)){
        iniciabilizarTablaSegmentos(&MaquinaVirtual,cabecera);
        lecturaArchivo(&MaquinaVirtual);
    }
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
        printf("No se pudo abrir el archivo.\n");
    }
}

int analizoValidez(char cabecera[]){
    int i=0;
    char version[6] = {'V', 'M', 'X', '2', '5', 0x01};
    while (i<6 && cabecera[i]==version[i])
        i++;
    return i==6;
}

void iniciabilizarTablaSegmentos(VM *MaquinaVirtual, char cabecera[]){
    MaquinaVirtual->tabla_seg[0].base=0;
    MaquinaVirtual->tabla_seg[0].tamano=cabecera[6]<<8 | cabecera[7] ;
    MaquinaVirtual->tabla_seg[1].base=MaquinaVirtual->tabla_seg[0].tamano;
    MaquinaVirtual->tabla_seg[1].tamano=16777216-MaquinaVirtual->tabla_seg[0].tamano;
}

void lecturaArchivo(VM *MaquinaVirtual){
    FILE * arch;
    unsigned char byte;
    int i;

    arch=fopen("sample.vmx","rb");
    if (arch){

        for(i=0;i<8;i++)
            fread(&byte,1,1,arch);

        fread(&byte,1,1,arch);
        i=0;
        while(!feof(arch)){
            MaquinaVirtual->Memoria[i]=byte;
            i++;
            fread(&byte,1,1,arch);
        }

        fclose(arch);
    }
    else{
        printf("No se pudo abrir el archivo.\n");
    }
}

/*
void inicializoRegistros(VM *MaquinaVirtual){
    *MaquinaVirtual->Registros[26]=*MaquinaVirtual->tabla_seg[0]->base; //Asigno CS
    *MaquinaVirtual->Registros[27]=*MaquinaVirtual->tabla_seg[1]->base; //Asigno DS
    *MaquinaVirtual->Registros[3]=*MaquinaVirtual->Registros[26]; //Asigno CS a IP
}
*/

int logica_fisica(VM *MaquinaVirtual, int registro){
    int base,offset;
    base = MaquinaVirtual->tabla_seg[registro >> 16].base;
    offset = registro & 0xFFFF;
    return base + offset;
}

