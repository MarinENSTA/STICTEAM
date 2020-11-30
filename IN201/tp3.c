#include <stdio.h>
#include <stdlib.h>


#define STACK_SIZE 4096

char stack1[STACK_SIZE];
char stack2[STACK_SIZE];
char stack3[STACK_SIZE];
char stack4[STACK_SIZE];

typedef void * coroutine_t;
coroutine_t coA;
coroutine_t coB;

/* Quitte le contexte courant et charge les registres et la pile de CR. */
void enter_coroutine(coroutine_t cr);

/* Initialise la pile et renvoie une coroutine telle que, 
lorsqu’on entrera dedans,elle commencera à s’exécuter à l’adresse initial_pc.  */

coroutine_t init_coroutine(void *stack_begin, unsigned int stack_size,void (*initial_pc)(void)){
	char *stack_end = ((char *)stack_begin) + stack_size;
	void **ptr = (void **) stack_end;
	ptr--;
	*ptr = initial_pc;
	ptr--;
	*ptr = 0; //rbp
	ptr--;
	*ptr = 0; //rbx
	ptr--;
	*ptr = 0; //rb12
	ptr--;
	*ptr = 0; //rb13
	ptr--;
	*ptr = 0; //rb14
	ptr--;
	*ptr = 0; //rb15 
	return ptr;
}


/* Sauvegarde le contexte courant dans p_from, et entre dans TO. */
void switch_coroutine(coroutine_t *p_from, coroutine_t to);


void boucleA(void){
	int i = 0;
	while(1){
		i++;
		printf("in the coroutine Q4: i = %d\n",i);
		switch_coroutine(&coA, coB);
	}
}

void boucleB(void){
	int j = 0;
	while(1){
		j++;
		printf("in the coroutine Q6 : j = %d\n",j);
		switch_coroutine(&coB,coA);
	}
}

int main(int argc, char const *argv[])
{	
	coA = init_coroutine(stack1, STACK_SIZE, boucleA);
	coB = init_coroutine(stack2, STACK_SIZE, boucleB);
	enter_coroutine(coA);

	return 0;
}
// executé avec gcc -static -Wall tp3.c tp3.s -o tp3 && ./tp3


