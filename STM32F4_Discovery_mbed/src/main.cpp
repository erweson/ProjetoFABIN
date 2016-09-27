


// == Includes ==
#include "mbed.h"
#include "DigitalOut.h"
#include "DigitalIn.h"
#include "InterruptIn.h"
#include "Serial.h"


// == Vari�veis globais ==
DigitalOut led3(LED3);
DigitalOut led4(LED4);
DigitalOut led5(LED5);
DigitalOut led6(LED6);

DigitalIn botao(USER_BUTTON);


/**
 * main
 */
int main(void)
{

	int count = 0;
	// == Aqui ficam as inicializa��es [semelhante � fun��o setup() do Arduino] ===





	// == Aqui � o loop infinito [semelhante � fun��o loop() do Arduino] ===
	while (1)
	{

		if(botao == 1 && count % 4 == 0){
			led3 = !led3;
			count++;
			while(botao == 1);
		}

		if(botao == 1 && count % 4 == 1){
			led4 = !led4;
			count++;
			while(botao == 1);
		}

		if(botao == 1 && count % 4 == 2){
			led5 = !led5;
			count++;
			while(botao == 1);
		}

		if(botao == 1 && count % 4 == 3){
			led6 = !led6;
			count++;
			while(botao == 1);
		}


	}
}
