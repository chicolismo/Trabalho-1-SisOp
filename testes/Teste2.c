/*

	Teste de semaforos mutex e cjoin
	colheita simultaneamente com a plantação.
	venda acontece após a colheita e a plantacao.
	venda e consumo acontecem simultaneamente.

	Foram inseridos inumeros cyield, com prints antes e depois. 
	Confirmando que o semaforo funciona.
	

*/

#include "../include/support.h"
#include "../include/cthread.h"
#include <stdio.h>
#define SEMENTES 12

csem_t Plantacao;
csem_t Mercado;

int plantados = 0;
int colhidos = 0;
int prateleira = 0;
int sementes_usadas = 0;
int a_venda = 0;
int vendidos = 0;

int Fazendeiro;
int Colheita;
int Venda;
int Consumo;

int acaboucolheita = 0;

void* fazendeiro(void *arg){
int pre_yield;
while(sementes_usadas < SEMENTES){
	cwait(&Plantacao);
	plantados++;
	pre_yield = plantados;
	cyield(); //yield para testar os semaforos.
	sementes_usadas++;
	printf("Foi plantado. Ha %d (%d) plantados. (TOTAL PLANTADOS = %d)\n", plantados, pre_yield, sementes_usadas);
	csignal(&Plantacao);
	cyield();	
	}
printf("acabou a plantacao\n");
return NULL;
}

void* colheita(void *arg){
int pre_yield;
while(colhidos < SEMENTES){
	cwait(&Plantacao);
	if(plantados>0){
		plantados--;
		pre_yield = plantados;
		cyield(); //yield para testar os semaforos.
		colhidos++;
		printf("Foi colhido. Ha %d (%d) plantados. (TOTAL COLHIDOS = %d) \n", plantados, pre_yield, colhidos);
		}
	else{
		printf("Nao ha o que colher\n");
		}
	csignal(&Plantacao);
	cyield();	
	}
printf("Fim de colheita. Foram colhidos todos os %d\n", colhidos);
return NULL;
}

void* venda(void *arg){
int pre_yield;
if(cjoin(Colheita)==0){
	acaboucolheita=1;
	while(colhidos > 0){
		cwait(&Mercado);
		colhidos--;
		a_venda++;
		pre_yield = a_venda;
		cyield(); //yield para testar os semaforos.
		printf("Foi enviado 1 ao mercado. Ha %d (%d) na prateleira. (TOTAL ENVIADO = %d) \n", 			a_venda, pre_yield, SEMENTES-colhidos);
		csignal(&Mercado);
		cyield();	
		}
	printf("Nao ha mais o que ser levado a venda\n");
	}
else{
	printf("ERRO: venda não esperou o fim de colheita\n");
	}
	
return NULL;
}
void* consumo(void *arg){
int pre_yield;
while(acaboucolheita==0){
	cyield();
	}
while( vendidos < SEMENTES){
	cwait(&Mercado);
	if(a_venda > 0){
		a_venda--;
		vendidos++;
		pre_yield = a_venda;
		cyield(); //yield para testar os semaforos.
		printf("Foi comprado 1 do mercado. Ha %d (%d) na prateleira. (TOTAL VENDIDOS = %d) \n", a_venda,pre_yield, vendidos);
		}
	else{
		printf("nao ha nada nas prateleiras\n");
		printf("%d\n", vendidos);
		}
	csignal(&Mercado);
	cyield();	
	}
printf("Foram comprados todos os produtos.\n FIM\n");
return NULL;
}


int main()
{
	int i = 42;
	printf("comecou teste: o valor entre parenteses é o valor correto, caso há divergencias há problemas no cyield.\n");
	if((Fazendeiro = ccreate(fazendeiro, (void *)&i)))
		if((Colheita = ccreate(colheita, (void *)&i)))
			if((Venda = ccreate(venda, (void *)&i)))
				if((Consumo = ccreate(consumo, (void *)&i))){
				printf("criou thread\n");
				if(csem_init(&Plantacao, 1) == 0){
					if(csem_init(&Mercado, 1)==0)
					printf("iniciou semaforo\n");
					if(cjoin(Consumo) == 0){
						return 1;
						}
					}
				}
	printf("ERRO DE INICIALIZACAO\n");
	return 1;
}
