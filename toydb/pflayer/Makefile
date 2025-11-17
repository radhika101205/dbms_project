#PUBLICDIR= /usr0/cs564/public/project
SRC= buf.c hash.c pf.c
OBJ= buf.o hash.o pf.o
HDR = pftypes.h pf.h 

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)

tests: testhash testpf

testpf: testpf.o pflayer.o
	cc -o testpf testpf.o pflayer.o

testhash: testhash.o pflayer.o
	cc -o testhash testhash.o pflayer.o
<<<<<<< HEAD
=======
pfbench: pfbench.o pf.o buf.o hash.o
	$(CC) -o pfbench pfbench.o pf.o buf.o hash.o

hfstudent: hfstudent.o hf.o pf.o buf.o hash.o
	$(CC) -o hfstudent hfstudent.o hf.o pf.o buf.o hash.o
>>>>>>> upstream/main

$(OBJ): $(HDR)

testhash.o: $(HDR)

testpf.o: $(HDR)

lint: 
	lint $(SRC)

install: pflayer.o 

