#include "acceso_memoria.h"
#include <stdio.h>
#include <stdlib.h>

int logica_fisica(VM MaquinaVirtual, int registro){
    int base,offset, valor;

    if(shiftRightLogico(registro,16)>=0 && shiftRightLogico(registro,16)<CANTSEG){
        base = MaquinaVirtual.tabla_seg[shiftRightLogico(registro,16)].base;
        offset = registro & 0xFFFF;
        if(offset < MaquinaVirtual.tabla_seg[shiftRightLogico(registro,16)].tamano) // Si no se cumple esta condicion hubo una invasion de memoria - Segmentation Fold
            valor = base + offset;
        else if(registro == MaquinaVirtual.Registros[3])
            valor = -1;
        else
            errores(SEG);
        return valor;
    }else
        errores(COD);
}

void modificoLAR_MAR(VM *MaquinaVirtual, int operando){
    int numBytes = 4 - shiftRightLogico(operando,22) & 0x3;
    MaquinaVirtual->Registros[LAR] = (operando & 0xFFFF) + (MaquinaVirtual->Registros[(shiftRightLogico(operando,16)) & 0x1F]); // Armo el LAR 2 primeros bytes del segmento y los 2 ultimos el offset
    MaquinaVirtual->Registros[MAR] = (numBytes << 16) | logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[LAR]); //Paso el LAR para que me calcule la posicion fisica de este y almacenarlo en el MAR
}

void getMemoria(VM *MaquinaVirtual, int operando, int *valor){
    int i,aux,tamCelda;
    long int pos,base;
    tamCelda = 4 - (shiftRightLogico(operando,22) & 0x3);
    modificoLAR_MAR(MaquinaVirtual, operando);
    base = operando;
    pos = logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[(base >> 16) & 0x1F] + (base & 0xFFFF));
    *valor=0;
    for (i=0;i<tamCelda;i++){
        aux = MaquinaVirtual->Memoria[pos];
        aux = aux << (8*(tamCelda-1) - 8*i); //A Medida que voy leyendo, los bits que leo son menos significativos, entonces los voy corriendo menos
        *valor |= aux;
        pos++;
    }
    MaquinaVirtual->Registros[MBR] = *valor; //El valor trabajado en la memoria lo copio directo en la MBR
}

void setMemoria(VM *MaquinaVirtual, int operando, int valor){
    int i, numCeldas;
    long int pos, base;
    modificoLAR_MAR(MaquinaVirtual, operando);
    numCeldas = 4 - (shiftRightLogico(operando,22) & 0x3);
    base = operando;
    pos = logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[(base >> 16) & 0x1F] + (base & 0xFFFF));
    for (i=0;i<numCeldas;i++){
        MaquinaVirtual->Memoria[pos] = valor >> (8 * (numCeldas - i - 1)) & 0xFF;
        pos++;
    }
    MaquinaVirtual->Registros[MBR] = valor; //El valor trabajado en la memoria lo copio directo en la MBR
}
