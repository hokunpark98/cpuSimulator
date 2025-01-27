#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

//메모리 크기(바이트 단위).
#define MEMORY_SIZE 256
// CPU의 범용 레지스터 개수
#define NUM_REGS 8 


// Opcode(어떤 연산을 수행 할 것인지)를 정의
typedef enum {
    HALT    = 0,
    NOP     = 1,
    MOV_RR  = 2, // MOV_RR dst, src : dst <- src
    MOV_RM  = 3, // MOV_RM addr, reg : [addr] <- reg
    MOV_MR  = 4, // MOV_MR reg, addr : reg <- [addr]
    ADD_RR  = 5, // ADD_RR dst, src : dst <- dst + src
    SUB_RR  = 6, // SUB_RR dst, src : dst <- dst - src
    JMP     = 7,
    INVALID = 8
} Opcode;


// 오퍼랜드(연산을 수행할 대상) 유형을 단순화해 놓은 예시.
typedef enum {
    OPERAND_NONE,    // 오퍼랜드가 없음 (HALT, NOP 등)
    OPERAND_REG_REG, // (regA, regB)
    OPERAND_REG_MEM, // (regA, imm)
    OPERAND_MEM_REG, // (imm, regB)
    OPERAND_IMM      // (imm) — 주소 혹은 상수
} OperandType;


// 디코딩된 명령어 정보를 담는 구조체
typedef struct {
    Opcode opcode;
    OperandType opType;
    uint8_t regA;
    uint8_t regB;
    uint8_t imm;
} Instruction;

// 파이프라이닝을 위한 단계 저장
typedef enum {
    STAGE_FETCH,
    STAGE_DECODE,
    STAGE_EXECUTE,
    STAGE_MEMORY,
    STAGE_WRITEBACK
} PipelineStage;


// CPU 상태를 나타내는 구조체
typedef struct {
    uint8_t  regs[NUM_REGS]; // 8bit 레지스터 8개
    uint16_t PC;

    // 다중 사이클을 위한 추가
    PipelineStage stage; // 현재 어떤 사이클인지
    Instruction currentInstr; // 현재 처리 중인 명령어
    uint16_t aluResult; //EX 단계 결과 저장
} CPUState;


// VM 상태
typedef struct {
    CPUState cpu;
    uint8_t memory[MEMORY_SIZE];
    bool running;
} VM;




// VM 초기화
void initVM(VM *vm);

// 명령어 크기(바이트) 반환
uint16_t getInstructionSize(const Instruction *instr);

// 다중 사이클: 한 클록(=1단계) 처리
void multiCycleStep(VM *vm);

// VM 실행 루프 (다중 사이클)
void runVM(VM *vm);

#endif

