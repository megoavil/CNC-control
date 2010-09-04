//Programa de prueba de Maquina de estados
//
//
//Ver adjunto de diagrama de estados
//
//
#include <avr/io.h>		//avr header files
#include <stdint.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>

//Valores
#define FOSC	1843200 //Clock Speed
#define BAUD	9600
#define MYUBRR FOSC/16/BAUD-1
#define STEP_ONE	0b00001010
#define STEP_TWO	0b00001001
#define STEP_THREE	0b00000101
#define	STEP_FOUR	0b00000110
//Estados
#define ST_Idle			1
#define ST_Reading		2
#define	ST_Moving		3
#define ST_Confirm		4

//#define ST_WaitUSB		2
//#define ST_ReadUSB		3
//#define ST_Browser		4
//#define ST_Confirm		5
//#define ST_Processing	6
//#define ST_JobReady		7
//Senales
#define SIG_Request		101
#define	SIG_NewLine		102
#define SIG_Complete	103
#define	SIG_End			104
#define	SIG_Again		105
//#define SIG_GoodPass 	101
//#define SIG_USBFound	102
//#define	SIG_JobDone		103
//#define SIG_TimeOut		104
//Keys
#define KEY_OK 		151
#define KEY_CANCEL	152
//Errores
#define ERROR			200
#define ERR_BadPass 	201 //Password no valido
#define	ERR_USBNotFound	202 //No se encuentra USB
#define	ERR_USBFormat	203 //El formato del USB no es reconocido por el VINCULUM
#define ERR_USB			204 //Error indefinido, relacionado con el stage Browser
#define	ERR_BadFile		205 //Error de sintaxis en archivo EXCELLON
#define	ERR_BadPosition	206 //Error debido a mala ubicacion de la baquelita

//RECORDAR ERROR PARADA DE EMERGENCIA


/////////////////////////////
//Variables
////////////////////////////
unsigned char signal;
unsigned char curr_step=STEP_ONE;

/////////////
//Funciones
/////////////
void send_usart(char mensaje_idle[]);
void stepper_move(unsigned char *);
void init(unsigned int);
unsigned char nextstate_query(unsigned char cur_st,unsigned char signal);

///////////////
//ESTRUCTURAS//
///////////////
//Estructura de funcion de estado
typedef struct PROGMEM{
		unsigned char curr_st;
		char (*p2Func) (unsigned char);
}STATE;
//Estructura de motor de pasos
typedef struct PROGMEM{
	unsigned char step;
	unsigned char next_step;
}NEXT_STEP;
//Estructura de mapeo de estados
typedef struct PROGMEM{
	unsigned char state;
	unsigned char signal;
	unsigned char nextstate;
}NEXT_STATE;

/////////////////////////////////
//INICIALIZACION DE ESTRUCTURAS//
/////////////////////////////////
const NEXT_STEP step_map[] PROGMEM={
	{STEP_ONE,			STEP_TWO},
	{STEP_TWO,			STEP_THREE},
	{STEP_THREE,		STEP_FOUR},
	{STEP_FOUR,			STEP_ONE}
};
const NEXT_STATE smap[] PROGMEM ={

	{ST_Idle,		SIG_Request,	ST_Reading},
	{ST_Reading,	SIG_NewLine,	ST_Moving},
	{ST_Moving,		SIG_Complete,	ST_Confirm},
	{ST_Confirm,	SIG_End,		ST_Idle},
	{ST_Confirm,	SIG_Again,		ST_Reading},

	/*//current_state		//signal			//next_state
	{ST_Idle,			SIG_GoodPass,		ST_WaitUSB},
	{ST_Idle,			ERR_BadPass,		ERROR},

	{ST_WaitUSB,		SIG_USBFound,		ST_ReadUSB},
	{ST_WaitUSB,		ERR_USBNotFound,	ERROR},

	{ST_ReadUSB,		KEY_CANCEL,			ST_Idle},
	{ST_ReadUSB,		KEY_OK,				ST_Browser},
	{ST_ReadUSB,		ERR_USBFormat,		ERROR},

	{ST_Browser,		KEY_OK,				ST_Confirm},
	{ST_Browser,		KEY_CANCEL,			ST_Idle},
	{ST_Browser,		ERR_USB,			ERROR},

	{ST_Confirm,		KEY_OK,				ST_Processing},
	{ST_Confirm,		KEY_CANCEL,			ST_Browser},

	{ST_Processing,		ERR_BadFile,		ERROR},
	{ST_Processing,		ERR_BadPosition,	ERROR},
	{ST_Processing,		SIG_JobDone,		ST_JobReady},

	{ST_JobReady,		SIG_TimeOut,		ST_Idle},		//Se deshabilita el uso de las teclas Cancel, Arrow-Up y Arrow-Down
	{ST_JobReady,		KEY_OK,				ST_Idle}*/
};

//STATE stateFunctions[]={
//	//current_state		//function
//	{ST_Idle,			/*Bienvenido*/},
//	{ST_WaitUSB,		/*IngresaUSB*/},
//	{ST_ReadUSB,		/*USBLeido*/},
//	{ST_Browser,		/*Explorador*/},
//	{ST_Confirm,		/*Confirmar*/},
//	{ST_Processing,		/*Taladrando*/},
//	{ST_JobReady,		/*Listo*/},
//	{ERROR,				/*ErrorHandler*/}
//};


/////////////////////////////////////////
//Main.c/////////////////////////////////
/////////////////////////////////////////
int main(void){
	unsigned char state=ST_Idle;//se inicializa la variable state al primer estado
	unsigned char buffer[],c=0;
	unsigned char j=0;
	//unsigned char nextstate;
	//faltan variables de estado, function test

	init(MYUBRR);
		
	DDRB=0x00;

	//PORTB=0x00;
	
	
	while(1){
		state=nextstate_query(state,signal);
		switch (state){
			case ST_Idle:
				send_usart(1); //Poner interrupcion que cambio el estado a reading
				break;
			case ST_Reading:
				while(c!='\n'){
					while ( !(UCSRA & (1<<RXC)) );//Leo lo que llega al
												  //USART hasta \n
					c=UDR;
					buffer[j]=c;
					j++;
				}
				state=ST_Moving;//Cambio de estado
				j=0;
				c=0;
				break;
			case ST_Moving:
				stepper_move(&buffer);
				break;
			case ST_Confirm:
				break;
		}
}


///////////////////////////////////////////
//OPERACION DE FUNCIONES///////////////////
///////////////////////////////////////////

///////////////////////////////////////////
//FUNCION QUE OBTIENE EL SIGUIENTE ESTADO//
///////////////////////////////////////////
unsigned char nextstate_query(unsigned char cur_st, unsigned char signal){
	unsigned char next_st = cur_st;
	int i;
	int max=3; //Es la cantidad de mapeos de estado que se tiene	
	for(i=0;i<max;i++){
		if (cur_st==pgm_read_byte(&smap[i].state) && signal==pgm_read_byte(&smap[i].signal)){
			next_st = pgm_read_byte(&smap[i].nextstate);
			break;
		}
	}
	return next_st; //Si no hay ningun cambio devuelve el estado original
}
////////////////////////////////////////////
//FUNCION QUE INICIALIZA EL USART///////////
////////////////////////////////////////////
void init(unsigned int ubrr){//9600 baudios 8bits, sin paridad, 1 bit de parada
	
	UBRRH=(unsigned char)(ubrr>>8);
	UBRRL=(unsigned char)ubrr;

	UCSRB=(1<<RXEN)| (1<<TXEN);

	UCSRC=(1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}

//////////////////////////////////
//FUNCION PARA MOVER LOS MOTORES//
//////////////////////////////////
void stepper_move(unsigned char *place[]){//Se le pasa lo que se leyo 
										//desde el puerto serial
	unsigned char i=0;
	unsigned char j=0;
	unsigned char c=0;
	unsigned char steps=0;
	//Aun no se usa lo que se lee del puerto serial
	for (i=0;;i++){
		c=*place[i];
		if (c!='\n'){
			steps=steps*10+c;
		}else break;
	}
	
	for(j=0;j=steps;j++){
		PORTB=curr_step;

	}
}

/////////////////////////////////////////////
//FUNCION PARA ENVIAR UN CARACTER POR USART//
/////////////////////////////////////////////
void send_usart(unsigned int data){
	while ( !( UCSRA & (1<<UDRE)) );
	UCSRB &= ~(1<<TXB8);
	if ( data & 0x0100 )
	UCSRB |= (1<<TXB8);
	/* Put data into buffer, sends the data */
	UDR = data;	
}
