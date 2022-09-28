run-raw-nuova: raw-nuova
	./raw-nuova prog

raw-nuova:
	gcc raw-nuova.c -o raw-nuova

run-nuova: nuova
	./nuova prog

nuova: nuova.c
	gcc nuova.c -o nuova

run-test: test
	./test

test: test.c
	gcc test.c -o test
