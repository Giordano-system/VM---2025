#include "acceso_memoria.h"

int logica_fisica(VM MaquinaVirtual, int registro){
    int base,offset;
    base = MaquinaVirtual.tabla_seg[registro >> 16].base;
    offset = registro & 0xFFFF;
    return base + offset;
}

void getVM(VM *MaquinaVirtual, int operando, int *valor){
    int pos,i,aux;
    MaquinaVirtual->Registros[0] = operando & 0x0000FFFF | 1 << 8; //Tomo el offset del operando y se lo asigno a la LAR
    pos = operando & 0x00FFFFFF;
    *valor = MaquinaVirtual->Memoria[pos];
    *valor = *valor << 24; //Los primeros 8 bits que leo son los mas significantes, entonces los corro
    for (i=0;i<3;i++){
        pos+=1;
        aux = MaquinaVirtual->Memoria[pos];
        aux = aux <<(16-i*8); //A Medida que voy leyendo, los bits que leo son menos significantes, entonces los voy corriendo menos
        *valor |= aux;
    }
    MaquinaVirtual->Registros[2] = *valor; //El valor trabajado en la memoria lo copio directo en la MBR
    MaquinaVirtual->Registros[1] = logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[0]); //Paso la LAR para que me calcule la posicion fisica de este y almacenarlo en la MAR
}

void setVM(VM *MaquinaVirtual, int operando, int valor){
    int pos,i;
    MaquinaVirtual->Registros[0] = operando & 0x0000FFFF | 1 << 8; //Tomo el offset del operando y se lo asigno a la LAR
    pos = operando & 0xFFFFFF;
    MaquinaVirtual->Memoria[pos] = (valor >> 24) & 0xFF; //Asigno el byte mas significativo de valor a la primera posicion de memoria ocupada por el valor
    for (i=0;i<3;i++){
        pos +=1;
        MaquinaVirtual->Memoria[pos] = (valor >> (16-8*i)) & 0xFF;
    }
     MaquinaVirtual->Registros[2] = valor; //El valor trabajado en la memoria lo copio directo en la MBR
    MaquinaVirtual->Registros[1] = logica_fisica(*MaquinaVirtual,MaquinaVirtual->Registros[0]); //Paso la LAR para que me calcule la posicion fisica de este y almacenarlo en la MAR
}
