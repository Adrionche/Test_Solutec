#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h> /* ne compile pas avec -std=c89 ou -std=c99 */

#ifndef N
#define N 10
#endif

ucontext_t tikping, takpong, exit_context;

void finalize(){}

void tak(int);
void tik(int numero)
{
  /* Break condition */
  if (numero < 0){
    setcontext(&exit_context);
  }
  makecontext(&takpong, (void (*)(void)) tak, 1, numero);
  printf("ping %d\n", numero);
  setcontext(&takpong);
}

void tak(int numero){
  makecontext(&tikping, (void (*)(void)) tik, 1, numero - 1);
  printf("pong %d\n", numero);
  setcontext(&tikping);
}

int main() {

  getcontext(&tikping); /* initialisation de uc avec valeurs coherentes
		    * (pour éviter de tout remplir a la main ci-dessous) */
  getcontext(&takpong);
  getcontext(&exit_context);
  
  tikping.uc_stack.ss_size = 64*1024;
  tikping.uc_stack.ss_sp = malloc(tikping.uc_stack.ss_size);
  tikping.uc_link = &takpong;
  makecontext(&tikping, (void (*)(void)) tik, 1, N);

  takpong.uc_stack.ss_size = 64*1024;
  takpong.uc_stack.ss_sp = malloc(takpong.uc_stack.ss_size);
  takpong.uc_link = &tikping;
  makecontext(&takpong, (void (*)(void)) tak, 1, N);

  exit_context.uc_stack.ss_size = 64*1024;
  exit_context.uc_stack.ss_sp = malloc(exit_context.uc_stack.ss_size);
  exit_context.uc_link = &tikping;
  makecontext(&exit_context, (void (*)(void)) finalize, 0);

  setcontext(&tikping);
  /* Si on avait autre chose que NULL dans tikping.uc_link,
  le programme serait revenu à la dernière sauvegarde de contexte à : 
  swapcontext(&takpong, &tikping);
  Donc donne une boucle infinie */
  printf("je ne reviens jamais ici\n");
  return 0;
}

/*
On peut faire deux fonctions qui se passent le contexte entre elles, sans jamais revenir dans le main
*/