/*main.c 
=========
IMPORTANTE: Por ahora faltan los includes

*/

#include <stdio.h>
#include <stdlib.h>

/*Definiciones globales:
========================
A continuacion se definen las 
	-señales (SIG_*)
	-teclas (KEY_*)
	-estados (ST_*)
	-errores (ERR_*)
de nuestra maquina de estados

NOTA: Por claridad y consistencia se ha numerado cada grupo a partir de cierto valor.*/

//Estados posibles (A partir del 1)
#define ST_Idle			1
#define ST_WaitUSB		2
#define ST_ReadUSB		3
#define ST_Browser		4
#define ST_Confirm		5
#define ST_Processing	6
#define ST_JOBRDY		7

//Señales (A partir del 101)
#define SIG_GoodPass 	101
#define SIG_USBFound	102
#define	SIG_JobDone		103


//Teclas (A partir del 151)
//FALTAN MAS (se puede hasta 8)
#define KEY_OK 		151
#define KEY_CANCEL	152


//Errores (A partir del 201)
//Password no valido
#define ERR_Badpass 	201
//No se encuentra USB
#define	ERR_USBNotFound	202
//El formato del USB no es reconocido por el VINCULUM
#define	ERR_USBFormat	203
//Error indefinido, relacionado con el stage Browser
#define ERR_USB			204
//Error de sintaxis en archivo EXCELLON
#define	ERR_BadFile		205
//Error debido a mala ubicacion de la baquelita
#define	ERR_BadPosition	206


/*Estructuras usadas para manejar los estados:
=============================================
A continuacion se definen las estructuras que guardaran los estados, sus funciones asociadas
y las senales o teclas que cambiaran al siguiente estados
	-La estructura STATE contiene la lista de estados y un puntero a su funcion asociada
	-La estructura NEXT_STATE contiene la lista de estados, y los posibles siguientes estados 
	segun la senal recibida*/
typedef struct{
	unsigned char current_state;
	char (*p2Func)(unsigned char);//MODIFICAR SI SE USARAN MAS VARIABLES
}STATE;

typedef struct{
	unsigned char current_state;
	unsigned char signal;
	unsigned char next_state;
}NEXT_STATE;

NEXT_STATE stateMapping[] = {
//current_state		//signal			//next_state
{ST_Idle,			SIG_GoodPass,		ST_WaitUSB},
{ST_Idle,			ERR_BadPass,		ERROR},

{ST_WaitUSB,		SIG_UsbFound,		ST_ReadUSB},
{ST_WaitUSB,		ERR_UsbNotFound,	ERROR},

{ST_ReadUSB,		KEY_Cancel,			ST_Idle},
{ST_ReadUSB,		KEY_OK,				ST_Browser},
{ST_ReadUSB,		ERR_USBFormat,		ERROR},

{ST_Browser,		KEY_OK,				ST_Confirm},
{ST_Browser,		KEY_Cancel,			ST_Idle},
{ST_Browser,		ERR_USB,			ERROR},

{ST_Confirm,		KEY_OK,				ST_Processing},
{ST_Confirm,		KEY_Cancel,			ST_Browser},

{ST_Processing,		ERR_BadFile,		ERROR},
{ST_Processing,		ERR_BadPosition,	ERROR},
{ST_Processing,		SIG_JobDone,		ST_JobRdy},

{ST_JobRdy,			SIG_TimeOut,		ST_Idle},		//Se deshabilita el uso de las teclas Cancel, Arrow-Up y Arrow-Down
{ST_JobRdy,			KEY_Enter,			ST_Idle}
};

/*
Aca se crea el arreglo con cada estado y su funcion correspondiente

Una vez se hallan creado las funciones asociadas a cada estado descomentar
esta seccion y agregar la funcion para cada caso
===========================================================================
STATE stateFunctions[] = {
//current_state		//function
{ST_Idle,		Bienvenido()},
{ST_ReadPass,		FALTA},
{ST_WaitUSB,		FALTA},
{ST_ReadUSB,		FALTA},
{ST_Browser,		FALTA},
{ST_Confirm,		FALTA},
{ST_Processing,		FALTA},
{ST_JobRdy,		FALTA},
{ST_Error,		FALTA}
};
*/

void main(void){
	//Variables que mantendran las senales y entradas recibidas
	unsigned char inputKey;
	unsigned char signal;
	unsigned char error;

	
	while(1){
		signal=getSignals();
		if (keysEnabled){
			inputKey=getKey();
		}
	}
}