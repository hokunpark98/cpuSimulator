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

    vm->running = false;
    vm->cpu.stage = STAGE_FETCH;
}

// 얼만큼 PC를 증가시킬 지 계산
uint16_t getInstructionSize(const Instruction *instr)
{
    switch (instr->opcode)
    {
    case HALT:
    case NOP:
        return 1;

    // opcode + regA + regB  (예: MOV_RR dst, src)
    // ADD_RR, SUB_RR도 동일 형식
    case MOV_RR:
    case ADD_RR:
    case SUB_RR:
        return 3;

    // opcode + 바이트2 + 바이트3
    // MOV_RM addr, reg
    // MOV_MR reg, addr
    case MOV_RM:
    case MOV_MR:
        return 3;

    // opcode + imm
    case JMP:
        return 2;

    default:
        return 1; // INVALID 등
    }
}

/*  -------------------------------------
        다중 사이클 단계별 함수
    -------------------------------------
*/

// STEP1: IF (PC가 가리키는 메모리 주소에서 opcode를 가져옴)
static void stageFetch(VM *vm)
{
    CPUState *cpu = &vm->cpu;

    if (cpu->PC >= MEMORY_SIZE)
    {
        printf("PC out of memory range!\n");
        vm->running = false;
        return;
    }

    uint8_t opcodeByte = vm->memory[cpu->PC];
    cpu->currentInstr.opcode = (Opcode)opcodeByte;

    // 오퍼랜드 초기화
    cpu->currentInstr.opType = OPERAND_NONE;
    cpu->currentInstr.regA = 0;
    cpu->currentInstr.regB = 0;
    cpu->currentInstr.imm = 0;
}

// STEP2: ID (가져온 opcode를 기반으로 PC를 증가시켜 오퍼랜드를 구성함)
//
//  ★★★ 첫 번째 피연산자를 목적지, 두 번째 피연산자를 소스로 해석 ★★★
//
static void stageDecode(VM *vm)
{
    CPUState *cpu = &vm->cpu;
    Instruction *instr = &cpu->currentInstr;

    switch (instr->opcode)
    {
    case HALT:
    case NOP:
        instr->opType = OPERAND_NONE;
        break;

    // MOV_RR dst, src
    // ADD_RR dst, src
    // SUB_RR dst, src
    // => regA = dst, regB = src
    case MOV_RR:
    case ADD_RR:
    case SUB_RR:
        instr->opType = OPERAND_REG_REG;
        instr->regA = vm->memory[cpu->PC + 1]; // 목적지(dst)
        instr->regB = vm->memory[cpu->PC + 2]; // 소스(src)
        //printf("PC=%d, regA=%d, regB=%d\n", cpu->PC + 2, instr->regA, instr->regB);
        break;

    // MOV_RM addr, reg => [addr] <- reg
    // => 첫 번째(imm=addr), 두 번째(regB=reg)
    case MOV_RM:
        instr->opType = OPERAND_REG_MEM;
        instr->imm = vm->memory[cpu->PC + 1];  // 목적지(메모리 주소)
        instr->regB = vm->memory[cpu->PC + 2]; // 소스(레지스터)
        break;

    // MOV_MR reg, addr => reg <- [addr]
    // => 첫 번째(regA=reg), 두 번째(imm=addr)
    case MOV_MR:
        instr->opType = OPERAND_MEM_REG;
        instr->regA = vm->memory[cpu->PC + 1]; // 목적지(레지스터)
        instr->imm = vm->memory[cpu->PC + 2];  // 소스(메모리 주소)
        break;

    case JMP:
        instr->opType = OPERAND_IMM;
        instr->imm = vm->memory[cpu->PC + 1];
        break;

    default:
        instr->opcode = INVALID;
        break;
    }
}

// STEP3: EXECUTE
static void stageExecute(VM *vm)
{
    CPUState *cpu = &vm->cpu;
    Instruction *instr = &cpu->currentInstr;
    cpu->aluResult = 0;

    switch (instr->opcode)
    {
    case HALT:
        vm->running = false;
        break;

    case NOP:
        break;

    // MOV_RR dst, src => dst <- src
    // EX 단계에서 aluResult에 "src의 값"을 임시 저장
    case MOV_RR:
        // dst = regA, src = regB
        cpu->aluResult = cpu->regs[instr->regB];
        break;

    // ADD_RR dst, src => dst <- dst + src
    case ADD_RR:
    {
        uint16_t sum = (uint16_t)cpu->regs[instr->regA] + (uint16_t)cpu->regs[instr->regB];
        cpu->aluResult = (uint8_t)(sum & 0xFF);
        break;
    }

    // SUB_RR dst, src => dst <- dst - src
    case SUB_RR:
    {
        uint16_t diff = (uint16_t)cpu->regs[instr->regA] - (uint16_t)cpu->regs[instr->regB];
        cpu->aluResult = (uint8_t)(diff & 0xFF);
        break;
    }

    // 메모리 접근은 MEM 단계에서
    case MOV_RM: // [addr] <- reg
    case MOV_MR: // reg <- [addr]
        break;

    case JMP:
        cpu->aluResult = (uint16_t)instr->imm;
        break;

    case INVALID:
    default:
        printf("Invalid opcode\n");
        vm->running = false;
        break;
    }
}

// STEP4: MEM
static void stageMemory(VM *vm)
{
    CPUState *cpu = &vm->cpu;
    Instruction *instr = &cpu->currentInstr;

    switch (instr->opcode)
    {
    // MOV_RM addr, reg => [addr] <- reg
    case MOV_RM:
        if (instr->imm < MEMORY_SIZE)
        {
            // imm=addr, regB=소스 레지스터
            vm->memory[instr->imm] = cpu->regs[instr->regB];
        }
        else
        {
            printf("Error: MOV_RM out of range\n");
            vm->running = false;
        }
        break;

    // MOV_MR reg, addr => reg <- [addr]
    case MOV_MR:
        if (instr->imm < MEMORY_SIZE)
        {
            // imm=addr, regA=목적지 레지스터
            cpu->aluResult = vm->memory[instr->imm];
        }
        else
        {
            printf("Error: MOV_MR out of range\n");
            vm->running = false;
        }
        break;

    // ADD, SUB, MOV_RR, JMP, HALT, NOP 등은 메모리 접근 없음
    default:
        break;
    }
}

// STEP5: WB 단계
static void stageWriteback(VM *vm)
{
    CPUState *cpu = &vm->cpu;
    Instruction *instr = &cpu->currentInstr;

    switch (instr->opcode)
    {

    // MOV_RR dst, src => dst <- aluResult
    case MOV_RR:
        cpu->regs[instr->regA] = (uint8_t)cpu->aluResult;
        break;

    // ADD_RR dst, src => dst <- aluResult
    // SUB_RR dst, src => dst <- aluResult
    case ADD_RR:
    case SUB_RR:
        cpu->regs[instr->regA] = (uint8_t)cpu->aluResult;
        break;

    // MOV_MR reg, addr => reg <- aluResult
    case MOV_MR:
        cpu->regs[instr->regA] = (uint8_t)cpu->aluResult;
        break;

    case JMP:
        if (cpu->aluResult < MEMORY_SIZE)
        {
            cpu->PC = cpu->aluResult;
            return;
        }
        else
        {
            printf("Error: JMP out of range\n");
            vm->running = false;
        }
        break;

    case HALT:
    default:
        break;
    }

    // JMP가 아닌 경우, 명령어 길이만큼 PC 증가
    if (instr->opcode != JMP)
    {
        cpu->PC += getInstructionSize(instr);
    }
}

/*  -------------------------------------
        실제 VM 실행 (각 스테이지를 한 클록마다 순서대로 실행)
    -------------------------------------
*/

void multiCycleStep(VM *vm)
{
    CPUState *cpu = &vm->cpu;
    if (!vm->running)
        return;

    switch (cpu->stage)
    {
    case STAGE_FETCH:
        stageFetch(vm);
        cpu->stage = STAGE_DECODE;
        break;

    case STAGE_DECODE:
        stageDecode(vm);
        cpu->stage = STAGE_EXECUTE;
        break;

    case STAGE_EXECUTE:
        stageExecute(vm);
        cpu->stage = STAGE_MEMORY;
        break;

    case STAGE_MEMORY:
        stageMemory(vm);
        cpu->stage = STAGE_WRITEBACK;
        break;

    case STAGE_WRITEBACK:
        stageWriteback(vm);
        cpu->stage = STAGE_FETCH; // 다음 명령어 실행을 위해 fetch 단계로 변경
        break;
    }
}

// VM 실행 루프
void runVM(VM *vm)
{
    vm->cpu.stage = STAGE_FETCH; // 초기화
    vm->running = true;

    while (vm->running)
    {
        multiCycleStep(vm);
    }
    printf("VM stopped.\n");
}
