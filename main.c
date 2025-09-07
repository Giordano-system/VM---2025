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


int main(){
    VM MaquinaVirtual;
    char cabecera[8], byte;
    int aux, i, n, pos;

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
            aux = 0;
            for(i=0; i<n; i++){
                pos++;
                byte = MaquinaVirtual.Memoria[pos];
                aux = aux | byte << 8 * (n-i-1);
            }
            MaquinaVirtual.Registros[6] |= aux;

            n = MaquinaVirtual.Registros[5] >> 30;
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

        } while(MaquinaVirtual.Registros[4] != 0x0F); // OPC != STOP
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

int logica_fisica(VM MaquinaVirtual, int registro){
    int base,offset;
    base = MaquinaVirtual.tabla_seg[registro >> 16].base;
    offset = registro & 0xFFFF;
    return base + offset;
}




void sys(VM *MaquinaVirtual){}
void jmp(VM *MaquinaVirtual){}
void jz(VM *MaquinaVirtual){}
void jp(VM *MaquinaVirtual){}
void jn(VM *MaquinaVirtual){}
void jnz(VM *MaquinaVirtual){}
void jnp(VM *MaquinaVirtual){}
void jnn(VM *MaquinaVirtual){}
void not(VM *MaquinaVirtual){}
void mov(VM *MaquinaVirtual){}
void add(VM *MaquinaVirtual){}
void sub(VM *MaquinaVirtual){}
void mul(VM *MaquinaVirtual){}
void divv(VM *MaquinaVirtual){}
void cmp(VM *MaquinaVirtual){}
void shl(VM *MaquinaVirtual){}
void shr(VM *MaquinaVirtual){}
void sar(VM *MaquinaVirtual){}
void and(VM *MaquinaVirtual){}
void or(VM *MaquinaVirtual){}
void xor(VM *MaquinaVirtual){}
void swap(VM *MaquinaVirtual){}
void ldl(VM *MaquinaVirtual){}
void ldh(VM *MaquinaVirtual){}
void rnd(VM *MaquinaVirtual){}
