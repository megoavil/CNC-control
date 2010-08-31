#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_Format 1
#define ERR_File 2
#define ERR_BadChar 3

//Termina el programa e imprime un error segun el codigo
void errorHandler(unsigned char code){
	switch(code){
		case ERR_Format:
			printf("ERROR: El archivo no tiene un formato correcto.\n");
			break;
		case ERR_File:
			printf("ERROR: Hubo un problema leyendo el archivo.\n");
			break;
		case ERR_BadChar:
			printf("ERROR: Caracter no reconocido en el archivo Excelon.\n");
			break;
		default:
			printf("ERROR: Error no reconocido.\n");
			break;
	}
	exit(1);
}

//Convierte el string que contiene las coordenadas a una arreglo de enteros
void parseCoordinates(char *string, int *coords){
 	coords[1] = atoi(strchr (string, 'Y')+1);
	coords[0] = atoi(strtok (string, "Y")+1);
}

//Lee un archivo Excellon (.drd) que se le pase como argumento en consola
//	C:\parser archivo.drd
//
int main(int argc, char **argv)
{
	char *filename = argv[argc-1];
	FILE *file;
	char buffer[16];//Ver tamano adecuado

	unsigned char start=0;//Se usa para ver si ya se empezaron a leer las coordenadas o no
	unsigned char drillTotal=0;//Cuenta la cantidad de taladros en el header
	unsigned char drillCurrent=0;//Va contando cada vez que se cambia de taladro
	//unsigned char controlCount=0;//Cuenta los caracteres de control %
	int coordinates[2];

	char command;

	file = fopen(filename,"r");
	if (!file){
		errorHandler(ERR_File);
	}
	//Mientras no se haya llegado al final del archivo
	while(strcmp(buffer,"M30\n")){
		fgets(buffer, sizeof(buffer), file);
		command = *buffer; //Primera letra para reconocer el tipo de comando
		
		//Cabecera de archivo Excellon, start=0
		if (start==0){
			switch(command){
				case('T'):
					drillTotal++;
					break;
				case('G'):
					printf(buffer);	//Los comandos de tipo GXX de la cabecera deben ser identificados
					break;
				case('M'):
					if(!strcmp(buffer,"M72\n")){
						printf("Usando pulgadas...\n");
					}else if(!strcmp(buffer,"M71\n")){
						printf("Usando milimetros...\n");
					}
					break;
				case('%'):
					printf("Se usaran %x taladros.\n",drillTotal);
					printf("Iniciando modo taladro...\n");
					start=1;		//*******************************Se inicia la lectura de coordenadas
					break;
				default:
					errorHandler(ERR_Format)
					break;
			}
		}
		
		//Zona de coordenadas, solo cuando start=1
		if (start==1){
			switch(command){
				case('T'): //Seleccion de herramienta
					if(strcmp(buffer,"T00\n")){ 			//Se valida que no sea footer del archivo excellon
						drillCurrent++;
						printf("Cambiando a taladro %x...\n",drillCurrent);
					}
					break;
				case('X')://Coordenadas
					parseCoordinates(buffer,coordinates);
					printf("Moviendo taladro a coordenadas (%d,%d)...\n",*coordinates,*(coordinates+1));
					break;
				default://Error de formato
					errorHandler(ERR_Format);
					break;
			}
		}
		
	}
	printf("Ejecucion finalizada de forma exitosa. Buena chino!\n");
	fclose(file);
	return 0;
}