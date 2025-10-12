toString: PUSH BP
MOV BP, SP
PUSH EAX
PUSH ECX
PUSH EDX


MOV EAX, [BP+8] ; Guardo el numero
MOV EBX, [BP+12] ; Puntero a string

CMP EAX, 10
JN finrecu

otro: MOV ECX, EAX
DIV ECX, 10
PUSH EBX
PUSH ECX
CALL toString
ADD SP, 8

DIV EAX, 10
MOV EDX, AC
sigue: ADD EDX, 30
MOV b[EBX], EDX
MOV b[EBX+1], 0
ADD EBX,1
JMP fin

finrecu: CMP EAX, -10
JNP otro
CMP EAX, 0
JN negativo
MOV EDX, EAX
JMP sigue
negativo: MOV b[EBX], 45
ADD EBX, 1
MOV EDX, EAX
NOT EDX
ADD EDX, 1
JMP sigue

fin: POP EDX
POP ECX
POP EAX
MOV SP, BP
POP BP
RET

main: MOV EAX, 1
MOV EDX, DS
LDL ECX, 1
LDH ECX, 4
SYS 1

MOV EBX, DS
ADD EBX, 100

PUSH EBX ; puntero a donde va el string 
PUSH [EDX] ; numero a convertirlo en string
CALL toString
ADD SP, 8
; en EBX esta el puntero al numero pasado a string 

MOV EDX, EBX
SYS 4
RET