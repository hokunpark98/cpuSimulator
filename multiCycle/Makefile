all: multiCycleCPUSimulator

multiCycleCPUSimulator: cpu.o load.o main.o
	gcc -o multiCycleCPUSimulator cpu.o load.o main.o

cpu.o: cpu.c cpu.h
	gcc -c cpu.c

load.o: load.c cpu.h
	gcc -c load.c

main.o: main.c cpu.h
	gcc -c main.c

clean:
	rm -f *.o multiCycleCPUSimulator

