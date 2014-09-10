/* 
   Assembler for PDP-8.  Memory and object file creation. 
*/

#include "asm8.h"


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* we want to assemble instructions.  We could assemble and output them
   all at once.  But we have a problem of forward references.  So we
   keep the (partially) assembled instructions in an array of them,
   essentially simulated memory.  That allows us to come back and 
   fix them up when the forward reference is resolved.

   We need to know which memory locations are from assembled
   instructions, and which are just empty; so each memory location
   has a bit (defined/not defined).
*/

INST     memory[4096];
Boolean defined[4096];
Address entry_point = 0;


void Clear_Object_Code(void)
{
    int i;
    for (i = 0; i < 4096; i++)
        {
            defined[i] = FALSE;
        }
}

void Define_Object_Code(Address addr, INST inst, Boolean redefine)
{
    if (debug)
        fprintf(stderr, "object code: 0x%03X = 0x%03X\n", addr, inst);
    if (defined[addr] && !redefine)
        {
            fprintf(stderr, "redefined memory location: 0x%03X: was 0x%03X; new value 0x%03X\n",
                    addr, memory[addr], inst);
            number_of_errors += 1;
        }
                
    defined[addr] = TRUE;
    memory[addr] = inst;
}

INST Fetch_Object_Code(Address addr)
{
    INST inst;

    if (defined[addr])
        inst = memory[addr];
    else
        inst = 0;

    if (debug)
        fprintf(stderr, "read object code: 0x%03X = 0x%03X\n", addr, inst);
    return(inst);
}


void put2(int n)
{
    /* split n into two 6-bit parts */
    int u6 = (n >> 6) & 0x3F;
    int l6 = (n >> 0) & 0x3F;
    putc(u6,output);
    putc(l6,output);
}



void Output_Object_Code(void)
{
    if (debug) fprintf(stderr, "Start object file output\n");
    putc('O',output); 
    putc('B',output); 
    putc('J',output); 
    putc('8',output);
    put2(entry_point);

    int i, j;
    for (i = 0; i < 4096; i++)
        {
            if (defined[i])
                {
                    /* find the next undefined word */
                    j = i;
                    while ((j < 4096) && defined[j] && (j-i < (255-3)/2))
                        j = j + 1;
                    /* output from i to j-1 (inclusive) */
                    int n = 1 + 2 + 2*(j-i);
                    if (debug) fprintf(stderr, "Output block of %d bytes at 0x%03X\n", n, i);
                    putc(n,output);
                    put2(i);
                    while (i < j)
                        {
                            put2(memory[i]);
                            i = i + 1;
                        }
                }
        }
}


