run-raw-nuova: raw-nuova
	./raw-nuova prog

raw-nuova:
	gcc raw-nuova.c -O3 -o raw-nuova

run-nuova: nuova
	./nuova prog

nuova: nuova.c
	gcc nuova.c -O3 -o nuova

run-test: test
	./test

test: test.c
	gcc test.c -O3 -o test
