MOV EAX,5
sigue: SUB EAX,1
CMP EAX,0
JZ fin
JMP sigue
fin: STOP