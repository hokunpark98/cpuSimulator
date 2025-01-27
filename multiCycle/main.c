#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

// load.c에 있는 함수 (프로그램 로더)
void loadProgramFromFile(VM *vm, const char *filename);

// 디버그용: VM 상태 출력
static void printVMState(const VM *vm)
{
    printf("----- CPU Registers -----\n");
    for (int i = 0; i < NUM_REGS; i++) {
        printf("regs[%d] = %u\n", i, vm->cpu.regs[i]);
    }
    printf("PC = %u\n", vm->cpu.PC);

    // 메모리 덤프
    printf("\n----- Memory Dump (256 bytes, Decimal) -----\n");
    for (int addr = 0; addr < MEMORY_SIZE; addr++) {
        if (addr % 16 == 0) {
            printf("\n%3d: ", addr);
        }
        printf("%3u ", vm->memory[addr]);
    }
    printf("\n\n");
}


int main(int argc, char *argv[])
{
    const char *filename = "program.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    VM vm;
    initVM(&vm);

    // 텍스트 파일 → 메모리 로드
    loadProgramFromFile(&vm, filename);

    // 다중 사이클 VM 실행
    runVM(&vm);

    // 실행 종료 후 전체 상태 출력
    printVMState(&vm);

    return 0;
}
