make: memsim

memsim: memsim.c
	gcc memsim.c -o memsim

runms: memsim
	./memsim simple.trace 4 rdm debug