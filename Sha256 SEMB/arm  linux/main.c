#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
//Define atributos para legibilidade
#define CHUNK_SIZE 64
#define TOTAL_LEN 8

//Inicializar o array de constantes
//(Primeiros 32bits da parte fracional da raiz cubica dos primeiros 64 primos: 2..311)
static const uint32_t k[] = {
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
	const uint8_t * p;
	size_t len;
	size_t total_len;
	int single_one_delivered; //bool
	int total_len_delivered;  //bool
};

//Inicia a estrutura de dados
static void init_buf_state(struct buffer_state * state, const void * input, size_t len) {
	state->p = input;
	state->len = len;
	state->total_len = len;
	state->single_one_delivered = 0;
	state->total_len_delivered = 0;
}

//Definir o comportamento para 0 < count < 32, qualquer count
static inline uint32_t rotate_right(uint32_t value, unsigned int count) {
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
	if (state->len >= CHUNK_SIZE) {
		memcpy(chunk, state->p, CHUNK_SIZE);
		state->p += CHUNK_SIZE;
		state->len -= CHUNK_SIZE;
		return 1;
	}

	//Copia a parte do CHUNK a ser processada
	memcpy(chunk, state->p, state->len);
	chunk += state->len;
	space_in_chunk = CHUNK_SIZE - state->len;
	state->p += state->len;
	state->len = 0;

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
		size_t len = state->total_len;
		int i;
		memset(chunk, 0x00, left);
		chunk += left;

		//Armazenar len*8 como big endian 64-bit sem overflow
		chunk[7] = (uint8_t) (len << 3);
		len >>= 5;
		for (i = 6; i >= 0; i--) {
			chunk[i] = (uint8_t) len;
			len >>= 8;
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
void calc_sha_256(uint8_t hash[32], const void * input, size_t len) {

	//Inicializar os Valores Hash
	//(Primeiros 32bits da parte fracional da raiz quadrada dos primeiros 8 primos: 2..19)
	uint32_t h[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	int i;
	int j;

	//o bloco de 512-bit em que havera as operacoes
	uint8_t chunk[64];

	struct buffer_state state;

	init_buf_state(&state, input, len);

	while (calc_chunk(chunk, &state)) {
		uint32_t ah[8];
		
		//criar uma mensagem de 64 entradas de palavras de 32 bits
		//copiar o block nas primeiras 16 palavras da mensagem
		uint32_t w[64];
		const uint8_t *p = chunk;

		memset(w, 0x00, sizeof w);
		for (i = 0; i < 16; i++) {
			w[i] = (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 |
				(uint32_t) p[2] << 8 | (uint32_t) p[3];
			p += 4;
		}

		//Extender as primeiras 16 palavras nos 48 espacos remanecentes da mensagem
		for (i = 16; i < 64; i++) {
			const uint32_t s0 = rotate_right(w[i - 15], 7) ^ rotate_right(w[i - 15], 18) ^ (w[i - 15] >> 3);
			const uint32_t s1 = rotate_right(w[i - 2], 17) ^ rotate_right(w[i - 2], 19) ^ (w[i - 2] >> 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}
		
		//Inicializar variaveis com o valor atual do hash
		for (i = 0; i < 8; i++)
			ah[i] = h[i];

		//Loop da compressao principal
		for (i = 0; i < 64; i++) {
			const uint32_t s1 = rotate_right(ah[4], 6) ^ rotate_right(ah[4], 11) ^ rotate_right(ah[4], 25);
			const uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
			const uint32_t temp1 = ah[7] + s1 + ch + k[i] + w[i];
			const uint32_t s0 = rotate_right(ah[0], 2) ^ rotate_right(ah[0], 13) ^ rotate_right(ah[0], 22);
			const uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
			const uint32_t temp2 = s0 + maj;

			ah[7] = ah[6];
			ah[6] = ah[5];
			ah[5] = ah[4];
			ah[4] = ah[3] + temp1;
			ah[3] = ah[2];
			ah[2] = ah[1];
			ah[1] = ah[0];
			ah[0] = temp1 + temp2;
		}

		//Adicionar o bloco comprimido ao valor hash
		for (i = 0; i < 8; i++)
			h[i] += ah[i];
	}

		//Produz o hash final em big endian
	for (i = 0, j = 0; i < 8; i++) {
		hash[j++] = (uint8_t) (h[i] >> 24);
		hash[j++] = (uint8_t) (h[i] >> 16);
		hash[j++] = (uint8_t) (h[i] >> 8);
		hash[j++] = (uint8_t) h[i];
	}
}


//Transformar a hash em uma string
static void hash_to_string(char string[65], const uint8_t hash[32]) {
	
	size_t i;
	for (i = 0; i < 32; i++) {
		string += sprintf(string, "%02x", hash[i]);
	}
} 





int main() {
	//um auxiliar
	int i;
	//Um caractere
	int ch;

	//Tamanho maximo de caracteres do arquivo
	char string[3001]={'\0'};

	//Ponteiro para a string
	char *p_string;

	//String que sera equivalente ao hash
	char hash_string[65]={'\0'};

	//O blob(bloco de dados) do hash
	uint8_t hash[32];

	FILE *inputFile;
	FILE *outputFile;

	p_string = string;

	//carregar input do arquivo inputFile
	inputFile = fopen("inputFile.txt", "rb");

	if(inputFile == NULL){
		printf("Arquivo nao encontrado.");
		return 1;
	}
	i = 0;

	//Para levar em conta os \n na string de entrada, usamos esse while para ler ate o final do arquivo e salvar cada caractere na variavel
	while (i < sizeof(string)-1 && EOF !=(ch=fgetc(inputFile))){
		string[i++] = ch;
	}
	printf("inputFile:\n%s \n", string);

	fclose(inputFile);

	//Obter o tamanho da string de input
	int j = 0;
	for (i = 0; i <= 3000; i++){
		if(string[i] != '\0'){
			j++;
		}
	}

	//variaveis de tempo
	clock_t init_count,end_count;
    	double period;
	//Inicia contagem de tempo
	init_count = clock();

	//Funcao principal
	calc_she_256(hash, p_string, j-2);

	//Funcao de tornar um blob hash em uma string
	hash_to_string(hash_string, hash);

	//Finaliza contagem e imprime tempo
	end_count = clock();
    	period = ( (end_count - init_count) / (CLOCKS_PER_SEC/1000.0) );
    	printf("\ntempo coletado = %f\n", period);

	//escrever hash_string no arquivo outputFile
	outputFile = fopen("outputFile.txt", "wb");
	int result = fputs(hash_string, outputFile);
	if(result == EOF){
		printf("Nenhum caractere encontrado a ser salvo");
		return 1;
	}

	printf("outputFile:\n%s \n",hash_string);

	fclose(outputFile);

	return 0;
}
