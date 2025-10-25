#include <stdio.h>
#include <stdlib.h>
#include "acceso_memoria.h"
#include <time.h>
#include <ctype.h>
#include <string.h>
//Errores
#define OPE 1
#define DIV0 2
#define SEG 3
#define COD 4
#define SET 5
#define SYS 6
#define SHIFT 7
#define MEM 8
#define SO 9
#define SU 10
//Registros
#define LAR 0
#define MAR 1
#define MBR 2
#define IP 3
#define OPC 4
#define OP1 5
#define OP2 6
#define SP 7
#define BP 8
#define EAX 10
#define EBX 11
#define ECX 12
#define EDX 13
#define EEX 14
#define EFX 15
#define AC 16
#define CC 17
#define CS 26
#define DS 27
#define ES 28
#define SS 29
#define KS 30
#define PS 31

const char *Mnemonicos[32] = {
    [0x00] = "SYS",
    [0x01] = "JMP",
    [0x02] = "JZ",
    [0x03] = "JP",
    [0x04] = "JN",
    [0x05] = "JNZ",
    [0x06] = "JNP",
    [0x07] = "JNN",
    [0x08] = "NOT",
    [0x0B] = "PUSH",
    [0x0C] = "POP",
    [0x0D] = "CALL",
    [0x0E] = "RET",
    [0x0F] = "STOP",
    [0x10] = "MOV",
    [0x11] = "ADD",
    [0x12] = "SUB",
    [0x13] = "MUL",
    [0x14] = "DIV",
    [0x15] = "CMP",
    [0x16] = "SHL",
    [0x17] = "SHR",
    [0x18] = "SAR",
    [0x19] = "AND",
    [0x1A] = "OR",
    [0x1B] = "XOR",
    [0x1C] = "SWAP",
    [0x1D] = "LDL",
    [0x1E] = "LDH",
    [0x1F] = "RND"
};


const char *registros[32] = {
    "LAR",
    "MAR",
    "MBR",
    "IP",
    "OPC",
    "OP1",
    "OP2",
    "SP",
    "BP",
    "-",
    "EAX",
    "EBX",
    "ECX",
    "EDX",
    "EEX",
    "EFX",
    "AC",
    "CC",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "CS",
    "DS",
    "ES",
    "SS",
    "KS",
    "PS"
};

const char* registros16[] = {"AX",  "BX",  "CX",  "DX", "EX", "FX"};
const char* registros8h[] = {"AH",  "BH",  "CH",  "DH", "EH", "FH"};
const char* registros8l[] = {"AL",  "BL",  "CL",  "DL", "EL", "FL"};

typedef void (*puntFunc)(VM *);

// Declaración de las funciones de operacion
void sys(VM *); void jmp(VM *); void jz(VM *); void jp(VM *); void jumpn(VM *);
void jnz(VM *); void jnp(VM *); void jnn(VM *); void not(VM *); void mov(VM *);
void add(VM *); void sub(VM *); void mul(VM *); void divv(VM *); void cmp(VM *);
void shl(VM *); void shr(VM *); void sar(VM *); void and(VM *); void or(VM *);
void xor(VM *); void swap(VM *); void ldl(VM *); void ldh(VM *); void rnd(VM *);
void stop(VM *); void push(VM *); void pop(VM *); void call(VM *); void ret(VM *);

puntFunc operaciones[0x20] = {
    sys,jmp,jz,jp,jumpn,jnz,jnp,jnn,not,NULL,NULL,push,pop,call,ret,stop,mov,add,
    sub,mul,divv,cmp,shl,shr,sar,and,or,xor,swap,ldl,ldh,rnd
};

void agregaParamSegment(VM *, char **, int, int *);
void leerCabecera(unsigned char [], VM);
int analizoValidez(unsigned char []);
void iniciabilizarTablaSegmentosyRegistros(VM *,unsigned char []);
void inicializaPila(VM *,int,int);
void lecturaArchivo(VM *,int);
void lecturaArchivoVMI(VM *);
void creaArchivoVMI(VM);
void cargaoperacion(VM *);
int logica_fisica(VM, int);
void getGeneral(VM *,int,int *);
void setGeneral(VM *,int,int);
void actualizaCC(VM *,int);
unsigned int shiftRightLogico(int,int);
void manejarBreakpoint(VM *);
void ejecutarInstruccion(VM *);
void desensamblado(VM *);
void errores(int);


int main(int argc, char *argv[]){
    VM MaquinaVirtual;
    unsigned char cabecera[18];
    int debug, cantParametros, i, j, puntero, n;
    char **parametros;


    static char *args[] = {"programa.exe", "pruebaSYS37F.vmx", "archivo.vmi", "-d", NULL};

    argc = 4;       // cantidad de argumentos
    argv = args;    // apuntamos argv a nuestro arreglo


    // Inicializar variables
    debug = 0;
    cantParametros = 0;
    parametros = NULL;
    strcpy(MaquinaVirtual.nombreVMI,"");
    strcpy(MaquinaVirtual.nombreVMX,"");
    MaquinaVirtual.tamanoMemoria = 16 * 1024; // REVISAR COMO LIMITAR EL ESPACIO DE MEMORIA

    // Lectura de parametros
    for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], ".vmx")) {
            strcpy(MaquinaVirtual.nombreVMX,argv[i]);
        } else if (strstr(argv[i], ".vmi")) {
            strcpy(MaquinaVirtual.nombreVMI,argv[i]);
        } else if (strncmp(argv[i], "m=", 2) == 0) {
            MaquinaVirtual.tamanoMemoria = atoi(argv[i] + 2) * 1024;
        } else if (strcmp(argv[i], "-d") == 0) {
            debug = 1;
        } else if (strcmp(argv[i], "-p") == 0 && strcmp(MaquinaVirtual.nombreVMX,"") != 0) {
            cantParametros = argc - (i + 1);
            parametros = &argv[i + 1];
            break;
        }
    }

    if(strcmp(MaquinaVirtual.nombreVMX,"") != 0) { // Preparo MV
       agregaParamSegment(&MaquinaVirtual,parametros,cantParametros,&puntero);
       leerCabecera(cabecera, MaquinaVirtual);
      if (analizoValidez(cabecera)){
           iniciabilizarTablaSegmentosyRegistros(&MaquinaVirtual, cabecera);
           lecturaArchivo(&MaquinaVirtual, cabecera[5]);
      } else
           exit(1);
    } else // Lectura VMI
        lecturaArchivoVMI(&MaquinaVirtual);

    if(debug){
       desensamblado(&MaquinaVirtual);
    }

    if(MaquinaVirtual.Registros[ES] != -1){
        MaquinaVirtual.Memoria[MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[ES] >> 16].base] = 0;
        MaquinaVirtual.Memoria[MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[ES] >> 16].base + 1] = (MaquinaVirtual.Registros[ES] >> 16) & 0xFF;
        MaquinaVirtual.Memoria[MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[ES] >> 16].base + 2] = 0;
        MaquinaVirtual.Memoria[MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[ES] >> 16].base + 3] = 4;
    }

    if(cabecera[5] == 2)
        inicializaPila(&MaquinaVirtual,cantParametros,puntero);
    do {
        cargaoperacion(&MaquinaVirtual);

        // Mover IP
        MaquinaVirtual.Registros[IP] += 1 + shiftRightLogico(MaquinaVirtual.Registros[OP1], 24) + shiftRightLogico(MaquinaVirtual.Registros[OP2], 24);

        // Ejecuta instrucción
        if ((MaquinaVirtual.Registros[OPC]>=0x00 && MaquinaVirtual.Registros[OPC]<=0x08)||(MaquinaVirtual.Registros[OPC]>=11 && MaquinaVirtual.Registros[OPC]<=31)) //Si la funcion no es reconocida aborto proceso.
            operaciones[MaquinaVirtual.Registros[4]](&MaquinaVirtual);
        else
            errores(OPE);
    } while(MaquinaVirtual.Registros[IP] != -1 && logica_fisica(MaquinaVirtual, MaquinaVirtual.Registros[IP])!= -1);
    return 0;
}

void agregaParamSegment(VM *MaquinaVirtual, char **parametros, int cantParametros, int *puntero){
    int i, j, pos = 0, argv[cantParametros];

    if(cantParametros != 0) {
        for(i=0; i<cantParametros; i++){
            argv[i] = pos;
            j=0;
            while(parametros[i][j] != '\0') {
                MaquinaVirtual->Memoria[pos] = parametros[i][j];
                j++;
                pos++;
            }
            MaquinaVirtual->Memoria[pos] = '\0';
            pos++;
        }
        *puntero = pos;
        for(i=0; i<cantParametros; i++)
            for (j=0;j<4;j++){
                MaquinaVirtual->Memoria[pos] = (argv[i] >> (24-8*j)) & 0xFF;
                pos++;
            }
        MaquinaVirtual->Registros[PS] = 0;
        MaquinaVirtual->tabla_seg[0].base = 0;
        MaquinaVirtual->tabla_seg[0].tamano = pos; // Ver si era pos o pos-1
    } else
        MaquinaVirtual->Registros[PS] = -1;
}

void leerCabecera(unsigned char cabecera[8], VM MaquinaVirtual){
    FILE* arch;
    unsigned char byte;
    int i;
    arch = fopen(MaquinaVirtual.nombreVMX,"rb");
    if (arch){
        for (i=0; i<6; i++){
            fread(&byte,1,1,arch);
            cabecera[i]=byte;
        }
        if(cabecera[5] == 1)
            for(i=6; i<8; i++) {
               fread(&byte,1,1,arch);
               cabecera[i]=byte;
            }
        else if(cabecera[5] == 2)
            for(i=6; i<18; i++) {
               fread(&byte,1,1,arch);
               cabecera[i]=byte;
            }
        fclose(arch);
    }
    else{
        printf("No se pudo abrir el archivo.\n");
    }
}

int analizoValidez(unsigned char cabecera[]){
    int i=0;
    char version[5] = {'V', 'M', 'X', '2', '5'};
    while (i<5 && cabecera[i]==version[i])
        i++;
    return i+1==6 && (cabecera[i]==0x1 || cabecera[i]==0x2);
}

void iniciabilizarTablaSegmentosyRegistros(VM *MaquinaVirtual, unsigned char cabecera[]){
    int seg, tamano;

    if(cabecera[5] == 1){ // version 1
        MaquinaVirtual->tabla_seg[0].base=0;
        MaquinaVirtual->tabla_seg[0].tamano=cabecera[6]<<8 | cabecera[7];
        MaquinaVirtual->Registros[CS] = 0;
        MaquinaVirtual->tabla_seg[1].base=MaquinaVirtual->tabla_seg[0].tamano;
        MaquinaVirtual->tabla_seg[1].tamano=MaquinaVirtual->tamanoMemoria-MaquinaVirtual->tabla_seg[0].tamano;
        MaquinaVirtual->Registros[DS] = 1 << 16;
        MaquinaVirtual->Registros[PS] = -1;
        MaquinaVirtual->Registros[KS] = -1;
        MaquinaVirtual->Registros[ES] = -1;
        MaquinaVirtual->Registros[SS] = -1;
    } else { // version 2
        if(MaquinaVirtual->Registros[PS] != -1){
            seg = 1;
            tamano = MaquinaVirtual->tabla_seg[0].tamano;
        }
        else {
            seg = 0;
            tamano = 0;
        }


        // Const Segment
        if((cabecera[14]<<8 | cabecera[15]) != 0){
            if(seg)
                MaquinaVirtual->tabla_seg[seg].base = MaquinaVirtual->tabla_seg[seg-1].base + MaquinaVirtual->tabla_seg[seg-1].tamano;
            else
                MaquinaVirtual->tabla_seg[seg].base = 0;
            MaquinaVirtual->tabla_seg[seg].tamano = cabecera[14]<<8 | cabecera[15];
            MaquinaVirtual->Registros[KS] = seg << 16;
            tamano += MaquinaVirtual->tabla_seg[seg].tamano;
            seg++;
        } else
            MaquinaVirtual->Registros[KS] = -1;

        // Code Segment
        if(seg)
            MaquinaVirtual->tabla_seg[seg].base = MaquinaVirtual->tabla_seg[seg-1].base + MaquinaVirtual->tabla_seg[seg-1].tamano;
        else
            MaquinaVirtual->tabla_seg[seg].base = 0;
        MaquinaVirtual->tabla_seg[seg].tamano = cabecera[6]<<8 | cabecera[7];
        MaquinaVirtual->Registros[CS] = seg << 16;
        tamano += MaquinaVirtual->tabla_seg[seg].tamano;
        seg++;

        // Data Segment
        if((cabecera[8]<<8 | cabecera[9]) != 0){
            MaquinaVirtual->tabla_seg[seg].base = MaquinaVirtual->tabla_seg[seg-1].base + MaquinaVirtual->tabla_seg[seg-1].tamano;
            MaquinaVirtual->tabla_seg[seg].tamano = cabecera[8]<<8 | cabecera[9];
            MaquinaVirtual->Registros[DS] = seg << 16;
            tamano += MaquinaVirtual->tabla_seg[seg].tamano;
            seg++;
        } else
            MaquinaVirtual->Registros[DS] = -1;

        // Extra Segment
        if((cabecera[10]<<8 | cabecera[11]) != 0){
            MaquinaVirtual->tabla_seg[seg].base = MaquinaVirtual->tabla_seg[seg-1].base + MaquinaVirtual->tabla_seg[seg-1].tamano;
            MaquinaVirtual->tabla_seg[seg].tamano = cabecera[10]<<8 | cabecera[11];
            MaquinaVirtual->Registros[ES] = seg << 16;
            tamano += MaquinaVirtual->tabla_seg[seg].tamano;
            seg++;
        } else
            MaquinaVirtual->Registros[ES] = -1;

        // Stack Segment
        MaquinaVirtual->tabla_seg[seg].base = MaquinaVirtual->tabla_seg[seg-1].base + MaquinaVirtual->tabla_seg[seg-1].tamano;
        MaquinaVirtual->tabla_seg[seg].tamano = cabecera[12]<<8 | cabecera[13];
        MaquinaVirtual->Registros[SS] = seg << 16;
        tamano += MaquinaVirtual->tabla_seg[seg].tamano;
        seg++;

        // IP y SP
        MaquinaVirtual->Registros[IP] = MaquinaVirtual->Registros[CS] + (cabecera[16]<<8 | cabecera[17]);
        MaquinaVirtual->Registros[SP] = MaquinaVirtual->Registros[SS] + MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[SS] >> 16].tamano;

        if(tamano > MaquinaVirtual->tamanoMemoria)
            errores(MEM);
    }
}

void inicializaPila(VM *MaquinaVirtual,int cantParametros,int puntero){
    int i,pos;

    MaquinaVirtual->Registros[SP] -= 12;
    pos = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[SP]);
    for(i=0; i<4; i++){ // RET = -1 (porque es el del main)
        MaquinaVirtual->Memoria[pos] = 0xFF;
        pos++;
    }
    if(cantParametros != 0){
        for(i=0; i<4; i++){ // CantParametros
            MaquinaVirtual->Memoria[pos] = (cantParametros >> (24-8*i)) & 0xFF;
            pos++;
        }
        for(i=0; i<4; i++){ // Puntero a las posiciones de los parametros
            MaquinaVirtual->Memoria[pos] = (puntero >> (24-8*i)) & 0xFF;
            pos++;
        }
    } else {
        for(i=0; i<4; i++){ // CantParametros = 0
            MaquinaVirtual->Memoria[pos] = 0;
            pos++;
        }
        for(i=0; i<4; i++){ // Puntero a las posiciones de los parametros = -1
            MaquinaVirtual->Memoria[pos] = 0xFF;
            pos++;
        }
    }
}

void lecturaArchivo(VM *MaquinaVirtual, int version){
    FILE * arch;
    unsigned char byte;
    int i;

    arch=fopen(MaquinaVirtual->nombreVMX,"rb");
    if (arch){

        if(version==1)
           for(i=0;i<8;i++)
              fread(&byte,1,1,arch);
        else
           for(i=0;i<18;i++)
              fread(&byte,1,1,arch);

        fread(&byte,1,1,arch);
        i = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[CS]);
        while((i - MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[CS] >> 16].base) < MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[CS] >> 16].tamano){
            MaquinaVirtual->Memoria[i] = byte;
            i++;
            fread(&byte,1,1,arch);
        }

        if(!feof(arch)){
            i = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[KS]);
            while(!feof(arch)){
                MaquinaVirtual->Memoria[i]=byte;
                i++;
                fread(&byte,1,1,arch);
            }
        }
        fclose(arch);
    }
    else{
        printf("No se pudo abrir el archivo.\n");
    }
}

void lecturaArchivoVMI(VM *MaquinaVirtual){
    FILE* arch;
    unsigned char byte;
    char version[6] = {'V', 'M', 'I', '2', '5', 0x1};
    int i, j, n;

    arch = fopen(MaquinaVirtual->nombreVMI,"rb");
    if (arch){
        // Identificador y Versión
        for (i=0; i<6; i++){
            fread(&byte,1,1,arch);
            if(byte != version[i])
               exit(1);
        }

        // Espacio de memoria
        MaquinaVirtual->tamanoMemoria = 0;
        for(i=0; i<2; i++){
            fread(&byte,1,1,arch);
            MaquinaVirtual->tamanoMemoria += (byte << (8 - 8 * i));
        }

        // Registros
        for(i=0; i<32; i++){
            MaquinaVirtual->Registros[i] = 0;
            for(j=0; j<4; j++){
                fread(&byte,1,1,arch);
                MaquinaVirtual->Registros[i] += (byte << (24 - j * 8));
            }
        }

        // Tabla de segmentos
        for(i=0; i<8; i++){
            MaquinaVirtual->tabla_seg[i].base = 0;
            MaquinaVirtual->tabla_seg[i].tamano = 0;
            for(j=0; j<2; j++){
                fread(&byte,1,1,arch);
                MaquinaVirtual->tabla_seg[i].base += (byte << (8 - j * 8));
            }
            for(j=0; j<2; j++){
                fread(&byte,1,1,arch);
                MaquinaVirtual->tabla_seg[i].tamano += (byte << (8 - j * 8));
            }
        }

        // Memoria
        // N se calcula como si SP fuera el ultimo registro... arreglar para generalizar
        n = MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[SP] >> 16].base + MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[SP] >> 16].tamano;
        for(i=0; i<n; i++){
            fread(&byte,1,1,arch);
            MaquinaVirtual->Memoria[i] = byte;
        }

        fclose(arch);
    }
    else{
        printf("No se pudo abrir el archivo.\n");
    }
}

void creaArchivoVMI(VM MaquinaVirtual){
    FILE* arch;
    unsigned char byte;
    char version[6] = {'V', 'M', 'I', '2', '5', 0x1};
    int i, j, byte4, n, val;
    short int byte2;


    arch = fopen(MaquinaVirtual.nombreVMI,"wb");
    if (arch){
        // Identificador y Versión
        for (i=0; i<6; i++){
            byte = version[i];
            fwrite(&byte,1,1,arch);
        }

        // Espacio de memoria
        byte2 = (MaquinaVirtual.tamanoMemoria & 0xFF) << 8;
        byte2 |= (MaquinaVirtual.tamanoMemoria & 0xFF00) >> 8;
        fwrite(&byte2,2,1,arch);

        // Registros
        for(i=0; i<32; i++){
            val = MaquinaVirtual.Registros[i];
            byte4 = (val & 0x000000FF) << 24;
            byte4 |= (val & 0x0000FF00) << 8;
            byte4 |= (val & 0x00FF0000) >> 8;
            byte4 |= (val & 0xFF000000) >> 24;
            fwrite(&byte4,4,1,arch);
        }

        // Tabla de segmentos
        for(i=0; i<8; i++){
            byte2 = (MaquinaVirtual.tabla_seg[i].base & 0xFF) << 8;
            byte2 |= (MaquinaVirtual.tabla_seg[i].base & 0xFF00) >> 8;
            fwrite(&byte2,2,1,arch);
            byte2 = (MaquinaVirtual.tabla_seg[i].tamano & 0xFF) << 8;
            byte2 |= (MaquinaVirtual.tabla_seg[i].tamano & 0xFF00) >> 8;
            fwrite(&byte2,2,1,arch);
        }

        // Memoria
        n= MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[SS] >> 16].base + MaquinaVirtual.tabla_seg[MaquinaVirtual.Registros[SS] >> 16].tamano;
        for(i=0; i<n; i++){
            byte = MaquinaVirtual.Memoria[i];
            fwrite(&byte,1,1,arch);
        }

        fclose(arch);
    }
    else{
        printf("No se pudo abrir el archivo.\n");
    }
}

void cargaoperacion(VM *MaquinaVirtual){
    unsigned char byte;
    int aux, i, n1, n2, pos;

    // Guardo la dimension fisica de IP en pos y guardo el valor de memoria de esa posición
    pos = logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[IP]);
    byte = MaquinaVirtual->Memoria[pos];

    // Actualizo OPC
    MaquinaVirtual->Registros[OPC] = byte & 0x1F;

    // Actualizo el primer byte de OP1 y OP2
    if(((byte >> 4) & 1) == 0 && ((byte >> 5) & 1) == 0){
        MaquinaVirtual->Registros[OP1] = shiftRightLogico(byte,6); // OP1
        MaquinaVirtual->Registros[OP2] = 0; // OP2
    } else {
        MaquinaVirtual->Registros[OP1] = byte >> 4 & 0x03; // OP1
        MaquinaVirtual->Registros[OP2] = shiftRightLogico(byte,6); // OP2
    }
    MaquinaVirtual->Registros[OP1] <<= 24; // OP1
    MaquinaVirtual->Registros[OP2] <<= 24; // OP2

    // Lectura operandos 2 y 1
    n1 = shiftRightLogico(MaquinaVirtual->Registros[OP2], 24);
    aux = 0;
    for(i=0; i<n1; i++){
        pos++;
        byte = MaquinaVirtual->Memoria[pos];
        aux = aux | (byte & 0xFF) << (8 * (n1-i-1));
    }
    MaquinaVirtual->Registros[OP2] |= aux;

    n2 = shiftRightLogico(MaquinaVirtual->Registros[OP1], 24);
    aux = 0;
    for(i=0; i<n2; i++){
        pos++;
        byte = MaquinaVirtual->Memoria[pos];
        aux = aux | (byte & 0xFF) << (8 * (n2-i-1));
    }
    MaquinaVirtual->Registros[OP1] |= aux;
}

void getGeneral(VM *MaquinaVirtual, int operando,int *valor){
    switch((operando >> 24) & 0x3) { // REVISAR (Si diferenciar los otros registros de los de uso general)(Si se debe propagar el signo)
        case 1:
            switch((operando >> 6) & 0x3) {
                case 0: // EAX
                    *valor = MaquinaVirtual->Registros[operando & 0x1F];
                    break;
                case 1: // AL
                    *valor = ((MaquinaVirtual->Registros[operando & 0x1F] & 0xFF) << 24) >> 24;
                    break;
                case 2: // AL
                    *valor = ((MaquinaVirtual->Registros[operando & 0x1F] & 0xFF00) << 16) >> 24; // Checkear si esto esta bien
                    break;
                case 3: // AX
                    *valor = ((MaquinaVirtual->Registros[operando & 0x1F] & 0xFFFF) << 16) >> 16;
                    break;
            }
            break;
        case 2:
            *valor = (operando & 0xFFFF) << 16 >> 16;
            break;
        case 3:
            getMemoria(MaquinaVirtual,operando,valor);
            break;
    }
}

void setGeneral(VM *MaquinaVirtual, int operando, int valor){
    if(((operando >> 24) & 0x3) == 1)
        switch((operando >> 6) & 0x3) { // REVISAR (Si diferenciar los otros registros de los de uso general) (Si estan bien truncados los valores)
            case 0: // EAX
                MaquinaVirtual->Registros[operando & 0x1F] = valor;
                break;
            case 1: // AL
                MaquinaVirtual->Registros[operando & 0x1F] = (MaquinaVirtual->Registros[operando & 0x1F] & 0xFFFFFF00) | (valor & 0xFF);
                break;
            case 2: // AH
                MaquinaVirtual->Registros[operando & 0x1F] = (MaquinaVirtual->Registros[operando & 0x1F] & 0xFFFF00FF) | ((valor & 0xFF) << 8);
                break;
            case 3: // AX
                MaquinaVirtual->Registros[operando & 0x1F] = (MaquinaVirtual->Registros[operando & 0x1F] & 0xFFFF0000) | (valor & 0xFFFF);
                break;
        }
    else if(((operando >> 24) & 0x3) == 3)
        setMemoria(MaquinaVirtual,operando,valor);
    else
        errores(SET);
}

void actualizaCC(VM *MaquinaVirtual, int valor){
    if(valor > 0)
        MaquinaVirtual->Registros[CC] = 0;
    else if(valor == 0)
        MaquinaVirtual->Registros[CC] = 1 << 30;
    else
        MaquinaVirtual->Registros[CC] = 1 << 31;
}

unsigned int shiftRightLogico(int valor,int shift){
    unsigned int resultado = (unsigned) valor;
    return resultado >> shift;
}

void jmp(VM *MaquinaVirtual){
    MaquinaVirtual->Registros[IP] = ((MaquinaVirtual->Registros[IP] >> 16) << 16) | (MaquinaVirtual->Registros[OP1] & 0xFFFF);
}
void jz(VM *MaquinaVirtual){
    if(((MaquinaVirtual->Registros[CC] >> 30) & 1) == 1)
        jmp(MaquinaVirtual);
}
void jp(VM *MaquinaVirtual){
    if((((MaquinaVirtual->Registros[CC] >> 30) & 1) == 0) && (((MaquinaVirtual->Registros[CC] >> 31) & 1) == 0))
        jmp(MaquinaVirtual);
}
void jumpn(VM *MaquinaVirtual){
    if(((MaquinaVirtual->Registros[CC] >> 31) & 1) == 1)
        jmp(MaquinaVirtual);
}
void jnz(VM *MaquinaVirtual){
    if(((MaquinaVirtual->Registros[CC] >> 30) & 1) == 0)
        jmp(MaquinaVirtual);
}
void jnp(VM *MaquinaVirtual){
    if((((MaquinaVirtual->Registros[CC] >> 30) & 1) == 1) || (((MaquinaVirtual->Registros[CC] >> 31) & 1) == 1))
        jmp(MaquinaVirtual);
}
void jnn(VM *MaquinaVirtual){
    if(((MaquinaVirtual->Registros[CC] >> 31) & 1) == 0)
        jmp(MaquinaVirtual);
}
void not(VM *MaquinaVirtual){
    int A;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], ~A);
    actualizaCC(MaquinaVirtual, ~A);
}
void mov(VM *MaquinaVirtual){
    int B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], B);
}
void add(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A + B);
    actualizaCC(MaquinaVirtual, A + B);
}
void sub(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A - B);
    actualizaCC(MaquinaVirtual, A - B);
}
void mul(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A * B);
    actualizaCC(MaquinaVirtual, A * B);
}
void divv(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    if (B!=0){
        setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A / B);
        actualizaCC(MaquinaVirtual, A / B);
        MaquinaVirtual->Registros[AC] = A % B; //Actualizo el AC con el resto de la division
    }else
        errores(DIV0);
}
void cmp(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    actualizaCC(MaquinaVirtual, A - B);
}
void shl(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    if(B>=0){
       setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A << B);
       actualizaCC(MaquinaVirtual, A << B);
    } else
        errores(SHIFT);
}
void shr(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    if(B>=0){
       setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], shiftRightLogico(A,B));
       actualizaCC(MaquinaVirtual, shiftRightLogico(A,B));
    } else
        errores(SHIFT);
}
void sar(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    if(B>=0){
       setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A >> B);
       actualizaCC(MaquinaVirtual, A >> B);
    } else
        errores(SHIFT);
}
void and(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A & B);
    actualizaCC(MaquinaVirtual, A & B);
}
void or(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A | B);
    actualizaCC(MaquinaVirtual, A | B);
}
void xor(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A ^ B);
    actualizaCC(MaquinaVirtual, A ^ B);
}
void swap(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], A);
}
void ldl(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], (A & 0xFFFF0000) | (B & 0xFFFF));
}
void ldh(VM *MaquinaVirtual){
    int A,B;
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
    getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP2], &B);
    setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], (A & 0xFFFF) | ((B & 0xFFFF)) <<16 );
}
void rnd(VM *MaquinaVirtual){
    srand((int)time(NULL));
    int r,max;
    getGeneral(MaquinaVirtual,MaquinaVirtual->Registros[OP2],&max);
    if (max>0)
        r = rand() % (max+1);
    else
        r = 0;
    setGeneral(MaquinaVirtual,MaquinaVirtual->Registros[OP1],r);
}

void push(VM *MaquinaVirtual){
    int i, A, pos;
    MaquinaVirtual->Registros[SP] -= 4;
    if(MaquinaVirtual->Registros[SP] < MaquinaVirtual->Registros[SS])
        errores(SO);
    else {
        getGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], &A);
        MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[SP];
        MaquinaVirtual->Registros[MAR] = (4 << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]);
        MaquinaVirtual->Registros[MBR] = A;
        pos = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[SP]);
        for(i=0; i<4; i++){
            MaquinaVirtual->Memoria[pos] = (A >> (24-8*i)) & 0xFF;
            pos++;
        }
    }
}

void pop(VM *MaquinaVirtual){
    int i, A, pos;
    if(MaquinaVirtual->Registros[SP] + 4 >= MaquinaVirtual->Registros[SS] + MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[SS] >> 16].tamano)
        errores(SU);
    else {
        pos = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[SP]);
        A = 0;
        for(i=0; i<4; i++){
            A |= MaquinaVirtual->Memoria[pos] << (24-i*8);
            pos++;
        }
        MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[SP];
        MaquinaVirtual->Registros[MAR] = (4 << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]);
        MaquinaVirtual->Registros[MBR] = A;
        setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], A);
        MaquinaVirtual->Registros[SP] += 4;
    }
}

void call(VM *MaquinaVirtual){
    int i, pos;
    MaquinaVirtual->Registros[SP] -= 4;
    if(MaquinaVirtual->Registros[SP] < MaquinaVirtual->Registros[SS])
        errores(SO);
    else {
        pos = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[SP]);
        for(i=0; i<4; i++){
            MaquinaVirtual->Memoria[pos] = (MaquinaVirtual->Registros[IP] >> (24-8*i)) & 0xFF;
            pos++;
        }
        MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[SP];
        MaquinaVirtual->Registros[MAR] = (4 << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]);
        MaquinaVirtual->Registros[MBR] = MaquinaVirtual->Registros[IP];
        jmp(MaquinaVirtual);
    }
}

void ret(VM *MaquinaVirtual){
    int i, A, pos;
    if(MaquinaVirtual->Registros[SP] + 4 >= MaquinaVirtual->Registros[SS] + MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[SS] >> 16].tamano)
        errores(SU);
    else {
        pos = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[SP]);
        A = 0;
        for(i=0; i<4; i++){
            A |= MaquinaVirtual->Memoria[pos] << (24-i*8);
            pos++;
        }
        MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[SP];
        MaquinaVirtual->Registros[MAR] = (4 << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]);
        MaquinaVirtual->Registros[MBR] = A;
        MaquinaVirtual->Registros[IP] = A;
        MaquinaVirtual->Registros[SP] += 4;
    }
}

void sys(VM *MaquinaVirtual){
    //CALCULAR LIMITES INICIO DS Y FINAL DS
    int tarea = (MaquinaVirtual->Registros[OP1]) & 0xF;
    int numCeldas = MaquinaVirtual->Registros[ECX] & 0xFFFF; //Tomo los 2 bytes menos significativos de ecx
    int numBytes = MaquinaVirtual->Registros[ECX] & 0xFFFF0000; // Aplico mascara para que quede en 0 todo menos los 2 bytes mas significativos de ecx
    numBytes = shiftRightLogico(numBytes,16);
    int eax = MaquinaVirtual->Registros[EAX]; //Formato de entrada/salida de los datos
    if (tarea>=1 && tarea<=4){
        MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[EDX]; //Al LAR le doy el valor de EDX, ya que es contiene la posicion de memoria a la cual se accede
        MaquinaVirtual->Registros[MAR] = ((numCeldas*numBytes) << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]); //Modifico el MAR en base a el- LAR
    }
    if (tarea == 1){ //Escribir en memoria //VERIFICAR LIMITES
        int i;
        long int celda, base;
        base = MaquinaVirtual->Registros[EDX];
        for (i=0;i<numCeldas;i++){
            int valido=1;
            long int valor=0;
            long int valorMax, valorMin;
            valorMin = -(1 << (8*numBytes - 1));
            valorMax = (1 << (8*numBytes - 1)) - 1;
            do{
                celda = logica_fisica(*MaquinaVirtual, base+i*numBytes);
                printf("[%04X]: ", celda);
                switch(eax){
                    case 0x01: {
                        if(scanf(" %d",&valor)!=1){
                            printf("Entrada invalida \n");
                            valido = 0; //Fuerzo repetir
                        }
                    } break;
                    case 0x02:{
                        char c;
                        if(scanf(" %c",&c)!=1){
                            //printf("Entrada invalida \n");
                            valido = 0; //Fuerzo repetir
                        }
                        valor = (unsigned char) c;
                    }break;
                    case 0x04:{ //Octal
                        if(scanf(" %lo",&valor)!=1){
                            printf("Entrada invalida \n");
                            valido = 0; //Fuerzo repetir
                        }
                    }break;
                    case 0x08:{ //Hexadecimal
                        if(scanf(" %lx",&valor)!=1){
                            printf("Entrada invalida \n");
                            valido = 0; //Fuerzo repetir
                        }
                    }break;
                    case 0x10:{
                        char bits[65]; //Numero de 64 bits y \n
                        int k;
                        if (scanf(" %64s",bits)!=1){
                            printf("Entrada Invalida \n");
                            valido=0;
                        }
                        for (k=0;bits[k]!='\0';k++){
                            valor = valor << 1;
                            if (bits[k]=='1')
                                valor |=1;
                            else if(bits[k]!='0'){
                                break;
                                valido = 0;
                            }
                        }
                    }break;
                    default: {
                        printf("Metodo invalido \n");
                        //TERMINAR SYS
                    }
                }
                if (!(valido && (valor>=valorMin && valor<=valorMax)))
                    valido = 0;
            }while (!valido);
            int j;
            long int posicion;
            for (j=0;j<numBytes;j++){
                posicion = logica_fisica(*MaquinaVirtual, base + i*numBytes + j);
                MaquinaVirtual->Memoria[posicion] = shiftRightLogico(valor, 8*(numBytes-j-1));
            }

        }
    }else if(tarea==2){ //Leer de memoria VERIFICAR LIMITES
        int i;
        long int celda, posicion, base;
        base = MaquinaVirtual->Registros[EDX];
        celda = logica_fisica(*MaquinaVirtual, base);
        for (i=0;i<numCeldas;i++){
            long int valor=0;
            int j;
            long int posicion;
            printf("[%04X]: ", celda);
            celda += numBytes;
            for (j=0;j<numBytes;j++){
                posicion=logica_fisica(*MaquinaVirtual,base + i*numBytes + j);
                unsigned char aux = MaquinaVirtual->Memoria[posicion];
                valor = (valor << 8) | aux;
            }
            int k=4;
            while (k>-1){
                int bit = shiftRightLogico(eax,k) & 1;
                if (bit){
                    switch(k){
                        case 0: printf("%d ", valor); break; //Decimal
                        case 1: {
                            int caracter;
                            for (caracter=0;caracter<numBytes;caracter++){
                            char aux = shiftRightLogico(valor,8*(numBytes-1-caracter));
                                if (isprint((unsigned char)aux)){
                                    printf("%c", (char)aux);
                                } else {
                                    printf(".");
                                }
                            }
                        } break; //Caracteres
                        case 2: printf("%o ", valor); break; //Octal
                        case 3: printf("%X ", valor); break; //Hexadecimal
                        case 4: {
                            int k;
                            for (k = 8*numBytes-1; k>=0; k--){
                                printf("%d", (shiftRightLogico(valor,k) & 1));
                                if (k % 4 == 0 && k != 0)
                                    printf(" ");
                            }
                        } break;
                        default:{
                            printf("Metodo invalido \n");
                        }
                    }
                }
            k--;
            }
            printf("\n");
        }
    }else if (tarea == 3){
        int i, base, posicion;
        int caracteres = MaquinaVirtual->Registros[ECX];
        char car;
        base = MaquinaVirtual->Registros[EDX];
        char cadena[1024];
        scanf("%s",cadena);
        i=0;
        car = cadena[i];
        while ((i<caracteres || caracteres==-1) && car!='\0'){
            posicion = logica_fisica(*MaquinaVirtual, base);
            MaquinaVirtual->Memoria[posicion] = car;
            i++;
            base++;
            car = cadena[i];
        }
        base +=1;
        posicion = logica_fisica(*MaquinaVirtual, base);
        MaquinaVirtual->Memoria[posicion] = '\0';
        fflush(stdin);
    } else if(tarea == 4){
        int i, base, posicion;
        char car;
        base = MaquinaVirtual->Registros[EDX];
        posicion = logica_fisica(*MaquinaVirtual, base);
        car = MaquinaVirtual->Memoria[posicion];
        printf("[%04X]: ", posicion);
        while (car!='\0'){
            printf("%c", car);
            base++;
            posicion = logica_fisica(*MaquinaVirtual, base);
            car = MaquinaVirtual->Memoria[posicion];
        }
        fflush(stdin);
    } else if (tarea == 7){
        system("cls");
    } else if (tarea == 15){ //Breakpoint
            manejarBreakpoint(MaquinaVirtual);
    } else {
        printf("Llamada desconocida. \n");
        exit(1);
    }
}

void stop(VM *MaquinaVirtual){
    MaquinaVirtual->Registros[IP] = -1;
}

void ejecutarInstruccion(VM *MaquinaVirtual){
    cargaoperacion(MaquinaVirtual);
    // Mover IP
    MaquinaVirtual->Registros[IP] += 1 + shiftRightLogico(MaquinaVirtual->Registros[OP1], 24) + shiftRightLogico(MaquinaVirtual->Registros[OP2], 24);
    // Ejecuta instrucción
    if ((MaquinaVirtual->Registros[OPC]>=0x00 && MaquinaVirtual->Registros[OPC]<=0x08)||(MaquinaVirtual->Registros[OPC]>=11 && MaquinaVirtual->Registros[OPC]<=31)) //Si la funcion no es reconocida aborto proceso.
        operaciones[MaquinaVirtual->Registros[4]](MaquinaVirtual);
    else
        errores(OPE);
    if (MaquinaVirtual->Registros[IP]==-1)
        exit(0); //Termino la ejecucion de programa sin errores
    // Si no termino el programa debo regresar al manejo del Breakpoint
}

void manejarBreakpoint(VM *MaquinaVirtual){
    if (strcmp(MaquinaVirtual->nombreVMI,"")){
        creaArchivoVMI(*MaquinaVirtual);
        char opcion = getchar();
        if (opcion == 'q')
            MaquinaVirtual->Registros[IP]=-1;
        else if (opcion == '\n'){
            ejecutarInstruccion(MaquinaVirtual);
            manejarBreakpoint(MaquinaVirtual);
        }
        else if (opcion=='g'){}
    }
}

void desensamblado(VM *MaquinaVirtual){
    short int posfisica;
    int i, inc, n, puntero;
    char cadena[200], registro[5];
    unsigned char byte;
    int aux, n1, n2, pos;

    if(MaquinaVirtual->Registros[KS] != -1) {
        puntero = MaquinaVirtual->Registros[KS];

        while((puntero & 0xFFFF) < MaquinaVirtual->tabla_seg[shiftRightLogico(puntero,16)].tamano) {
            posfisica = logica_fisica(*MaquinaVirtual, puntero);
            printf(" [%04X] ", posfisica);

            i=0;
            while(MaquinaVirtual->Memoria[posfisica] != 0) {
                cadena[i] = MaquinaVirtual->Memoria[posfisica];
                i++;
                posfisica++;
                puntero++;
            }
            cadena[i+1] = 0;
            puntero++;

            n=i+1;
            i=0;
            while(i<n && i<6) {
                printf("%02X ",cadena[i]);
                i++;
            }

            if(n==7)
                printf("00 ");
            else if(n>7)
                printf(".. ");
            else
                for(i=n; i<7;i++)
                    printf("   ");

            printf(" | \"");
            n = strlen(cadena);
            for(i=0; i<n; i++)
                if (isprint((unsigned char)cadena[i]))
                    printf("%c", (char)cadena[i]);
                else
                    printf(".");
            printf("\"\n");
        }
    }
    puntero = MaquinaVirtual->Registros[CS];
    do {
        // CARGA LA OPERACION
        pos = logica_fisica(*MaquinaVirtual, puntero);
        byte = MaquinaVirtual->Memoria[pos];

        MaquinaVirtual->Registros[OPC] = byte & 0x1F;

        if(((byte >> 4) & 1) == 0 && ((byte >> 5) & 1) == 0){
            MaquinaVirtual->Registros[OP1] = shiftRightLogico(byte,6);
            MaquinaVirtual->Registros[OP2] = 0;
        } else {
            MaquinaVirtual->Registros[OP1] = byte >> 4 & 0x03;
            MaquinaVirtual->Registros[OP2] = shiftRightLogico(byte,6);
        }
        MaquinaVirtual->Registros[OP1] <<= 24;
        MaquinaVirtual->Registros[OP2] <<= 24;

        n1 = shiftRightLogico(MaquinaVirtual->Registros[OP2], 24);
        aux = 0;
        for(i=0; i<n1; i++){
            pos++;
            byte = MaquinaVirtual->Memoria[pos];
            aux = aux | (byte & 0xFF) << (8 * (n1-i-1));
        }
        MaquinaVirtual->Registros[OP2] |= aux;

        n2 = shiftRightLogico(MaquinaVirtual->Registros[OP1], 24);
        aux = 0;
        for(i=0; i<n2; i++){
            pos++;
            byte = MaquinaVirtual->Memoria[pos];
            aux = aux | (byte & 0xFF) << (8 * (n2-i-1));
        }
        MaquinaVirtual->Registros[OP1] |= aux;
        // TERMINA CARGA DE OPERACION

        if(puntero == MaquinaVirtual->Registros[IP])
            printf(">");
        else
            printf(" ");

        posfisica = logica_fisica(*MaquinaVirtual, puntero);
        printf("[%04X] ", posfisica);

        inc = 1 + shiftRightLogico(MaquinaVirtual->Registros[OP1], 24) + shiftRightLogico(MaquinaVirtual->Registros[OP2], 24);
        for(i=posfisica; i<posfisica + inc; i++)
            printf("%02X ",MaquinaVirtual->Memoria[i]);

        for(i=0; i<7-inc;i++)
            printf("   ");

        printf(" |  %s ",Mnemonicos[MaquinaVirtual->Registros[OPC]]);

        if(((MaquinaVirtual->Registros[OP1] >> 24) & 0x3)==1 || ((MaquinaVirtual->Registros[OP1] >> 24) & 0x3)==3) {
            if(((MaquinaVirtual->Registros[OP1] >> 24) & 0x3)==3){
                aux = MaquinaVirtual->Registros[OP1] >> 16;
                switch((MaquinaVirtual->Registros[OP1] >> 22) & 0x3) {
                    case 2:
                        byte = 'w';
                        break;
                    case 3:
                        byte = 'b';
                        break;
                    case 0:
                        byte = 'l';
                        break;
                }
            }
            else
                aux = MaquinaVirtual->Registros[OP1];

            strcpy(registro, registros[aux & 0x1F]);
            if(((MaquinaVirtual->Registros[OP1] >> 24) & 0x3)==1)
                switch((MaquinaVirtual->Registros[OP1] >> 6) & 0x3) {
                    case 0:
                        strcpy(registro, registros[aux & 0x1F]);
                        break;
                    case 1:
                        strcpy(registro, registros8l[(aux & 0x1F) - 10]);
                        break;
                    case 2:
                        strcpy(registro, registros8h[(aux & 0x1F) - 10]);
                        break;
                    case 3:
                        strcpy(registro, registros16[(aux & 0x1F) - 10]);
                        break;
                }
        }
        switch((MaquinaVirtual->Registros[OP1] >> 24) & 0x3) {
            case 1:
                printf("%s",registro);
                break;
            case 2:
                if(MaquinaVirtual->Registros[OPC] >= 1 && MaquinaVirtual->Registros[OPC] <= 7 || MaquinaVirtual->Registros[OPC] == 13)
                    printf("%X",((MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16) + MaquinaVirtual->tabla_seg[MaquinaVirtual->Registros[CS] >> 16].base);
                else
                    printf("%d",(MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16);
                break;
            case 3:
                if((MaquinaVirtual->Registros[OP1] & 0xFFFF) == 0)
                    printf("%c[%s]",byte,registro);
                else if((MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16 > 0)
                    printf("%c[%s + %d]",byte,registro,(MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16);
                else
                    printf("%c[%s - %d]",byte,registro,-((MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16));
                break;
        }

        if(((MaquinaVirtual->Registros[OP2] >> 24) & 0x3) != 0)
            printf(", ");

        if(((MaquinaVirtual->Registros[OP2] >> 24) & 0x3)==1 || ((MaquinaVirtual->Registros[OP2] >> 24) & 0x3)==3) {
            if(((MaquinaVirtual->Registros[OP2] >> 24) & 0x3)==3){
                aux = MaquinaVirtual->Registros[OP2] >> 16;
                switch((MaquinaVirtual->Registros[OP2] >> 22) & 0x3) {
                    case 2:
                        byte = 'w';
                        break;
                    case 3:
                        byte = 'b';
                        break;
                    case 0:
                        byte = 'l';
                        break;
                }
            }
            else
                aux = MaquinaVirtual->Registros[OP2];

            strcpy(registro, registros[aux & 0x1F]);
            if(((MaquinaVirtual->Registros[OP2] >> 24) & 0x3)==1)
                switch((MaquinaVirtual->Registros[OP2] >> 6) & 0x3) {
                    case 0:
                        strcpy(registro, registros[aux & 0x1F]);
                        break;
                    case 1:
                        strcpy(registro, registros8l[(aux & 0x1F) - 10]);
                        break;
                    case 2:
                        strcpy(registro, registros8h[(aux & 0x1F) - 10]);
                        break;
                    case 3:
                        strcpy(registro, registros16[(aux & 0x1F) - 10]);
                        break;
                }
        }
        switch((MaquinaVirtual->Registros[OP2] >> 24) & 0x3) {
            case 1:
                printf("%s",registro);
                break;
            case 2:
                printf("%d ",(MaquinaVirtual->Registros[OP2] & 0xFFFF) << 16 >> 16);
                break;
            case 3:
                if((MaquinaVirtual->Registros[OP2] & 0xFFFF) == 0)
                    printf("%c[%s]",byte,registro);
                else if((MaquinaVirtual->Registros[OP2] & 0xFFFF) << 16 >> 16 > 0)
                    printf("%c[%s + %d]",byte,registro,(MaquinaVirtual->Registros[OP2] & 0xFFFF) << 16 >> 16);
                else
                    printf("%c[%s - %d]",byte,registro,-((MaquinaVirtual->Registros[OP2] & 0xFFFF) << 16 >> 16));
                break;
        }
        printf("\n");

        puntero += inc;
    }while((puntero & 0xFFFF) < MaquinaVirtual->tabla_seg[shiftRightLogico(puntero,16)].tamano);
}

void errores(int error) {
    switch(error) {
        case 1:
            printf("Instruccion Invalida. Abortando proceso.");
            break;
        case 2:
            printf("No es posible dividir por cero. Proceso detenido.");
            break;
        case 3:
            printf("Segmentation Fall. Se produjo una invasion de memoria.");
            break;
        case 4:
            printf("Fallo de Segmento. Codigo de segmento inexistente.");
            break;
        case 5:
            printf("No se le puede asignar un valor a un operando de tipo inmediato.");
            break;
        case 6:
            printf("Llamada al sistema erronea. Abortando proceso.");
            break;
        case 7:
            printf("No se permite shiftear una cantidad negativa de bits. Abortando proceso.");
            break;
        case 8:
            printf("Falta de memoria.");
            break;
        case 9:
            printf("Stack Overflow.");
            break;
        case 10:
            printf("Stack Underflow.");
            break;
        default:
            printf("Error desconocido.");
    }
    exit(1);
}
