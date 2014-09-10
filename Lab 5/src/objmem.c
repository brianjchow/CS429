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
int wrap = 0;
void print_block(int block[253], int count);

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


void Output_Object_Code(void) {

	fprintf(output, "%c%c%c%c", 0x4f, 0x42, 0x4a, 0x38);		// OBJ8
	wrap += 4;
	fprintf(output, "%c%c", ((entry_point >> 6) & 0x3f), entry_point & 0x3f);
	wrap += 2;

	int counter = 0;
	int block[253];
	int i = 0;
	for (; i < 4096; i++) {
		if (defined[i]) {
			while (defined[i] && i < 4096) {
				if (counter == 0) {

					//printf("%d\n", block[counter]);

					block[counter] = (i >> 6) & 0x3f;
					counter++;
					block[counter] = i & 0x3f;
					counter++;
				}
				block[counter] = (memory[i] >> 6) & 0x3f;
				counter++;
				if (counter == 253) {
					print_block(block, counter);
					counter = 0;
				}
				block[counter] = memory[i] & 0x3f;
				counter++;
				i++;
			}
			if (counter != 0) {
				print_block(block, counter);
				counter = 0;
			}
		}
	}

}

void print_block(int block[253], int counter) {
	int i;
	fprintf(output, "%c", counter + 1);

	//printf("%c", counter + 1);

	wrap++;

	for (i = 0; i < counter; i++) {
		fprintf(output, "%c", block[i]);

		//printf("%c", block[i]);

		wrap++;
	}
}
