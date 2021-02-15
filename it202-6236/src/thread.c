#ifndef THREAD_C
#define THREAD_C
#include "thread.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <valgrind/valgrind.h>

/* Queue Docuentation : http://man7.org/linux/man-pages/man3/queue.3.html */

#ifndef ARCHI
#define ARCHI 64 /* 64 or 32 depending on your architecture */
#endif
#ifndef STACK_SIZE
#define STACK_SIZE 1024
#endif

/* Set of colors for printf */
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

/* thread_struct macros */
#define STATUS_TERMINATED 0
#define STATUS_RUNNING 1
#define STATUS_WAITING 2
#define STATUS_JOINING 3

/* ============================
          PRIVATE
============================  */



/* Queues containing all the threads */
STAILQ_HEAD(thread_queue, thread_struct);

struct thread_queue fifo_table[10];

thread_t current_thread;

/* Queue containing all the dead threads */
STAILQ_HEAD(dead_queue, thread_struct) dead = STAILQ_HEAD_INITIALIZER(dead);

/* Queue containing all the joining threads */
STAILQ_HEAD(joining_queue, thread_struct) joining = STAILQ_HEAD_INITIALIZER(joining);


/* Debug function for errors */
void exit_error(const char*, const char*)  __attribute__ ((__noreturn__));;
void exit_error(const char* error_message, const char *function){
    printf(RED"ERROR : %s in %s\n"RESET, error_message, function);
    exit(EXIT_FAILURE);
}

void init_queue(void) __attribute__ ((constructor));

void free_queue(void) __attribute__ ((destructor));

void init_queue() {
    int i = 0;
    while (i < 10) {
        STAILQ_INIT(&fifo_table[i]);
        i++;
    }
    STAILQ_INIT(&dead);
    STAILQ_INIT(&joining);
    
    ucontext_t uc;
    getcontext(&uc);
    uc.uc_stack.ss_size = ARCHI*STACK_SIZE;
    uc.uc_stack.ss_sp = malloc(uc.uc_stack.ss_size);
    uc.uc_link = NULL;

    int valgrind_stackid = VALGRIND_STACK_REGISTER(uc.uc_stack.ss_sp,
                                                uc.uc_stack.ss_sp + uc.uc_stack.ss_size);

    thread_t th = malloc(sizeof(struct thread_struct));
    th->context = uc;
    th->status = STATUS_RUNNING;
    th->val = NULL;
    th->stack_id = valgrind_stackid;
    th->priority = 0;
    th->father   = NULL;

    current_thread = th;

    STAILQ_INSERT_HEAD(&fifo_table[th->priority], th, entries);
}

void free_queue() {
    thread_t h;
    int i = 0;
    while (i < 10) {
        while (!STAILQ_EMPTY(&fifo_table[i])) {
            h = STAILQ_FIRST(&fifo_table[i]);
            STAILQ_REMOVE_HEAD(&fifo_table[i], entries);
            VALGRIND_STACK_DEREGISTER(h->stack_id);
            free(h->context.uc_stack.ss_sp);
            free(h);
        }
        i++;
    }
    while (!STAILQ_EMPTY(&dead)) {
        h = STAILQ_FIRST(&dead);
        STAILQ_REMOVE_HEAD(&dead, entries);
        VALGRIND_STACK_DEREGISTER(h->stack_id);
        free(h->context.uc_stack.ss_sp);
        free(h);
    }
    while(!STAILQ_EMPTY(&joining)) {
        h = STAILQ_FIRST(&joining);
        STAILQ_REMOVE_HEAD(&joining, entries);
        VALGRIND_STACK_DEREGISTER(h->stack_id);
        free(h->context.uc_stack.ss_sp);
        free(h);
    }
}

/* Atomic function to change value of an integer */
int compare_and_swap(int *ptr, int oldval, int newval){
    /* atomic */
    int currentval = *ptr;
    if (currentval == oldval){
      *ptr = newval;
      return 1;
    }
    /* end atomic */
    return 0;
}

/* ============================
          PUBLIC
============================  */


/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(){
  return current_thread;
}

int max_fifo() {
    int i = 9;
    while (i > -1) {
        if (!STAILQ_EMPTY(&fifo_table[i])) {
            return i;
        }
        i --;
    }
    return -1;
}

void set_priority(thread_t thread, int new_priority) {
    if (new_priority != thread->priority) {
        STAILQ_REMOVE(&fifo_table[thread->priority], thread, thread_struct, entries);
        STAILQ_INSERT_TAIL(&fifo_table[new_priority], thread, entries);
        thread->priority = new_priority;
    }
}

void thread_manual_exit() {
    void *res = current_thread->func(current_thread->funcarg);
    if (current_thread->status != STATUS_TERMINATED) {
        STAILQ_REMOVE_HEAD(&fifo_table[current_thread->priority], entries);
        current_thread->status = STATUS_TERMINATED;
        current_thread->val = res;
        STAILQ_INSERT_TAIL(&dead, current_thread, entries);
        if (current_thread->father != NULL && current_thread->father->status == STATUS_JOINING){
        STAILQ_REMOVE(&joining, current_thread->father, thread_struct, entries);
        STAILQ_INSERT_TAIL(&fifo_table[current_thread->father->priority], current_thread->father, entries);
        current_thread->father->status = STATUS_WAITING;
        }
        int max = max_fifo();
        if (max > -1) {
            thread_t new = STAILQ_FIRST(&fifo_table[max]);
            new->status = STATUS_RUNNING;
            thread_t old = current_thread;
            current_thread = new;
            swapcontext(&old->context, &new->context); // à remplacer par setcontext
        }
    }
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    if (newthread == NULL) {
        exit_error("NULL pointer given for newethread", __func__);
    }
    if (func == NULL){
        exit_error("NULL pointer given for func", __func__);
    }

    /* Allocating memory */
    *newthread = malloc(sizeof(struct thread_struct));

    /* Thread initialization */
    ucontext_t uc;
    getcontext(&uc);
    uc.uc_stack.ss_size = ARCHI*STACK_SIZE;
    uc.uc_stack.ss_sp = malloc(uc.uc_stack.ss_size);
    uc.uc_link = NULL;
    /* juste après l'allocation de la pile */
    int valgrind_stackid = VALGRIND_STACK_REGISTER(uc.uc_stack.ss_sp,
                                                uc.uc_stack.ss_sp + uc.uc_stack.ss_size);
    /* stocker valgrind_stackid dans votre structure thread */
    (*newthread)->context  = uc;
    (*newthread)->status   = STATUS_WAITING;
    (*newthread)->val      = NULL;
    (*newthread)->stack_id = valgrind_stackid;
    (*newthread)->func     = func;
    (*newthread)->funcarg  = funcarg;
    (*newthread)->priority = 5;
    (*newthread)->father   = thread_self();

    /* Adding thread into the ordonancer */
    STAILQ_INSERT_TAIL(&fifo_table[(*newthread)->priority], *newthread, entries);

    makecontext(&(*newthread)->context, (void (*)(void)) thread_manual_exit, 0);

    return EXIT_SUCCESS;
}
/*
void print_head(){
  printf("printing the head queue : \n\n");
  thread_t th = STAILQ_FIRST(&head);
  while(th != NULL){
    printf("thread : %p, context : %p, context.stack_size : %lu, context.stack_pointer : %p, context.uc_link : %p\n", th, &th->context, th->context.uc_stack.ss_size, th->context.uc_stack.ss_sp, th->context.uc_link);
    th = STAILQ_NEXT(th, entries);
  }
  printf("\n\n\n");
}
*/
/* passer la main à un autre thread.
 */
int thread_yield(){
  //print_head();
    /* Moving head to tail of the queue */
    STAILQ_REMOVE_HEAD(&fifo_table[current_thread->priority], entries);
    int max = max_fifo();
    thread_t new;
    if (max == -1) {
        new = current_thread;
    }
    else
    {
        new = STAILQ_FIRST(&fifo_table[max]);
    }
    
    /* Getting threads and swaping context */
    STAILQ_INSERT_TAIL(&fifo_table[current_thread->priority], current_thread, entries);
    current_thread->status = STATUS_WAITING;
    new->status = STATUS_RUNNING;
    thread_t old = current_thread;
    current_thread = new;
    swapcontext(&(old->context), &(new->context));
    return EXIT_SUCCESS;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
extern int thread_join(thread_t thread, void **retval) {
    if (thread == NULL) {
        exit_error("null thread", __func__);
    }
    thread_t father = thread_self();
    while (thread->status != STATUS_TERMINATED) {
      STAILQ_REMOVE(&fifo_table[father->priority], father, thread_struct, entries);
      STAILQ_INSERT_HEAD(&joining, father, entries);
      father->status = STATUS_JOINING;
      int max = max_fifo();
      thread_t new = STAILQ_FIRST(&fifo_table[max]);
      current_thread = new;
      swapcontext(&father->context, &new->context);
    }
    father->status = STATUS_RUNNING;
    if (retval != NULL) {
        *retval = thread->val;
    }
    return EXIT_SUCCESS;
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
extern void thread_exit(void *retval) {
    if (current_thread == NULL) {
        exit_error("null thread", __func__);
    }
    STAILQ_REMOVE_HEAD(&fifo_table[current_thread->priority], entries);
    current_thread->status = STATUS_TERMINATED;
    current_thread->val = retval;

    if (current_thread->father != NULL && current_thread->father->status == STATUS_JOINING){
      STAILQ_REMOVE(&joining, current_thread->father, thread_struct, entries);
      STAILQ_INSERT_TAIL(&fifo_table[current_thread->father->priority], current_thread->father, entries);
      current_thread->father->status = STATUS_WAITING;
    }
    
    int max = max_fifo();
    if (max == -1) {
        exit_error("Queues empty after removing current thread", __func__);
    }
    
    thread_t new = STAILQ_FIRST(&fifo_table[max]);
    new->status = STATUS_RUNNING;
    STAILQ_INSERT_TAIL(&dead, current_thread, entries);
    thread_t old = current_thread;
    current_thread = new;
    swapcontext(&old->context, &new->context); // should be setcontext
    exit_error("Thread didn't exit normally", __func__);
}

/*interface possible pour les mutex */
int thread_mutex_init(thread_mutex_t *mutex){
    mutex->lock = UNLOCKED;
    return EXIT_SUCCESS;
}

int thread_mutex_destroy(thread_mutex_t *mutex){
    (void) mutex;
    return EXIT_SUCCESS;
}

int thread_mutex_lock(thread_mutex_t *mutex){
    /* TODO: Faire de l'attente passive au lieu de l'attente active (signaux dans unlock pour appeller lock ?)*/
    /*
    File de thread voulant faire un lock
    Signal pour tous les threads dans la file dans thread_mutex_unlock
    Au reveil, faire 1 compare_and_swap seulement
    Se rendormir si echec
    */
    while (__sync_val_compare_and_swap(&(mutex->lock), UNLOCKED, LOCKED)){
        thread_yield();
    }
    //while (compare_and_swap(&(mutex->lock), UNLOCKED, LOCKED)){}
    return EXIT_SUCCESS;
}

int thread_mutex_unlock(thread_mutex_t *mutex){
    __sync_val_compare_and_swap(&(mutex->lock), LOCKED, UNLOCKED);
    //compare_and_swap(&(mutex->lock), LOCKED, UNLOCKED);
    return EXIT_SUCCESS;
}
#endif
