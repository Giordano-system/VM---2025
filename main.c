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
//Registros
#define LAR  0
#define MAR  1
#define MBR  2
#define IP   3
#define OPC  4
#define OP1  5
#define OP2  6
#define EAX 10
#define EBX 11
#define ECX 12
#define EDX 13
#define EEX 14
#define EFX 15
#define AC   16
#define CC   17
#define CS   26
#define DS   27

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
    "-",
    "-",
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
    "-",
    "-",
    "-",
    "-"
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
void stop(VM *);

puntFunc operaciones[0x20] = {
    sys,jmp,jz,jp,jumpn,jnz,jnp,jnn,not,NULL,NULL,NULL,NULL,NULL,NULL,stop,mov,add,
    sub,mul,divv,cmp,shl,shr,sar,and,or,xor,swap,ldl,ldh,rnd
};

void leerCabecera(unsigned char [], char []);
int analizoValidez(unsigned char []);
void iniciabilizarTablaSegmentos(VM *,unsigned char []);
void lecturaArchivo(VM *,char []);
void inicializoRegistros(VM *);
void cargaoperacion(VM *);
int logica_fisica(VM, int);
void getGeneral(VM *,int,int *);
void setGeneral(VM *,int,int);
void actualizaCC(VM *,int);
unsigned int shiftRightLogico(int,int);
void desensamblado(VM *);
void errores(int);


int main(int argc, char *argv[]){
    VM MaquinaVirtual;
    unsigned char cabecera[8];

    leerCabecera(cabecera,argv[1]);
    if (analizoValidez(cabecera)){
        iniciabilizarTablaSegmentos(&MaquinaVirtual,cabecera);
        inicializoRegistros(&MaquinaVirtual);
        lecturaArchivo(&MaquinaVirtual,argv[1]);
        if(argc == 3 && strcmp(argv[2],"-d") == 0)
            desensamblado(&MaquinaVirtual);
        MaquinaVirtual.Registros[IP] = MaquinaVirtual.Registros[CS]; // Reinicio IP
        do {
            cargaoperacion(&MaquinaVirtual);

            // Mover IP
            MaquinaVirtual.Registros[IP] += 1 + shiftRightLogico(MaquinaVirtual.Registros[OP1], 24) + shiftRightLogico(MaquinaVirtual.Registros[OP2], 24);

            // Ejecuta instrucción
            if ((MaquinaVirtual.Registros[OPC]>=0x00 && MaquinaVirtual.Registros[OPC]<=0x08)||(MaquinaVirtual.Registros[OPC]>=15 && MaquinaVirtual.Registros[OPC]<=31)) //Si la funcion no es reconocida aborto proceso.
                operaciones[MaquinaVirtual.Registros[4]](&MaquinaVirtual);
            else
                errores(OPE);
        } while(MaquinaVirtual.Registros[IP] != -1 && logica_fisica(MaquinaVirtual, MaquinaVirtual.Registros[IP])!= -1);
    }
    return 0;
}

void leerCabecera(unsigned char cabecera[8], char nombre[]){
    FILE* arch;
    unsigned char byte;
    int i;
    arch = fopen(nombre,"rb");
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

int analizoValidez(unsigned char cabecera[]){
    int i=0;
    char version[6] = {'V', 'M', 'X', '2', '5', 0x01};
    while (i<6 && cabecera[i]==version[i])
        i++;
    return i==6;
}

void iniciabilizarTablaSegmentos(VM *MaquinaVirtual, unsigned char cabecera[]){
    MaquinaVirtual->tabla_seg[0].base=0;
    MaquinaVirtual->tabla_seg[0].tamano=cabecera[6]<<8 | cabecera[7];
    MaquinaVirtual->tabla_seg[1].base=MaquinaVirtual->tabla_seg[0].tamano;
    MaquinaVirtual->tabla_seg[1].tamano=16384-MaquinaVirtual->tabla_seg[0].tamano;
}

void lecturaArchivo(VM *MaquinaVirtual, char nombre[]){
    FILE * arch;
    unsigned char byte;
    int i;

    arch=fopen(nombre,"rb");
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
    MaquinaVirtual->Registros[CS]=0; //Asigno CS
    MaquinaVirtual->Registros[DS]=1 << 16; //Asigno DS
    MaquinaVirtual->Registros[IP]=MaquinaVirtual->Registros[CS]; //Asigno CS a IP
}

void cargaoperacion(VM *MaquinaVirtual){
    char byte;
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
    switch((operando >> 24) & 0x3) {
        case 1:
            *valor = MaquinaVirtual->Registros[operando & 0x1F];
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
        MaquinaVirtual->Registros[operando & 0x1F] = valor;
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
        int q = A / B;
        int r = A % B;
        if (r != 0 && ((r > 0) != (B > 0))) {
            q--;
            r += B;
        }
        setGeneral(MaquinaVirtual, MaquinaVirtual->Registros[OP1], q);
        actualizaCC(MaquinaVirtual, q);
        MaquinaVirtual->Registros[AC] = r; //Actualizo el AC con el resto de la division
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

void sys(VM *MaquinaVirtual){
    int tarea = (MaquinaVirtual->Registros[OP1]) & 0x3;
    int numCeldas = MaquinaVirtual->Registros[ECX] & 0xFFFF; //Tomo los 2 bytes menos significativos de ecx
    int numBytes = MaquinaVirtual->Registros[ECX] & 0xFFFF0000; // Aplico mascara para que quede en 0 todo menos los 2 bytes mas significativos de ecx
    numBytes = shiftRightLogico(numBytes,16);
    int eax = MaquinaVirtual->Registros[EAX]; //Formato de entrada/salida de los datos
    MaquinaVirtual->Registros[LAR] = MaquinaVirtual->Registros[EDX]; //Al LAR le doy el valor de EDX, ya que es contiene la posicion de memoria a la cual se accede
    MaquinaVirtual->Registros[MAR] = ((numCeldas*numBytes) << 16) | logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[LAR]); //Modifico el MAR en base a el- LAR
    if (tarea == 1){ //Escribir en memoria
        int i, celda, base;
        base = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[EDX]);
        celda = base;
        for (i=0;i<numCeldas;i++){
            int valido=1;
            long int valor=0;
            long int valorMax, valorMin;
            valorMin = -(1 << (8*numBytes - 1));
            valorMax = (1 << (8*numBytes - 1)) - 1;
            celda += numBytes;
            do{
                printf("[%04X]: ", base+i*numBytes);
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
                posicion = base + i*numBytes + j;
                MaquinaVirtual->Memoria[posicion] = shiftRightLogico(valor, 8*(numBytes-j-1));
            }

        }
    }else if(tarea==2){ //Leer de memoria
        int i, celda, base;
        base = logica_fisica(*MaquinaVirtual, MaquinaVirtual->Registros[EDX]);
        celda = base;
        for (i=0;i<numCeldas;i++){
            long int valor=0;
            int j;
            long int posicion;
            printf("[%04X] ", celda);
            celda += numBytes;
            for (j=0;j<numBytes;j++){
                posicion=base + i*numBytes + j;
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
                        int caracter,bandera=0;
                        for (caracter=0;caracter<numBytes;caracter++){
                            char aux = shiftRightLogico(valor,8*(numBytes-1-caracter));
                            if (isprint((unsigned char)aux)){
                                printf("%c", (char)aux);
                                bandera=1;
                            }
                        }
                        if (bandera)
                            printf(" ");
                        else
                            printf(". ");
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
    }else
        errores(SYS);
}

void stop(VM *MaquinaVirtual){
    MaquinaVirtual->Registros[IP] = -1;
}

void desensamblado(VM *MaquinaVirtual){
    short int posfisica;
    int i, inc, n;
    char cadena[200];
    char byte;
    int aux, n1, n2, pos, registro;

    if(MaquinaVirtual->Registros[KS] != -1) {
        puntero = MaquinaVirtual->Registros[KS];

        while((puntero & 0xFFFF) < MaquinaVirtual.tabla_seg[shiftRightLogico(puntero,16)].tamano) {
            posfisica = logica_fisica(*MaquinaVirtual, puntero);
            printf(" [%04X] ", posfisica);

            i=0;
            while(MaquinaVirtual.Memoria[posfisica] != 0) {
                cadena[i] = MaquinaVirtual.Memoria[posfisica];
                i++;
                posfisica++;
                puntero++;
            }
            cadena[i+1] = 0;

            n=i+1;
            i=0;
            while(i<n && i<6)
                printf("%02X ",cadena[i]);

            if(n==7)
                printf("00 ");
            else if(n>7)
                printf(".. ");
            else
                for(i=n; i<7;i++)
                    printf("   ");

            printf(" |  \"%s\"\n", cadena); // Probar que hace si hay caracteres no imprimibles
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
                printf("%d",(MaquinaVirtual->Registros[OP1] & 0xFFFF) << 16 >> 16);
                break;
            case 3:
                if((MaquinaVirtual->Registros[OP1] & 0xFFFF) == 0)
                    printf("%c[%s]",byte,registro);
                else
                    printf("%c[%s + %d]",byte,registro,MaquinaVirtual->Registros[OP1] & 0xFFFF);
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
                else
                    printf("%c[%s + %d]",byte,registro,MaquinaVirtual->Registros[OP2] & 0xFFFF);
                break;
        }
        printf("\n");

        puntero += inc;
    }while((puntero & 0xFFFF) < MaquinaVirtual.tabla_seg[shiftRightLogico(puntero,16)].tamano);
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
            printf("Fallo de Segmento. Código de segmento inexistente.");
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
        default:
            printf("Error desconocido.");
    }
    exit(1);
}
