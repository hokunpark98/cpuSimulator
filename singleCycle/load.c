#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cpu.h"

#define MAX_LINE 128

// txt파일의 문자열을 받아 Opcode를 결정하는 함수.
static Opcode stringToOpcode(const char *str)
{
    if (strcmp(str, "HALT") == 0)
        return HALT;
    if (strcmp(str, "NOP") == 0)
        return NOP;
    if (strcmp(str, "MOV_RR") == 0)
        return MOV_RR;
    if (strcmp(str, "MOV_RM") == 0)
        return MOV_RM;
    if (strcmp(str, "MOV_MR") == 0)
        return MOV_MR;
    if (strcmp(str, "ADD_RR") == 0)
        return ADD_RR;
    if (strcmp(str, "SUB_RR") == 0)
        return SUB_RR;
    if (strcmp(str, "JMP") == 0)
        return JMP;

    return INVALID; // 알 수 없는 문자열
}

// program.txt를 열어서 한 줄씩 읽는다:
void loadProgramFromFile(VM *vm, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    // PC(메모리에 명령어를 쓸 위치)
    uint16_t pc = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp))
    {
        // 주석이나 빈 줄 처리
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
        {
            continue;
        }

        // 공백 기준으로 첫 토큰 추출
        char *token = strtok(line, " \t\r\n");
        if (!token)
            continue;

        // 레지스터 초기화 처리 (R로 시작한다면 레지스터인것으로)
        // 레지스터에 들어갈 값을 해당 레지스터에 저장
        if (token[0] == 'R')
        {
            char regChar = token[1];
            // 레지스터 번호확인
            if (regChar >= '0' && regChar <= '7')
            {
                int regIndex = regChar - '0';
                // 레지스터에 들어갈 값을 레지스터에 저장
                char *valStr = strtok(NULL, " \t\r\n");
                if (valStr)
                {
                    int val = (int)strtol(valStr, NULL, 0);
                    if (regIndex < NUM_REGS)
                    {
                        // 해당 register에 값을 저장
                        vm->cpu.regs[regIndex] = (uint8_t)(val & 0xFF);
                    }
                    else
                    {
                        printf("Register R%d out of range!\n", regIndex);
                    }
                }
                continue;
            }
        }

        // 명령어 처리
        Opcode op = stringToOpcode(token);
        if (op == INVALID)
        {
            printf("Unknown opcode: %s\n", token);
            continue;
        }

        // opcode 저장
        vm->memory[pc] = (uint8_t)op;
        pc++;

        // switch-case로 opcode별 오퍼랜드 처리
        // 메모리에 프로그램을 올리는 거니까 주소나, opcode 번호나 값 등을 메모리에 올림
        switch (op)
        {
        case HALT:
        case NOP:
            // 오퍼랜드 없음
            break;

        case MOV_RR:
        case ADD_RR:
        case SUB_RR:
        {
            char *aStr = strtok(NULL, " \t\r\n");
            char *bStr = strtok(NULL, " \t\r\n");
            if (aStr && bStr)
            {
                int a = (int)strtol(aStr, NULL, 0);
                int b = (int)strtol(bStr, NULL, 0);
                vm->memory[pc] = (uint8_t)(a & 0xFF);
                vm->memory[pc + 1] = (uint8_t)(b & 0xFF);
                pc += 2;
            }
            break;
        }

        case MOV_RM:
        case MOV_MR:
        {
            // reg + imm (2바이트)
            char *rStr = strtok(NULL, " \t\r\n");
            char *iStr = strtok(NULL, " \t\r\n");
            if (rStr && iStr)
            {
                int r = (int)strtol(rStr, NULL, 0);
                int imm = (int)strtol(iStr, NULL, 0);
                vm->memory[pc] = (uint8_t)(r & 0xFF);
                vm->memory[pc + 1] = (uint8_t)(imm & 0xFF);
                pc += 2;
            }
            break;
        }

        case JMP:
        {
            // imm (1바이트)
            char *immStr = strtok(NULL, " \t\r\n");
            if (immStr)
            {
                int imm = (int)strtol(immStr, NULL, 0);
                vm->memory[pc] = (uint8_t)(imm & 0xFF);
                pc += 1;
            }
            break;
        }

        default:
            // INVALID 등은 위에서 이미 거르지만
            break;
        }
    }

    fclose(fp);

    // PC 초기값 0으로 설정
    vm->cpu.PC = 0;
    printf("Program loaded from %s. PC=0\n", filename);
}
