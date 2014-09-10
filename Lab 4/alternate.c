#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//This program emulates a PDP8 machine. It loads in an object file which must conform to the following (X = hex value):
//PC: XXX
//XXX: XXX
//XXX: XXX
//(etcetera)

//input format: pdp8 [-v] filename.obj

#define ON 1
#define OFF 0

typedef char* String;

FILE* input;

short PC; //Program Counter
short rA = 0; //Accumulator
short link = 0; //link bit
long long time = 0; //time in cycles
short memory[4096];
short verbose_mode = OFF;
char code[29]; //instruction mnenonic such as HLT

//START ERROR FUNCTION/////////////////////
void object_file_error(){
    fprintf(stderr, "Error in the object file\n");
    exit(3);
}
//END ERROR FUNCTION/////////////////////
//START SETUP FUNCTIONS/////////////////////
void scan_args(char* argv){
    if ((*argv == '-') && (*(++argv) == 'v')){
        verbose_mode = ON;
        return;
    }
    input = fopen(argv,"r");
    if (input == NULL){
        fprintf(stderr, "Error: invalid input. Acceptable format: pdp8 [-v] filename.obj\n");
        exit(2);
    }
}

void check_line_valid(char s[]){
    if (strlen(s) < 8)  object_file_error();
    if (strncmp(s+3,": ",2)) object_file_error();
    if ( (*(s+8) != '\n') && 
         (*(s+8) != '\0')  ) object_file_error();
}

int get_line(FILE* input, String line){
    int i;
    int c = fgetc(input);
    for (i = 0; (c != EOF) && (c != '\n') && (i < 9); i++){
        *(line + i) = c;
        c = fgetc(input);
    }
    *(line + i) = '\0';
    if (c == EOF) return 0; 
    return 1;
}

int hex_to_binary(int c){
    if ((c >= '0') && (c <= '9')) return (c - '0');
    if ((c >= 'A') && (c <= 'F')) return (c + 10 - 'A');
    if ((c >= 'a') && (c <= 'f')) return (c + 10 - 'a');
    object_file_error();
}

int convert_hex(char s[]){
    int i;
    i = hex_to_binary(*s) << 8;
    i += hex_to_binary(*(s+1)) << 4;
    i += hex_to_binary(*(s+2));
    return i;
}

void put_line(char line[]){
    check_line_valid(line);
    int address = convert_hex(line);
    short data = convert_hex (line + 5);
    *(memory + address) = data;
}

void load_memory(){
    char line[10];
    if (get_line(input,line) == 0) object_file_error(); //get_line == 0 when EOF reached 
    if (strncmp(line,"EP: ", 4) != 0) object_file_error();
        if (strlen(line) != 7) object_file_error();
    PC = convert_hex(line+4);
    while (get_line(input,line)) put_line(line);
    if (*line != '\0') put_line(line);
}
//END SETUP FUNCTIONS/////////////////////
//START INCREMENT FUNCTIONS////////////////////
void increment_pc(){
    PC++;
    PC = PC & 0xFFF;
}
void add_to_rA(short n){
    rA += n;
    if (rA & 0x1000) link = !link;
    rA = rA & 0xFFF;
}
//END INCREMENT FUNCTIONS//////////////////////
//START OPERATE FUNCTIONS////////////////////////
void rotate_right(){
    rA = rA | (link << 12);
    link = rA & 1;
    rA = rA >> 1;
}

void rotate_left(){ 
    rA = rA << 1;
    rA = rA | link;
    link = (rA >> 12);
    rA = rA & 0xFFF;
}

short op7_group1(short data){
    if (data & 0x80){ // Clear the A register; set it to zero. 
        rA = 0;
        strcat(code, " CLA");
    }
    if (data & 0x40){ // Clear the Link bit. 
        link = 0;
        strcat(code, " CLL");
    }
    if (data & 0x20){ //Complement the A register
        rA = (~rA) & 0xFFF;
        strcat(code, " CMA");
    }
    if (data & 0x10){ //Complement the Link bit.
        link = !link;
        strcat(code, " CML");
    }
    if (data & 1){ //Add 1 to the A register.
        add_to_rA(1);
        strcat(code, " IAC");
    }
    if ((data & 8) && (data & 4)){
        strcat(code, " HLT");
        return 0;
    }
    if (data & 8){
        rotate_right();
        if (data & 2){
            rotate_right();
            strcat(code, " RTR");
        }
        else strcat(code, " RAR");
    }
    else if (data & 4){
        rotate_left();
        if (data & 2){
            rotate_left();
            strcat(code, " RTL");
        }
        else strcat(code, " RAL");
    }
    increment_pc();
    return 1;
}

short op7_group2(short data){
    int skip = OFF;
    if (data & 0x40){ //SMA Skip on Minus A
        if (rA & 0x800) skip = ON;
        strcat(code, " SMA");
    }
    if (data & 0x20){ //SZA Skip on Zero A
        if (rA == 0) skip = ON; 
        strcat(code, " SZA");
    }
    if (data & 0x10){ //SNL Skip on Nonzero Link
        if (link) skip = ON; 
        strcat(code, " SNL");
    }
    if (data & 8){ //If RSS is selected, complement the Skip flag
        skip = !skip;
        strcat(code, " RSS");
    }
    if (data & 0x80){ //Clear the A register; set it to zero.
        rA = 0;    
        strcat(code, " CLA");
    }
    if (data & 4){ //Treat the OSR instruction as a NOP
        strcat(code, " OSR");
    }
    if (data & 3){ //Halt
        strcat(code, " HLT");
        return 0;
    }
    increment_pc();
    if (skip) increment_pc();
    return 1;
}
//END OPERATE FUNCTIONS////////////////////////////

int op0_to_6(data){
    short opcode = (data & 07000) >> 9;
    int address = data & 0177;
    short di = data & 0400;
    if (data & 0200){ //if Z/C is one, the high order five bits of the memory address are the same as the high order 5 bits of the address P (current page).
        address = address | (PC & 07600);
    }
    if (di) { //if D/I != 0, access address at current address (indirect addressing)
        address = memory[address];
        time++;
    }
    short device;
    increment_pc();
    switch(opcode){
        case 0: //Logical AND
            rA = rA & memory[address];
            time++;
            strcat(code, " AND");
            break;
        case 1: //Two's complement add
            add_to_rA(memory[address]);
            time++;
            strcat(code, " TAD");
            break;
        case 2: //Increment and skip if zero
            memory[address] = ((memory[address]) + 1) & 0xFFF;
            time++;
            if (memory[address] == 0) increment_pc();
            strcat(code, " ISZ");
            break;
        case 3: //Deposit and clear accumulator
            memory[address] = rA;
            time++;
            rA = 0;
            strcat(code, " DCA");
            break;
        case 4: //Jump to subroutine
            memory[address] = PC;
            time++;
            PC = address + 1;
            strcat(code, " JMS");
            break;
        case 5: //Jump
            PC = address;
            strcat(code, " JMP");
            break;
        case 6: //IOT
            device = (data & 0770) >> 3;
            if (device == 3){
                rA = getchar();
                rA = rA & 0xFFF;
                strcat(code, " IOT 3");
            }
            else if (device == 4){
                putchar((char) (rA & 0xFF));
                strcat(code, " IOT 4");
            }
            else{
                strcat(code, " HLT");
                return 0;
            }
            break;
    }
    if (di) strcat(code, " I");
    return 1;
}

int execute(){
    code[0] = '\0';
    code[1] = '\0';
    short instruction_PC = PC;
    short data = *(memory + PC);
    time++; //instruction fetch takes 1 cycle
    short opcode = data & 07000;
    short halt_state = OFF;
    if (opcode == 07000){
        if (data & 0x100) halt_state = op7_group2(data);
        else halt_state = op7_group1(data);
    }
    else halt_state = op0_to_6(data);
    
    if (verbose_mode){
        fprintf(stderr, "Time %lld: PC=0x%03X instruction = 0x%03X (%s), rA = 0x%03X, rL = %d\n",time,instruction_PC,data,code+1,rA,link);
    }
    return (halt_state);
}

main(int argc, char** argv){
    argc--, argv++; 
    while (argc != 0){
        scan_args(*argv);
        argc--, argv++;
    }
    load_memory();
    while (execute()); //executes until reaches HLT instruction, which returns 0
    return 1;
}
