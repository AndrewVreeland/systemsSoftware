/*
Assignment:
vm - HW1 PM/0 virtual machine
Author: Andrew Vreeland and Graham Wilson
Language: C only
To Compile:
gcc -Wall -Wextra -std=c11 -O2 vm.c -o vm
To Execute (on Eustis):
./vm <input_file>
where:
<input_file> is the path to a text file holding one PM/0 instruction
per line, as three integers OP L M
Notes:
- Implements the PM/0 virtual machine described in the homework
instructions.
- No heap allocation and no function-like macros. The PAS array is
indexed, not walked with a pointer.
- Does not implement any VM instruction as a separate function; the
fetch-execute cycle runs directly in main.
- Defines at most three functions: main, base, and one print function.
- Exits with status 0 after a normal halt and with a non-zero status
after any error.
- Tested on Eustis.
Class: COP 3402 - Systems Software
Instructor: Jie Lin, Ph.D.
Due Date: See Webcourses
*/

#include <stdio.h>
#include <stdlib.h>

#define PAS_SIZE 1000
#define TEXT_START 200
#define STACK_TOP 999

int arr[PAS_SIZE];
int programCounter = TEXT_START;
int basePointer = STACK_TOP;
int stackPointer = 1000;
int lastCodeAddr;
int op;
int l;
int m;

// provided helper function
int base(int basePoint, int L)
{
    int arb = basePoint;
    while (L > 0) {
        arb = arr[arb];
        L--;
    }
    return arb;
}

// prints stack, bar before each record
void printStack(void)
{
    int i, j;
    for (i = STACK_TOP; i >= stackPointer; i--) {
        // walk dynamic links for bases
        for (j = basePointer; j != STACK_TOP && j > TEXT_START; j = arr[j - 1]) {
            if (j == i) {
                printf("| ");
                break;
            }
        }
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    const char *names[] = { "", "LIT", "OPR", "LOD", "STO",
                            "CAL", "INC", "JMP", "JPC", "SYS" };
    const char *oprNames[] = { "RTN", "ADD", "SUB", "MUL", "DIV", "EQL",
                               "NEQ", "LSS", "LEQ", "GTR", "GEQ" };
    const char *name = "";
    const char *err = NULL;
    int loadAddr = TEXT_START;
    int halted = 0;
    int a, b, addr;
    FILE *fp;

    if (argc != 2) {
        printf("Usage: ./vm <input file>\n");
        return 1;
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Error: cannot open %s\n", argv[1]);
        return 1;
    }

    // load text segment from file
    while (fscanf(fp, "%d %d %d", &op, &l, &m) == 3) {
        if (loadAddr + 2 > STACK_TOP) {
            printf("\nError: program too large for the text segment\n");
            fclose(fp);
            return 1;
        }
        arr[loadAddr] = op;
        arr[loadAddr + 1] = l;
        arr[loadAddr + 2] = m;
        loadAddr += 3;
    }
    fclose(fp);
    lastCodeAddr = loadAddr - 1;

    printf("\tL\tM\tPC\tBP\tSP\tstack\n");
    printf("Initial values:\t\t\t%d\t%d\t%d\n",
           programCounter, basePointer, stackPointer);

    // fetch-execute cycle
    while (!halted) {
        // fetch the instruction (OP,L,M) from the PAS text
        if (programCounter < TEXT_START || programCounter + 2 > lastCodeAddr) {
            printf("\nError: program counter left the text segment\n");
            return 1;
        }
        op = arr[programCounter];
        l = arr[programCounter + 1];
        m = arr[programCounter + 2];

        // advance before executing
        programCounter += 3;

        // execite the instruction
        switch (op) {
        case 1: // LIT
            name = names[op];
            if (stackPointer - 1 <= lastCodeAddr) {
                printf("\nError: stack overflow\n");
                return 1;
            }
            stackPointer--;
            arr[stackPointer] = m;
            break;

        case 2: // OPR
            if (m < 0 || m > 10) {
                printf("\nError: unknown OPR sub-operation\n");
                return 1;
            }
            name = oprNames[m];
            if (m == 0) { // RTN: restor the callers stack frame and return the PC
                stackPointer = basePointer + 1;
                basePointer = arr[stackPointer - 2];
                programCounter = arr[stackPointer - 3];
                break;
            }
            a = arr[stackPointer + 1];
            b = arr[stackPointer];
            if (m == 4 && b == 0) {
                printf("\nError: division by zero\n");
                return 1;
            }
            stackPointer++;
            switch (m) {
            case 1: arr[stackPointer] = a + b; break;
            case 2: arr[stackPointer] = a - b; break;
            case 3: arr[stackPointer] = a * b; break;
            case 4: arr[stackPointer] = a / b; break;
            case 5: arr[stackPointer] = (a == b); break;
            case 6: arr[stackPointer] = (a != b); break;
            case 7: arr[stackPointer] = (a < b); break;
            case 8: arr[stackPointer] = (a <= b); break;
            case 9: arr[stackPointer] = (a > b); break;
            case 10: arr[stackPointer] = (a >= b); break;
            }
            break;

        case 3: // LOD
            name = names[op];
            addr = base(basePointer, l) - m;
            if (addr <= lastCodeAddr || addr > STACK_TOP) {
                printf("\nError: data address out of range\n");
                return 1;
            }
            if (stackPointer - 1 <= lastCodeAddr) {
                printf("\nError: stack overflow\n");
                return 1;
            }
            stackPointer--;
            arr[stackPointer] = arr[addr];
            break;

        case 4: // STO
            name = names[op];
            addr = base(basePointer, l) - m;
            if (addr <= lastCodeAddr || addr > STACK_TOP) {
                printf("\nError: data address out of range\n");
                return 1;
            }
            arr[addr] = arr[stackPointer];
            stackPointer++;
            break;

        case 5: // CAL: pushes the static and dynamic link and the return address
            name = names[op];
            if (stackPointer - 3 <= lastCodeAddr) {
            printf("\nError: stack overflow\n");
            return 1;
            }
            arr[stackPointer - 1] = base(basePointer, l); // static link
            arr[stackPointer - 2] = basePointer;          // dynamic link
            arr[stackPointer - 3] = programCounter;       // return address
            basePointer = stackPointer - 1;
            programCounter = m; //jump to the address m
            break;

        case 6: // INC
            name = names[op];
            if (stackPointer - m <= lastCodeAddr) {
                printf("\nError: stack overflow\n");
                return 1;
            }
            stackPointer -= m;
            break;

        case 7: // JMP
            name = names[op];
            programCounter = m;
            break;

        case 8: // JPC: conditional jump
            name = names[op];
            if (arr[stackPointer] == 0)
                programCounter = m;
            stackPointer++; // pop either way
            break;

        case 9: // SYS
            name = names[op];
            switch (m) {
            case 1: // write
                printf("Output result is: %d\n", arr[stackPointer]);
                stackPointer++;
                break;
            case 2: // read
                printf("Please Enter an Integer: ");
                if (scanf("%d", &a) != 1)
                    a = 0;
                printf("%d\n", a);
                stackPointer--;
                arr[stackPointer] = a;
                break;
            case 3: // halt
                halted = 1;
                break;
            default:
                printf("\nError: unknown SYS operation\n");
                return 1;
            }
            break;

        default:
            printf("\nError: unknown opcode\n");
            return 1;
        }

        // trace line after execution, prints nmenonic, registers, and current stack contents
        printf("%s\t%d\t%d\t%d\t%d\t%d\t",
               name, l, m, programCounter, basePointer, stackPointer);
        printStack();
    }

    return 0;


}