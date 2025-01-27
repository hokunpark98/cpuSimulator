// main.c
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

// load.c에 있는 함수 선언
void loadProgramFromFile(VM *vm, const char *filename);

// VM 상태(모든 레지스터, 메모리)를 출력하는 함수

static void printVMState(const VM *vm)
{
    // 1) 레지스터 출력
    printf("----- CPU Registers -----\n");
    for (int i = 0; i < NUM_REGS; i++)
    {
        printf("regs[%d] = %u\n", i, vm->cpu.regs[i]);
    }
    printf("PC = %u\n", vm->cpu.PC);

    // 2) 메모리 덤프 (256바이트, 10진수)
    printf("\n----- Memory Dump (256 bytes, Decimal) -----\n");
    for (int addr = 0; addr < MEMORY_SIZE; addr++)
    {
        // 16바이트 단위로 줄바꿈
        if (addr % 16 == 0)
        {
            printf("\n%3d: ", addr); // 주소 (10진수)
        }
        // 각 바이트를 10진수로 (0~255)
        printf("%3u ", vm->memory[addr]);
    }
    printf("\n\n");
}

int main(int argc, char *argv[])
{
    // 인자로부터 파일 이름 결정
    const char *filename = "program.txt";
    if (argc > 1)
    {
        filename = argv[1];
    }

    // VM 초기화
    VM vm;
    initVM(&vm);

    // 프로그램 로드(텍스트 파일 -> vm.memory)
    loadProgramFromFile(&vm, filename);

    // VM 실행
    runVM(&vm);

    // 전체 상태(모든 레지스터, 메모리) 출력
    printVMState(&vm);

    return 0;
}
