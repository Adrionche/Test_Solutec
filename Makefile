#######################################################
###				CONFIGURATION
#######################################################

SRC_DIR = src
TST_DIR = tst
BLD_DIR = build

CC			   = gcc
CFLAGS		   = -Wall -Wextra -std=gnu99 -g

N_TESTS        = 6

SRC_FILES	   = $(wildcard $(SRC_DIR)/*.c)
SRC_OBJ	       = $(SRC_FILES:%.c=%.o)
TST_FILES	   = $(wildcard $(TST_DIR)/*.c)
TST_OBJ	       = $(TST_FILES:%.c=%.o)
TST_OBJ_PTHRD  = $(TST_FILES:%.c=%_pthread.o)

ARGS           = 10 10

COMPARE_FILE   = compare.py

#######################################################
###				COMMANDS CATEGORIES
#######################################################

# Accessible commands
.PHONY: all help build install run vrun grun test vtest ctest docs clean

#######################################################
###				DEFAULT MAKE COMMAND
#######################################################

all: build

#######################################################
###				MAKE INSTRUCTIONS / HELP
#######################################################

help:
	@echo -e \
	======================================================================	'\n'\
		'\t' ENSEIRB S8 : Projet Systeme                           			'\n'\
	======================================================================	'\n'\
	'\n'Available commands:													'\n'\
		'\t' make     '\t' '\t' Compiler votre bibliothèque et les tests.   '\n'\
		'\t' make check    '\t' Exécuter les tests, avec des valeurs raisonnables pour les tests qui veulent des arguments en ligne de commande.'\n'\
		'\t' make valgrind '\t' Exécuter les tests sous valgrind.           '\n'\
		'\t' make pthreads '\t' Compiler les tests pour les pthreads.       '\n'\
		'\t' make graphs   '\t' Tracer des courbes comparant les performances de votre implémentation et de pthreads en faisant varier les arguments passés aux tests.'\n'\
		'\t' make help	   '\t' Show the availables commands				'\n'\
		'\t' make clean    '\t' Clean all generated objects and binairies	'\n'

#######################################################
###				MAKE BUILD
#######################################################

build: $(SRC_OBJ) $(TST_OBJ)
	@mkdir -p $(BLD_DIR)
	@for t_obj in $(TST_OBJ); do \
		tmp=`echo $${t_obj} | cut -d'/' -f 2 | cut -d'.' -f 1` ; \
		$(CC) $(SRC_OBJ) $${t_obj} -o $(BLD_DIR)/$${tmp} ;\
	done

#######################################################
###				MAKE CHECK
#######################################################

check: build
	@for t_obj in $(TST_OBJ); do \
		tmp=`echo $${t_obj} | cut -d'/' -f 2 | cut -d'.' -f 1` ; \
		echo "\033[1;33m\nExecuting $${tmp}\033[0m"; \
		$(BLD_DIR)/$${tmp} $(ARGS);\
	done

#######################################################
###				MAKE PCHECK
#######################################################

pcheck: pthreads
	@for t_obj in $(TST_OBJ_PTHRD); do \
		tmp=`echo $${t_obj} | cut -d'/' -f 2 | cut -d'.' -f 1` ; \
		echo "\033[1;33m\nExecuting $${tmp}\033[0m"; \
		$(BLD_DIR)/$${tmp} $(ARGS);\
	done

#######################################################
###				MAKE VALGRIND
#######################################################

valgrind: build
	@for t_obj in $(TST_OBJ); do \
		tmp=`echo $${t_obj} | cut -d'/' -f 2 | cut -d'.' -f 1` ; \
		echo "\033[1;33m\nExecuting $${tmp}\033[0m"; \
		valgrind $(BLD_DIR)/$${tmp} $(ARGS);\
	done

#######################################################
###				MAKE PTHREADS
#######################################################

pthreads: $(SRC_OBJ) $(TST_OBJ_PTHRD)
	@mkdir -p $(BLD_DIR)
	@for t_obj in $(TST_OBJ_PTHRD); do \
		tmp=`echo $${t_obj} | cut -d'/' -f 2 | cut -d'.' -f 1` ; \
		$(CC) $${t_obj} -o $(BLD_DIR)/$${tmp} -lpthread;\
	done

#######################################################
###				MAKE GRAPHS
#######################################################

graphs: build
	@python3 ./$(COMPARE_FILE)

#######################################################
###				MAKE CLEAN
#######################################################

clean:
	@echo Starting cleanup...
	@find . -type f -name '*.o' -delete
	@find . -type f -name '*.so' -delete
	@rm -rf $(BLD_DIR)/*
	@echo Cleanup complete.


#######################################################
###				OBJECTS FILES
#######################################################

%_pthread.o : %.c
	$(CC) -DUSE_PTHREAD -c $(CFLAGS) $(CPPFLAGS) $< -o $@ -lpthread

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
