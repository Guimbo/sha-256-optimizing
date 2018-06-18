/*******************************************************************************
 * MARK - Cabeçalho ************************************************************
 *******************************************************************************
    Generated Main Source File

    Company:
        Microchip Technology Inc.

    File Name:
        main.c

    Summary:
        Este é o arquivo principal gerado usando o PIC18 MCU.
    
    Plataforma Alvo: PIC18F4550

    Data: 
        25 de maio de 2018

    Contexto de desenvolvimento:
        O código foi desenvolvido na disciplina de sistemas embarcados
        no curso de engenharia de computação - IFCE. Este arquivo tem como Fim
        ser executado no microcontroller PIC18F4550

    Restrição:  
        O código foi desenvolvido para PIC18F4550 e não é garantido
        que ESTA VERSÃO da implementação funcione em outros dispositivos.
    
    Copyright: 
        No copyright.

    Modo de uso:    
        Para o uso desde arquivo é necessário ter ou um simulador de PIC18F4550
        Ou então o próprio pic18f4550. Em seguida basta executar o código em MPLABX o compilador XC8.
        Assim basta gerar o arquivo .hex(na pasta onde este código se encontra já pode ter o arquivo)
        e gravar o código no pic18f4550. Através de alguma interface serial é possível ver a execução.
        No caso de um simulador, o arquivo .coff pode ser usado para debug.
    
    Entradas:
        Sem entradas para a versão para pic do algoritmo.
    Saída:
        O hash é mostrado pela porta serial.

    autores/estudantes: 
        Daniel Silva Alves Barbosa & Guilherme Araujo da Silva


    Description:
        Implementação do algoritmo hash SHA-256.
        SHA-256 é um dos três algoritmos do SHA2
        especificação. Os outros, SHA-384 e SHA-512, não são
        oferecido nesta implementação.
        Especificação de algoritmo pode ser encontrada aqui:
            * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf *


    Referências:
    Sha-256: 
        Brad Conte (brad AT bradconte.com) / github: https://github.com/B-Con/crypto-algorithms
    USART:
	http://www.electronicwings.com/users/lokeshc/codes/mpu6050_interfacing_with_pic18f4550/USART_Source_File.c
*/

/*******************************************************************************
 * MARK - Imports  *************************************************************
 ******************************************************************************/

#include <p18f4550.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



//Define Frequencia
#define F_CPU 8000000/64
#define Baud_value (((float)(F_CPU)/(float)baud_rate)-1)
//Define atributos para legibilidade
#define CHUNK_SIZE 64
#define TOTAL_LEN 8


//Configuracoes do Pic
#pragma config FOSC     = HS    /// EXTERN CLOCK 8MHZ
#pragma config IESO     = OFF   /// INTERNAL/EXTERNAL OSCILATOR DISABLE
#pragma config PWRT     = OFF   /// DISABLE POWER-UP TIMER
#pragma config BORV     = 3     /// BROWN-OUT RESET MINIMUM
#pragma config WDT      = OFF   /// DISABLE WATCHDOG TIMER
#pragma config WDTPS    = 32768 /// WATCHDOG TIMER 32768s
#pragma config MCLRE    = OFF   /// MASTER CLEAR PIN (RE3) DISBALE
#pragma config LPT1OSC  = OFF   /// TIMER1 LOW POWER OPERATION
#pragma config PBADEN   = OFF   /// PORTB.RB0,1,2,3,4 AS I/O DIGITAL
#pragma config STVREN   = ON    /// STACK FULL/UNDERFLOW CAUSE RESET
#pragma config LVP      = OFF   /// DISABLE LOW VOLTAGE PROGRAM (ICSP DISABLE)





/*******************************************************************************
 * MARK - USART ****************************************************************
 ******************************************************************************/
static const char string[] = {
	'a','b','c','d','a','b','c','d','a','b','c','d','a','b','c','d'
};





/*******************************************************************************
 * MARK - USART ****************************************************************
 ******************************************************************************/
void USART_Init(long);
void USART_TxChar(char);
void USART_SendString(const char *);

//Funcao para iniciar a configuracao para a comunicacao
//Entrada: um long -> a frequencia de baud rate
//Saida: nada
void USART_Init(long baud_rate)
{
    float temp;
    TRISC6=0;                       /*Make Tx pin as output*/
    TRISC7=1;                       /*Make Rx pin as input*/
    temp=Baud_value;     
    SPBRG=(int)temp;                /*baud rate=9600, SPBRG = (F_CPU /(64*9600))-1*/
    TXSTA=0x20;                     /*Transmit Enable(TX) enable*/ 
    RCSTA=0x80;                     /*Receive Enable(RX) enable and serial port enable */
}

//Funcao de mandar um char pelo Tx usando usart
//Entrada: um char -> char a ser transmitido
//Saida: nada
void USART_TxChar(char out)
{        
        while(TXIF==0);            /*wait for transmit interrupt flag*/
        TXREG=out;                 /*wait for transmit interrupt flag to set which indicates TXREG is ready
                                    for another transmission*/    
}

//Funcao de mandar uma string pelo Tx usando usart
//Entrada: um ponteiro para um array de char -> array a ser transmitido
//Saida: nada
void USART_SendString(const char *out)
{
   while(*out!='\0')
   {            
        USART_TxChar(*out);
        out++;
   }
}



/*******************************************************************************
 * MARK - SHA256 ***************************************************************
 ******************************************************************************/

//Inicializar o array de constantes
//(Primeiros 32bits da parte fracional da raiz cubica dos primeiros 64 primos: 2..311)
static const uint32_t primes[] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

//Uma estrutura de dados para ficar checando o quanto de bloco de dados ainda falta a ser processada
struct buffer_state {
	const uint8_t * pointer;
	size_t length;
	size_t total_len;
	int single_one_delivered; //bool
	int total_len_delivered;  //bool
};

//Inicia a estrutura de dados
static void init_buf_state(struct buffer_state * state, const void * input, size_t length) {
	state->pointer = input;
	state->length = length;
	state->total_len = length;
	state->single_one_delivered = 0;
	state->total_len_delivered = 0;
}

//Definir o comportamento para 0 < count < 32, qualquer count
static uint32_t rotate_right(uint32_t value, unsigned int count) {
	return value >> count | value << (32 - count);
}

//Calcula o pegaco de dados a ser processado e atualiza o buffer
static int calc_chunk(uint8_t chunk[CHUNK_SIZE], struct buffer_state * state) {
	size_t space_in_chunk;

	//Se nao houver mais dados sobrando, retorna 0 para terminar o while
	if (state->total_len_delivered) {
		return 0;
	}

	//Atualiza os atributos do CHUNK, salvando assim, quais partes do CHUNK ja foi processada e a que ainda falta
	if (state->length >= CHUNK_SIZE) {
		memcpy(chunk, state->pointer, CHUNK_SIZE);
		state->pointer += CHUNK_SIZE;
		state->length -= CHUNK_SIZE;
		return 1;
	}

	//Copia a parte do CHUNK a ser processada
	memcpy(chunk, state->pointer, state->length);
	chunk += state->length;
	space_in_chunk = CHUNK_SIZE - state->length;
	state->pointer += state->length;
	state->length = 0;

	//se a execucao chegou ate aqui, o space_in_chunk e pelo menos 1
	if (!state->single_one_delivered) {
		*chunk++ = 0x80;
		space_in_chunk -= 1;
		state->single_one_delivered = 1;
	}

	//Entao
	//Ha espaco suficiente para o tamalho total e o programa poderar concluir
	//Ou ha muito pouco espaco remanecente e sera necessario encher o resto do block com `0`s
	//
	//Este if fara essa segunda ocasiao
	if (space_in_chunk >= TOTAL_LEN) {
		const size_t left = space_in_chunk - TOTAL_LEN;
		size_t length = state->total_len;
		int i;
		memset(chunk, 0x00, left);
		chunk += left;

		//Armazenar length*8 como big endian 64-bit sem overflow
		chunk[7] = (uint8_t) (length << 3);
		length >>= 5;
		for (i = 6; i >= 0; i--) {
			chunk[i] = (uint8_t) length;
			length >>= 8;
		}
		state->total_len_delivered = 1;
	} else {
		//Preenche o espaco remanecente com '0's
		memset(chunk, 0x00, space_in_chunk);
	}

	return 1;
}

//Limitacoes
//- input tem que ser carregado completamente na RAM
//- o algoritmo SHA teoricamente opera em bits da string. Porem, esta implementacao nao suporta strings com caracteres 
//maiores que 8bits.
void calc_sha_256(uint8_t hash[32], const char * input, size_t length) {

	//Inicializar os Valores Hash
	//(Primeiros 32bits da parte fracional da raiz quadrada dos primeiros 8 primos: 2..19)
	uint32_t h[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	int i;
	int j;

	//o bloco de 512-bit em que havera as operacoes
	uint8_t chunk[64];

	struct buffer_state state;

	init_buf_state(&state, input, length);

	while (calc_chunk(chunk, &state)) {
		uint32_t aux_hash[8];
		
		//criar uma mensagem de 64 entradas de palavras de 32 bits
		//copiar o block nas primeiras 16 palavras da mensagem
		uint32_t word[64];
		const uint8_t *pointer = chunk;

		memset(word, 0x00, sizeof word);
        i=0;
        while(i < 16){
			word[i] = (uint32_t) pointer[0] << 24 | (uint32_t) pointer[1] << 16 |
				(uint32_t) pointer[2] << 8 | (uint32_t) pointer[3];
			pointer += 4;
            i++;
		}

		//Extender as primeiras 16 palavras nos 48 espacos remanecentes da mensagem
		i = 16; 
        while(i < 64){
			const uint32_t s0 = rotate_right(word[i - 15], 7) ^ rotate_right(word[i - 15], 18) ^ (word[i - 15] >> 3);
			const uint32_t s1 = rotate_right(word[i - 2], 17) ^ rotate_right(word[i - 2], 19) ^ (word[i - 2] >> 10);
			word[i] = word[i - 16] + s0 + word[i - 7] + s1;
             i++;
		}
		
		//Inicializar variaveis com o valor atual do hash
        i = 0;
		while(i < 8){
			aux_hash[i] = h[i];
            i++;
        }

		//Loop da compressao principal
		i = 0; 
        while(i < 64) {
			const uint32_t s1 = rotate_right(aux_hash[4], 6) ^ rotate_right(aux_hash[4], 11) ^ rotate_right(aux_hash[4], 25);
			const uint32_t ch = (aux_hash[4] & aux_hash[5]) ^ (~aux_hash[4] & aux_hash[6]);
			const uint32_t temp1 = aux_hash[7] + s1 + ch + primes[i] + word[i];
			const uint32_t s0 = rotate_right(aux_hash[0], 2) ^ rotate_right(aux_hash[0], 13) ^ rotate_right(aux_hash[0], 22);
			const uint32_t maj = (aux_hash[0] & aux_hash[1]) ^ (aux_hash[0] & aux_hash[2]) ^ (aux_hash[1] & aux_hash[2]);
			const uint32_t temp2 = s0 + maj;

			aux_hash[7] = aux_hash[6];
			aux_hash[6] = aux_hash[5];
			aux_hash[5] = aux_hash[4];
			aux_hash[4] = aux_hash[3] + temp1;
			aux_hash[3] = aux_hash[2];
			aux_hash[2] = aux_hash[1];
			aux_hash[1] = aux_hash[0];
			aux_hash[0] = temp1 + temp2;
            i++;
		}

		//Adicionar o bloco comprimido ao valor hash
		i = 0;
        while(i < 8){
			h[i] += aux_hash[i];
            i++;
        }
	}

		//Produz o hash final em big endian
	i = 0, j = 0;
    while(i < 8){
		hash[j++] = (uint8_t) (h[i] >> 24);
		hash[j++] = (uint8_t) (h[i] >> 16);
		hash[j++] = (uint8_t) (h[i] >> 8);
		hash[j++] = (uint8_t) h[i];
        i++;
	}
}

//Transformar a hash em uma string
static void hash_to_string(char string[65], const uint8_t hash[32]) {
	
	size_t i = 0;
	while (i < 32) {
		string += sprintf(string, "%02x", hash[i]);
        i++;
	}
} 





/*******************************************************************************
 * MARK - MAIN *****************************************************************
 ******************************************************************************/

void main() {
    
    //String que sera equivalente ao hash
    char hash_string[65]={'\0'};
    
    //O blob(bloco de dados) do hash
    uint8_t hash[32];
    
    //Ponteiro para a string
    char *pronteiro_string;
    pronteiro_string = string;
    
    USART_Init(9600);
    
    while(1){

        //Funcao principal
        calc_sha_256(hash, pronteiro_string, 16);

        //Funcao de tornar um blob hash em uma string
        hash_to_string(hash_string, hash);
		
		
        USART_SendString("Hash String: ");
        USART_SendString(hash_string);
        USART_SendString("          ");
    }
}
