#include <stdio.h>
#include <stdlib.h>
#include "acceso_memoria.h"
#define IP


typedef void (*puntFunc)(VM *);

// Declaración de las funciones de operacion
void sys(VM *); void jmp(VM *); void jz(VM *); void jp(VM *); void jn(VM *);
void jnz(VM *); void jnp(VM *); void jnn(VM *); void not(VM *); void mov(VM *);
void add(VM *); void sub(VM *); void mul(VM *); void divv(VM *); void cmp(VM *);
void shl(VM *); void shr(VM *); void sar(VM *); void and(VM *); void or(VM *);
void xor(VM *); void swap(VM *); void ldl(VM *); void ldh(VM *); void rnd(VM *);

puntFunc operaciones[0x20] = {
    sys,jmp,jz,jp,jn,jnz,jnp,jnn,not,NULL,NULL,NULL,NULL,NULL,NULL,NULL,mov,add,
    sub,mul,divv,cmp,shl,shr,sar,and,or,xor,swap,ldl,ldh,rnd
};

void leerCabecera(char []);
int analizoValidez(char []);
void iniciabilizarTablaSegmentos(VM *,char []);
void lecturaArchivo(VM *);
void inicializoRegistros(VM *);
int logica_fisica(VM, int);
void getGeneral(VM *,int,int *);
void setGeneral(VM *,int,int);
void actualizaCC(VM *,int);


int main(){
    VM MaquinaVirtual;
    char cabecera[8], byte;
    int aux, i, n, pos, valor;

    leerCabecera(cabecera);
    if (analizoValidez(cabecera)){
        iniciabilizarTablaSegmentos(&MaquinaVirtual,cabecera);
        inicializoRegistros(&MaquinaVirtual);
        lecturaArchivo(&MaquinaVirtual);

        do {
            // Guardo la dimension fisica de IP en pos y guardo el valor de memoria de esa posición
            pos = logica_fisica(MaquinaVirtual,MaquinaVirtual.Registros[3]);
            byte = MaquinaVirtual.Memoria[pos];

            // Actualizo OPC
            MaquinaVirtual.Registros[4] = byte & 0x1F;

            // Actualizo primeros 2 bits de OP1 y OP2
            if(byte >> 4 & 1 == 0 && byte >> 5 & 1 == 0){
                MaquinaVirtual.Registros[5] = byte >> 6; // OP1
                MaquinaVirtual.Registros[6] = 0; // OP2
            } else {
                MaquinaVirtual.Registros[5] = byte >> 4 & 0x03; // OP1
                MaquinaVirtual.Registros[6] = byte >> 6; // OP2
            }
            MaquinaVirtual.Registros[5] <<= 30; // OP1
            MaquinaVirtual.Registros[6] <<= 30; // OP2

            // Lectura operandos 2 y 1 (probar modularizar)
            n = MaquinaVirtual.Registros[6] >> 30;
            printf("%d",n);
            aux = 0;
            for(i=0; i<n; i++){
                pos++;
                byte = MaquinaVirtual.Memoria[pos];
                aux = aux | byte << 8 * (n-i-1);
            }
            MaquinaVirtual.Registros[6] |= aux;

            n = MaquinaVirtual.Registros[5] >> 30;
            printf("%d",n);
            aux = 0;
            for(i=0; i<n; i++){
                pos++;
                byte = MaquinaVirtual.Memoria[pos];
                aux = aux | byte << 8 * (n-i-1);
            }
            MaquinaVirtual.Registros[5] |= aux;

            // Mover IP
            MaquinaVirtual.Registros[3] += 1 + (MaquinaVirtual.Registros[5] >> 30) + (MaquinaVirtual.Registros[6] >> 30);

            // Ejecuta instrucción
            operaciones[MaquinaVirtual.Registros[4]];
            printf("%X / %X / %X / %X",MaquinaVirtual.Registros[3],MaquinaVirtual.Registros[4],MaquinaVirtual.Registros[5],MaquinaVirtual.Registros[6]);

        } while(MaquinaVirtual.Registros[4] != 0x01F && logica_fisica(MaquinaVirtual, MaquinaVirtual.Registros[3])!= -1); // OPC != STOP
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

void inicializoRegistros(VM *MaquinaVirtual){
    MaquinaVirtual->Registros[26]=MaquinaVirtual->tabla_seg[0].base; //Asigno CS
    MaquinaVirtual->Registros[27]=MaquinaVirtual->tabla_seg[1].base; //Asigno DS
    MaquinaVirtual->Registros[3]=MaquinaVirtual->Registros[26]; //Asigno CS a IP
}

void getGeneral(VM *MaquinaVirtual, int operando,int *valor){
    switch(operando >> 30) {
        case 1:
            *valor = MaquinaVirtual->Registros[operando & 0x1F];
            break;
        case 2:
            *valor = operando & 0xFF;
            break;
        case 3:
            getMemoria(MaquinaVirtual,operando,valor);
            break;
    }
}

void setGeneral(VM *MaquinaVirtual, int operando, int valor){
    if(operando >> 30 == 1)
        MaquinaVirtual->Registros[operando & 0x1F] = valor;
    else
        setMemoria(MaquinaVirtual,operando,valor);
}

void actualizaCC(VM *MaquinaVirtual, int valor){
    if(valor > 0)
        MaquinaVirtual->Registros[17] = 0;
    else if(valor == 0)
        MaquinaVirtual->Registros[17] = 1 << 30;
    else
        MaquinaVirtual->Registros[17] = 1 << 31;
}

void sys(VM *MaquinaVirtual){}
void jmp(VM *MaquinaVirtual){
    MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jz(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 30 & 1 == 1)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jp(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 30 & 1 == 0 && MaquinaVirtual->Registros[17] >> 31 & 1 == 0)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jn(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 31 & 1 == 1)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jnz(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 30 & 1 == 0)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jnp(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 30 & 1 == 1 || MaquinaVirtual->Registros[17] >> 31 & 1 == 1)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void jnn(VM *MaquinaVirtual){
    if(MaquinaVirtual->Registros[17] >> 31 & 1 == 0)
        MaquinaVirtual->Registros[3] = ((MaquinaVirtual->Registros[3] >> 16) << 16) | (MaquinaVirtual->Registros[5] & 0xFFFF);
}
void not(VM *MaquinaVirtual){
    int A;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &A);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], ~A);
    actualizaCC(MaquinaVirtual, ~A);
}
void mov(VM *MaquinaVirtual){
    int B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], B);
}
void add(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A); // Agarro lo que hay en OP1
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B); // Agarro lo que hay en OP2
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A + B); // Seteo OP1 con el valor de la operacion
    actualizaCC(MaquinaVirtual, A + B); // Actualizo el registro CC
}
void sub(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A - B);
    actualizaCC(MaquinaVirtual, A - B);
}
void mul(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A * B);
    actualizaCC(MaquinaVirtual, A * B);
}
void divv(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    // Chequear que el operando 2 (B) sea distinto de 0
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A / B);
    actualizaCC(MaquinaVirtual, A / B);
    MaquinaVirtual->Registros[16] = A % B; // Actualiza el registro AC con el resto de la division
}
void cmp(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    actualizaCC(MaquinaVirtual, A - B);
}
void shl(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A << B);
    actualizaCC(MaquinaVirtual, A << B);
}
void shr(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A >> B);
    actualizaCC(MaquinaVirtual, A >> B);
}
void sar(VM *MaquinaVirtual){}
void and(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A & B);
    actualizaCC(MaquinaVirtual, A & B);
}
void or(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A | B);
    actualizaCC(MaquinaVirtual, A | B);
}
void xor(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], A ^ B);
    actualizaCC(MaquinaVirtual, A ^ B);
}
void swap(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], A);
}
void ldl(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], (A & 0xFFFF) << 16 | (B & 0xFFFF));
}
void ldh(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[6], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[5], (A & 0xFFFF0000) | (B & 0xFFFF));
}
void rnd(VM *MaquinaVirtual){

}
