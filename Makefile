pipe: nuova solver
	./solver
	./nuova prog

run-nuova: nuova
	./nuova prog

nuova: nuova.c
	gcc nuova.c -o nuova

run-solver: solver
	./solver

solver: solver.c
	gcc solver.c -o solver
