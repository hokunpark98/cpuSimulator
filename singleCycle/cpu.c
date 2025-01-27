#include <stdio.h>
#include <string.h>
#include "cpu.h"

/**
 * VM 초기화
 * VM 구조체를 0으로 초기화
 * 필수적으로 running 플래그를 false 또는 필요 시 true로 세팅
 * PC, 레지스터, 메모리 등을 모두 초기 상태로 만듦
 */
void initVM(VM *vm)
{
    memset(vm, 0, sizeof(VM));
}

/**
 * 디코딩
 * PC가 가리키는 메모리 바이트들을 읽어 명령어 구조체에 넣음
 */

Instruction decodeInstruction(VM *vm)
{
    Instruction instr;
    memset(&instr, 0, sizeof(instr));

    // opcode|operand 순으로 되어있어서, 처음 한 바이트를 opcode로 인식
    uint8_t opcodeByte = vm->memory[vm->cpu.PC];

    // opcode enum에 있는 것으로 변환
    // ex) opcodeByte = 1 -> NOP(= 1) 으로 해석
    instr.opcode = (Opcode)opcodeByte;

    // opcode에 따라 operand들을 읽음
    switch (instr.opcode)
    {
    case HALT:
    case NOP:
        instr.opType = OPERAND_NONE;
        break;
    case MOV_RR:
    case ADD_RR:
    case SUB_RR:
        instr.opType = OPERAND_REG_REG;
        instr.regA = vm->memory[vm->cpu.PC + 1];
        instr.regB = vm->memory[vm->cpu.PC + 2];
        break;
    // 레지스터에 있는 값을 imm에 저장된 메모리 주소로 이동
    case MOV_RM:
        instr.opType = OPERAND_REG_MEM;
        instr.regA = vm->memory[vm->cpu.PC + 1];
        instr.imm = vm->memory[vm->cpu.PC + 2];
        break;
    // imm 메모리 주소에 있는 갑을 레지스터로 이동
    case MOV_MR:
        instr.opType = OPERAND_MEM_REG;
        instr.imm = vm->memory[vm->cpu.PC + 1];
        instr.regB = vm->memory[vm->cpu.PC + 2];
        break;
    // 점프할 imm 값을 가져옴
    case JMP:
        instr.opType = OPERAND_IMM;
        instr.imm = vm->memory[vm->cpu.PC + 1];
        break;
    default:
        // 알 수 없는 opcode
        instr.opcode = INVALID;
        instr.opType = OPERAND_NONE;
        break;
    }

    return instr;
}

/**
 * 명령어 바이트 길이를 반환.
 * PC 얼마나 증가시킬지 (지금 명령어 크기만큼 더해준 주소로 가게)
 */
uint16_t getInstructionSize(const Instruction *instr)
{
    switch (instr->opcode)
    {
    case HALT:
    case NOP:
        return 1;
    case MOV_RR:
    case ADD_RR:
    case SUB_RR:
        return 3;
    case MOV_RM:
    case MOV_MR:
        return 3;
    case JMP:
        return 2;
    // INVALID
    default:
        return 1;
    }
}

// 실제 명령어를 실행하는 함수
void executeInstruction(VM *vm, const Instruction *instr)
{
    switch (instr->opcode)
    {
    // 프로그램 종료
    case HALT:
    {
        vm->running = false;
        break;
    }
    // 아무 동작 안 함
    case NOP:
    {
        break;
    }
    // 레지스터 간 값 이동 regA <- regB
    case MOV_RR:
    {
        vm->cpu.regs[instr->regA] = vm->cpu.regs[instr->regB];
        break;
    }
    // 레지스터에 있는 값을 메모리 주소로 이동 imm <- regA
    case MOV_RM:
    {
        if (instr->imm < MEMORY_SIZE)
        {
            vm->memory[instr->imm] = vm->cpu.regs[instr->regA];
        }
        else
        {
            printf("Error: MOV_RM out of memory range\n");
            vm->running = false;
        }
        break;
    }

    // 메모리에 있는 값을 특정 레지스터로 이동
    case MOV_MR:
    {
        if (instr->imm < MEMORY_SIZE)
        {
            vm->cpu.regs[instr->regB] = vm->memory[instr->imm];
        }
        else
        {
            printf("Error: MOV_RM out of memory range\n");
            vm->running = false;
        }
        break;
    }

    case ADD_RR:
    {
        // 8비트 레지스터 두 개 더할 시 9비트 오버플로우 발생할 수 있어서 16으로 먼저 계산
        uint16_t result = (uint16_t)vm->cpu.regs[instr->regA] +
                          (uint16_t)vm->cpu.regs[instr->regB];
        // 오버플로우 된거 마스킹 0000 0000 1111 1111
        vm->cpu.regs[instr->regA] = (uint8_t)(result & 0xFF);
        break;
    }

    case SUB_RR:
    {
        uint16_t result = (uint16_t)vm->cpu.regs[instr->regA] -
                          (uint16_t)vm->cpu.regs[instr->regB];
        vm->cpu.regs[instr->regA] = (uint8_t)(result & 0xFF);
        break;
    }

    // PC 값을 imm으로 변경
    case JMP:
    {
        if (instr->imm < MEMORY_SIZE)
        {
            vm->cpu.PC = instr->imm;
            return;
        }
        else
        {
            printf("Error: JMP out of memory range\n");
            vm->running = false;
        }
        break;
    }

    case INVALID:
    default:
    {
        printf("Error: Invalid opcode (0x%X) at PC=%u\n",
               instr->opcode, vm->cpu.PC);
        vm->running = false;
        break;
    }
    }
    vm->cpu.PC += getInstructionSize(instr);
}

// 한 번 사이클에 한 개 명령어 실행
// fetch -> decode -> execute
void singleCycle(VM *vm)
{
    // step1: fetch & decode
    Instruction instr = decodeInstruction(vm);
    executeInstruction(vm, &instr);
}

// VM 실행 루프
// running = true인동안 실행
void runVM(VM *vm)
{
    vm->running = true;
    while (vm->running)
    {
        if (vm->cpu.PC >= MEMORY_SIZE)
        {
            printf("Error: PC out of memory range!\n");
            vm->running = false;
            break;
        }
        singleCycle(vm);
    }
    printf("VM stopped.\n");
}
