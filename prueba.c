//Programa de prueba de Maquina de estados
//Controlada por Switches
//
//3 Pulsadores
//8 leds
//
#include <avrio.h>		//avr header files

#define ST_UNO=1
#define ST_DOS=2
#define ST_TRES=3

struct NEXT_STATE{
	unsigned char curr_ST;
	unsigned char signal;
	unsigned char next_ST;
}state_mapping[]={
	{ST_UNO,	1,		ST_DOS},
	{ST_DOS,	2,		ST_TRES},
	{ST_TRES,	4,		ST_UNO}
};

int analize_signal(unsigned char *,struct NEXT_STATE *);

int main(void){
	unsigned char state=ST_UNO;//se inicializa la variable state al primer estado

	DDRA=0x00;	
	DDRB=0xFF;

	PORTB=0x00;

	while(1){
		analize_signal(&state);
		switch(state){
			case ST_UNO:
				PORTB=0x01;
				break;
			case ST_DOS:
				PORTB=0x02;
				break;
			case ST_TRES:
				PORTB=0x03;
				break;
		}
	}
}

unsigned char analize_signal(unsigned char *state){
	unsigned char temp;
	struct NEXT_STATE *p;
	temp=PINA;
	
	for(p=state_mapping;p<state_mapping+3;p++){
		if ((*state==p->curr_ST))&&((temp==p->signal))
			return p->next_ST;
	}
	
	return *state;
	
}
