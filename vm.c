
/*
Assignment:
vm - HW1 PM/0 virtual machine
Author: <Andrew Vreeland> <Graham Wilson>
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
13
- Defines at most three functions: main, base, and one print function.
- Exits with status 0 after a normal halt and with a non-zero status
after any error.
- Tested on Eustis.
Class: COP 3402 - Systems Software
Instructor: Jie Lin, Ph.D.
Due Date: See Webcourses
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>



int arr[1000];
int programCounter = 200;
int basePointer = 999; 
int stackPointer = 1000;
int op;
int l;
int m;



int main(int argc, char *argv[])
{

    int instructionValue;

    FILE *fp = fopen(argv[1], "file");
    if (fp == NULL) {
    printf("Error: cannot open %s\n", argv[1]);
    return 1;
    }

    while(fscanf(fp, "%d", &instructionValue) == 1)
    {
        
        arr[programCounter] = instructionValue;
        programCounter +=1;

    }
    
    fclose(fp);

    int numberOfInstructions = (programCounter - 200) / 3;



    

    return 0;
}

// helper function
int base(int basePoint, int L)
{
int arb = basePoint;
while (L > 0) {
arb = arr[arb];
L--;
}
return arb;
}