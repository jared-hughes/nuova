pipe: nuova solver
	./solver
	./nuova prog

run-raw-nuova: raw-nuova
	./raw-nuova prog

raw-nuova:
	gcc raw-nuova.c -o raw-nuova

run-nuova: nuova
	./nuova prog

nuova: nuova.c
	gcc nuova.c -o nuova
