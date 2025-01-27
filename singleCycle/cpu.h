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
    HALT = 0,      // 프로그램 종료
    NOP = 1,       // 아무 동작 없음
    MOV_RR = 2,    // MOV regA <- regB
    MOV_RM =3 ,    // MOV [imm] <- regA
    MOV_MR = 4,    // MOV regB <- [imm]
    ADD_RR = 5,    // regA = regA + regB
    SUB_RR = 6,    // regA = regA - regB
    JMP = 7,       // PC = imm
    INVALID = 8    // 알 수 없는 명령어 (디코딩 실패 시)
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


// CPU 상태를 나타내는 구조체
typedef struct {
    uint8_t  regs[NUM_REGS]; // 8bit 레지스터 8개
    uint16_t PC;
} CPUState;


// VM 상태
typedef struct {
    CPUState cpu;
    uint8_t memory[MEMORY_SIZE];
    bool running;
} VM;



/**
 * VM 초기화 함수
*/
void initVM(VM *vm);

/**
 * Fetch랑 Decode 함수
 */
Instruction decodeInstruction(VM *vm);

/**
 * 명령어 크기를 반환하는 함수
 */
uint16_t getInstructionSize(const Instruction *instr);

/**
 * Execute 함수 (실제 명령어 수행)
 */
void executeInstruction(VM *vm, const Instruction *instr);

/**
 * 단일 사이클(=1개의 명령어) 처리:
 */
void singleCycle(VM *vm);

/**
 * VM 메인 루프
 */
void runVM(VM *vm);

#endif 
