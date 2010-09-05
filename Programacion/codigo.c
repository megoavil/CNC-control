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


/////////////
//*********//
//Variables//
//*********//
/////////////
unsigned char signal;
unsigned char curr_step=STEP_ONE; //Variable que almacena el paso
								  //anterior del motor
/////////////
//*********//
//Funciones//
//*********//
/////////////
unsigned char nextstep_query(unsigned char c_step);
void send_usart(char mensaje_idle[]);
void stepper_move(unsigned char []);
void init(unsigned int);
unsigned char nextstate_query(unsigned char cur_st,unsigned char signal);

///////////////
//***********//
//ESTRUCTURAS//
//***********//
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
//*****************************//
//INICIALIZACION DE ESTRUCTURAS//
//*****************************//
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


//////////
//******//
//Main.c//
//******//
//////////
int main(void){
	unsigned char state=ST_Idle;//se inicializa la variable state al primer estado
	unsigned char buffer[30],c=0;
	unsigned char j=0;
	//unsigned char nextstate;
	//faltan variables de estado, function test

	init(MYUBRR);
		
	DDRB=0x00;

	//Se habilitan las interrupciones
	sei();
	while(1){
		state=nextstate_query(state,signal);
		switch (state){
			case ST_Idle:
				send_usart("m"); //Poner interrupcion que cambio el estado a reading
				break;
			case ST_Reading:
				while(c!='\n'){
					while ( !(UCSRA & (1<<RXC)) );//Leo lo que llega al
												  //USART hasta \n
					c=UDR;
					buffer[j]=c;
					j++;
				}
				signal=SIG_NewLine;//Cambio de estado
				j=0;
				c=0;
				break;
			case ST_Moving:
				stepper_move(buffer);
				signal=SIG_Complete;
				break;
			case ST_Confirm:
				
				break;
		};
	};
}


////////////////////////////
//************************//
//DECLARACION DE FUNCIONES//
//************************//
////////////////////////////

///////////////////////////////////////////
//***************************************//
//FUNCION QUE OBTIENE EL SIGUIENTE ESTADO//
//***************************************//
///////////////////////////////////////////
unsigned char nextstate_query(unsigned char cur_st, unsigned char signal){
	unsigned char next_st = cur_st;
	int i;
	int max=sizeof(smap); //Es la cantidad de mapeos de estado que se tiene	
	for(i=0;i<max;i++){
		if (cur_st==pgm_read_byte(&smap[i].state) && signal==pgm_read_byte(&smap[i].signal)){
			next_st = pgm_read_byte(&smap[i].nextstate);
			break;
		};
	};
	return next_st; //Si no hay ningun cambio devuelve el estado original
}
///////////////////////////////////
//*******************************//
//FUNCION QUE INICIALIZA EL USART//
//*******************************//
///////////////////////////////////
void init(unsigned int ubrr){//9600 baudios 8bits, sin paridad, 1 bit de parada
	
	UBRRH=(unsigned char)(ubrr>>8);
	UBRRL=(unsigned char)ubrr;

	UCSRB=(1<<RXEN)| (1<<TXEN);

	UCSRC=(1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}

//////////////////////////////////
//******************************//
//FUNCION PARA MOVER LOS MOTORES//
//******************************//
//////////////////////////////////
void stepper_move(unsigned char place[]){//Se le pasa lo que se leyo 
										  //desde el puerto serial
	int i=0;
	unsigned char j=0;
	unsigned char c=0;
	unsigned char steps=0;
	//Loop de conversion del string enviado por USART a
	//el valor de pasos que el motor deb dar. Version 1. 
	//La conversion no es correcta,
	//solo es una primera aproximacion debido a que no se tiene
	//en cuenta el tratamiento de bits y de caracteres ASCII.
	for (i=0;;i++){
		c=place[i];
		if (c!='\n'){
			steps=steps*10+c;
		}else break;
	};
	//Una vez que se tiene un valor de pasos que debe dar el motor
	//se entra a un Loop que permite controlar la cantidad de pasos
	//que debe dar el motor de acuerdo a la instruccion
	//enviada por serial.
	for(j=0;(j==steps);j++){
		//Se debe controlar el paso anterior del motor en una variable
		//global:curr_step.
		//Esta variable es variada de acuerdo a la estructura
		//NEXT_STEP declarada al inicio.
		switch(curr_step){
			case STEP_ONE:
				PORTB=0b00001010;
				break;
			case STEP_TWO:
				PORTB=0b00001001;
				break;
			case STEP_THREE:
				PORTB=0b00000101;
				break;
			case STEP_FOUR:
				PORTB=0b00000110;
				break;
		};
		curr_step=nextstep_query(curr_step);//Manejo de pasos para
											//el motor
	};
}

/////////////////////////////////////////////
//*****************************************//
//FUNCION PARA ENVIAR UN CARACTER POR USART//
//*****************************************//
/////////////////////////////////////////////
void send_usart(char data[]){
	int i=0;

	for(i=0;i<sizeof(data);i++){
		while ( !( UCSRA & (1<<UDRE)) );	
		/* Put data into buffer, sends the data */
		UDR = data[i];
	};	
}

//////////////////////////////////////////////////
//**********************************************//
//FUNCION PARA SABER EL SIGUIENTE PASO DEL MOTOR//
//**********************************************//
//////////////////////////////////////////////////
unsigned char nextstep_query(unsigned char c_step){
	unsigned char next_step = c_step;
	int i;
	int max=sizeof(step_map); //Es la cantidad de mapeos de estado que se tiene	
	for(i=0;i<max;i++){
		if (c_step==pgm_read_byte(&step_map[i].step)){
			next_step = pgm_read_byte(&step_map[i].next_step);
			break;
		};
	};
	return next_step;
}


///////////////////////////////////////
//***********************************//
//RUTINAS DE SERVICIO DE INTERRUPCION//
//***********************************//
///////////////////////////////////////

////////////////////////////////////////
//************************************//
//INTERRUPCION POR COMUNICACION SERIAL//
//************************************//
////////////////////////////////////////
ISR(USART_RXC_vect){
	
}

//****************************************
//****************************************
//COMENTARIOS DE VERSION PRELIMINAR*******
//****************************************
//Falta controlar la comunicacion serial con la computadora
//es decir, el envio y recepcion de datos que sera de la sgte manera:
//
//ST_Idle->Se espera interrupcion por comunicacion serial
//		   Cuando llegue un dato por serial, se pasa a la rutina de 
//		   servicio de interrupcion que debe validar un caracter o 
//		   string relacionado con el inicio del envio de pasos del 
//		   motor. 
//
//ST_Receive->Se recibe la cantidad de pasos que debe dar el motor
//			  Cuando la PC termina de enviar el dato debe enviar
//			  "\n" para indicar el fin del envio
//
//ST_Moving->Se procede a mover el motor en una cantidad de pasos
//			 igual a la especificada por la PC.
//			 
//ST_Confirm->Se envia un byte de confirmacion de taladrado a la PC.
//			  Luego, se espera que la PC envie un byte que indique
//			  si se seguira taladrando o si se llego al fin del 
//			  trabajo. Esto indicara si se pasa a ST_Idle o ST_Receive.
//
//Rev2. Version preliminar.
//Por: Mario Egoavil
//
