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
int wordWrap = 0;
void printBlock(int getBlock[253], int count);

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


void Output_Object_Code(void)
{
 // binaryFile = fopen("thien.obj","wb");
 // printf("asdfsadfsdafsadfa %s asdfsadfsadfsadfsad\n",output );
 int hex1 = 0x4F;
 int hex2 = 0x42;
 int hex3 = 0x4A;
 int hex4 = 0x38;

 fprintf(outputOBJ, "%c%c",hex1, hex2); //First 2 bytes: EP
 fprintf(outputOBJ, "%c%c",hex3, hex4); //First 2 bytes: EP
 wordWrap+=4;

 fprintf(outputOBJ, "%c%c",((entry_point >> 6) & 0x3F), entry_point& 0x3F); //First 2 bytes: EP
 wordWrap+=2;

 int i;   
 int count=0;
 int getBlock[253];
 for (i = 0; i < 4096; i++)
          {
              if (defined[i]){
                while(defined[i] && i < 4096){
                  if(count==0){
                    getBlock[count] = (i>>6) & 0x3F;
                    count++;
                    getBlock[count] = i & 0x3F;
                    count++;
                  }
                  
                  getBlock[count] = (memory[i] >> 6) & 0x3F;
                  count++;
                  
                  if(count==253){
                    printBlock(getBlock,count);
                    count=0;
                  }
                    getBlock[count] =  memory[i] & 0x3F;
                    count++;

                  // if(count==253){
                  //   printBlock(getBlock,count);
                  //   count=0;
                  // }

                  i++;
                }

                 if(count !=0){
                  printBlock(getBlock,count);
                  count=0;
                }

              }
          }

    fprintf(output, "EP: %03X\n", entry_point);
    for (i = 0; i < 4096; i++)
        {
            if (defined[i])
                fprintf(output, "%03X: %03X\n", i, memory[i]);
        }
}

void printBlock(int getBlock[253], int count){
  int i;
  fprintf(outputOBJ, "%c",count+1);
  wordWrap++;

  for(i = 0; i < count; i++){
    fprintf(outputOBJ, "%c",getBlock[i]);
    ++wordWrap;
  }
}