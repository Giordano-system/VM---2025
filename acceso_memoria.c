#include "acceso_memoria.h"
#include <stdio.h>
#include <stdlib.h>

int logica_fisica(VM MaquinaVirtual, int registro){
    int base,offset, valor;
    base = MaquinaVirtual.tabla_seg[shiftRightLogico(registro,16)].base;
    offset = registro & 0xFFFF;
    if(offset < MaquinaVirtual.tabla_seg[shiftRightLogico(registro,16)].tamano) // Si no se cumple esta condicion hubo una invasion de memoria - Segmentation Fold
        valor = base + offset;
    else{
        valor = -1;
        printf("Se produjo una invasion de memoria. Segmentation Fall");
        exit(1);
    }
    return valor;
}

void modificoLAR_MAR(VM *MaquinaVirtual, int operando){
    MaquinaVirtual->Registros[LAR] = (operando & 0xFFFF) + (MaquinaVirtual->Registros[(shiftRightLogico(operando,16)) & 0x1F]); // Armo el LAR 2 primeros bytes del segmento y los 2 ultimos el offset
    MaquinaVirtual->Registros[MAR] = (4 << 16) | logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[LAR]); //Paso el LAR para que me calcule la posicion fisica de este y almacenarlo en el MAR
}

void getMemoria(VM *MaquinaVirtual, int operando, int *valor){
    int pos,i,aux;
    modificoLAR_MAR(MaquinaVirtual, operando);
    pos = MaquinaVirtual->Registros[MAR] & 0xFFFF; // pos tiene la direccion fisica para acceder a memoria
    *valor = MaquinaVirtual->Memoria[pos];
    *valor = *valor << 24; //Los primeros 8 bits que leo son los mas significativos, entonces los corro (ver posibilidad de GENERALIZAR)
    // El tope deberia ser MAR >> 16, aca se asume que es siempre 4 (habria que generalizar)
    for (i=0;i<3;i++){
        pos+=1;
        aux = MaquinaVirtual->Memoria[pos];
        aux = aux << (16-i*8); //A Medida que voy leyendo, los bits que leo son menos significativos, entonces los voy corriendo menos
        *valor |= aux;
    }
    MaquinaVirtual->Registros[MBR] = *valor; //El valor trabajado en la memoria lo copio directo en la MBR
}

void setMemoria(VM *MaquinaVirtual, int operando, int valor){
    int pos,i;
    modificoLAR_MAR(MaquinaVirtual, operando);
    pos = MaquinaVirtual->Registros[MAR] & 0xFFFF;
    MaquinaVirtual->Memoria[pos] = (valor >> 24) & 0xFF; //Asigno el byte mas significativo de valor a la primera posicion de memoria ocupada por el valor
    for (i=0;i<3;i++){
        pos +=1;
        MaquinaVirtual->Memoria[pos] = (valor >> (16-8*i)) & 0xFF;
    }
    MaquinaVirtual->Registros[MBR] = valor; //El valor trabajado en la memoria lo copio directo en la MBR
}
