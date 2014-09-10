/*
  PDP-8 Interpreter:

   opcodes include:

   mem-ref:  TAD, AND, ISZ, DCA, JMS, JMP
   operate:  CLA, CLL, CMA, CML, RAR, RAL, IAC, SMA, SZA, SNL, OSR, HLT,
             NOP, SPA, SNA, SZL, SKP  RTR, RTL
   i/o:      IOT

*/

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE *input;

typedef short Boolean;
#define TRUE 1
#define FALSE 0

Boolean debug = FALSE;
Boolean verbose = FALSE;
Boolean ASCII = TRUE;

typedef char *STRING;

#define CAST(t,e) ((t)(e))
#define TYPED_MALLOC(t) CAST(t*, malloc(sizeof(t)))


/* ***************************************************************** */
/*                                                                   */
/* print representation of a character for debugging                 */
/*                                                                   */
char   *printrep (unsigned short  c)
{
    static char pr[8];

    if (c < 32)
        {
            /* control characters */
            pr[0] = '^';
            pr[1] = c + 64;
            pr[2] = '\0';
        }
    else if (c < 127)
        {
            /* printing characters */
            pr[0] = c;
            pr[1] = '\0';
        }
    else if (c == 127)
        return("<del>");
    else if (c <= 0377)
        {
            /* upper 128 codes from 128 to 255;  print as \ooo - octal  */
            pr[0] = '\\';
            pr[3] = '0' + (c & 7);
            c = c >> 3;
            pr[2] = '0' + (c & 7);
            c = c >> 3;
            pr[1] = '0' + (c & 3);
            pr[4] = '\0';
        }
    else
        {
            /* very large number -- print as 0xffff - 4 digit hex */
            (void)sprintf(pr, "0x%04x", c);
        }
    return(pr);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* MALLOC space for a string and copy it */

STRING remember_string(const STRING name)
{
    size_t n;
    STRING p;

    if (name == NULL) return(NULL);

    /* get memory to remember file name */
    n = strlen(name) + 1;
    p = CAST(STRING, malloc(n));
    strcpy(p, name);
    return(p);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

char line[10];
int line_length = 8;

int get_next_line(void)
{
    /* get the first character to see if we have EOF */
    int c;
    int i = 0;
    c = getc(input);
    if (c != EOF)
        {
            line[i] = c;
            i = 1;
            while (((c = getc(input)) != EOF) && (c != '\n'))
                {
                    if (i < line_length)
                        {
                            line[i] = c;
                            i = i + 1;
                        }
                }
        }
    line[i] = '\0';

    if (debug) fprintf(stderr, "next input line: %s\n", line);

    if ((c == EOF) && (i == 0))
        return(EOF);
    else
        return(i);
}



/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

Boolean is_hex(char c)
{
    if (('0' <= c) && (c <= '9')) return(TRUE);
    if (('A' <= c) && (c <= 'F')) return(TRUE);
    if (('a' <= c) && (c <= 'f')) return(TRUE);
    return(FALSE);
}

int hex_char_value(char c)
{
    if (('0' <= c) && (c <= '9')) return(c-'0');
    if (('A' <= c) && (c <= 'F')) return(c-'A' + 10);
    if (('a' <= c) && (c <= 'f')) return(c-'a' + 10);
    return(-1);
}

int hex_value(char *p)
{
    int n = 0;
    while (is_hex(*p))
        {
            n = n * 16 + hex_char_value(*p);
            p++;
        }
    return(n);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* PDP-8 types */

typedef short Address;
typedef short INST;
typedef short Word;


#define MASK_W12      0xFFF
#define MASK_UPPER6   0xFC0
#define MASK_LOWER6   0x03F
#define MASK_SIGN_BIT 0x800


#define MASK_I_BIT   0x100
#define MASK_ZC_BIT  0x080
#define MASK_OFFSET  0x07F

#define MASK_OVERFLOW 0xF000    /* Any bit in high-order 4 bits is overflow */

#define MASK_CLA 0x080
#define MASK_CLL 0x040
#define MASK_CMA 0x020
#define MASK_CML 0x010
#define MASK_RAR 0x008
#define MASK_RAL 0x004
#define MASK_BSW 0x002
#define MASK_IAC 0x001
#define MASK_SMA 0x040
#define MASK_SZA 0x020
#define MASK_SNL 0x010
#define MASK_RSS 0x008
#define MASK_OSR 0x004
#define MASK_HLT 0x002

#define MASK_IOT_DEVICE   0x1F8
#define MASK_IOT_FUNCTION 0x007


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

INST     memory[4096];
Boolean defined[4096];


void Clear_Memory(void)
{
    int i;
    for (i = 0; i < 4096; i++)
        {
            defined[i] = FALSE;
        }
}

void Store_Memory(Address addr, Word value)
{
    if (debug)
        fprintf(stderr, "write memory: 0x%03X = 0x%03X\n", addr, value);
    defined[addr] = TRUE;
    memory[addr] = value & MASK_W12;
}

INST Fetch_Memory(Address addr)
{
    Word value;

    if (defined[addr])
        value = memory[addr];
    else
        value = 0;

    if (debug)
        fprintf(stderr, "read memory: 0x%03X = 0x%03X\n", addr, value);
    return(value);
}



/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* registers */
Address PC = 0;
Word A = 0;
Word L = 0;

/* internal controls */
Word Switch_Register = 0;
Boolean Halted = TRUE;
long long time = 0;

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

Address Load_ASCII_Object_File(STRING name)
{
    Address entry_point = 0;
    Word data;

    while (get_next_line() != EOF)
        {
            char *p = line;
            while (isalnum(*p)) p++;
            *p = '\0';
            while (!is_hex(*p)) p++;
            /* two values: one at line, the other at p */
            data = hex_value(p);
            if (strcasecmp(line, "EP") == 0)
                entry_point = data;
            else
                {
                    Address addr = hex_value(line);
                    Store_Memory(addr, data);
                }
        }

    return(entry_point);
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

int get2(void)
{
    int c1 = getc(input);
    int c2 = getc(input);
    if (debug) fprintf(stderr, "read two bytes: 0x%X, 0x%X\n", c1, c2);
    if ((c1 == EOF) || (c2 == EOF))
        {
            fprintf(stderr, "Premature EOF\n");
            exit(1);
        }
    if (c1 & (~0x3F)) fprintf(stderr, "Extra high order bits for 0x%X\n", c1);
    if (c2 & (~0x3F)) fprintf(stderr, "Extra high order bits for 0x%X\n", c2);
    int n = ((c1 & 0x3F) << 6) | (c2 & 0x3F);
    return(n);
}

Address Load_Binary_Object_File(STRING name)
{
    int c1 = getc(input);
    int c2 = getc(input);
    int c3 = getc(input);
    int c4 = getc(input);
    if (debug) fprintf(stderr, "read four bytes: 0x%X, 0x%X, 0x%X, 0x%X\n", c1, c2, c3, c4);

    if ((c1 != 'O') || (c2 != 'B') || (c3 != 'J') || (c4 != '8'))
        {
            fprintf(stdout, "First four bytes are not OBJ8: ");
            fprintf(stdout, "%s", printrep(c1));
            fprintf(stdout, "%s", printrep(c2));
            fprintf(stdout, "%s", printrep(c3));
            fprintf(stdout, "%s", printrep(c4));
            fprintf(stdout, " (%02X %02X %02X %02X)\n", c1, c2, c3, c4);

            exit(1);
        }

    Address entry_point = get2();

    int n;
    while ((n = getc(input)) != EOF)
        {
            if (debug) fprintf(stderr, "Read next block of %d bytes\n", n);
            n = n - 1;
            Address addr = get2(); n -= 2;
            while (n > 0)
                {
                    Word data = get2(); n -= 2;            
                    Store_Memory(addr, data);
                    addr += 1;
                }
        }

    return(entry_point);
}

void Load_Object_File(STRING name)
{
    Address entry_point = 0;

    Clear_Memory();

    if (ASCII)
        entry_point = Load_ASCII_Object_File(name);
    else
        entry_point = Load_Binary_Object_File(name);

    time = 0;
    Halted = FALSE;
    PC = entry_point & MASK_W12;
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* constructing the opcode name for an instruction */

char opcode_name[32];

void clear_opcode(void)
{
    opcode_name[0] = '\0';
}

void append_opcode(STRING name)
{
    if (opcode_name[0] != '\0')
        strncat(opcode_name, " ", sizeof(opcode_name));
    strncat(opcode_name, name, sizeof(opcode_name));
}


char *get_opcode(void)
{
    return(opcode_name);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void Check_Overflow(void)
{
    /* check for overflow and complement L if so */
    if ((A & MASK_OVERFLOW) != 0) L = 1-L;
    A = A & MASK_W12;
}

void rotate_right(void)
{
    Word tL = ((A & 0x1) != 0);
    A = (A >> 1);
    if (L == 1) A = A | MASK_SIGN_BIT;
    L = tL;
}

void rotate_left(void)
{
    Word tL = ((A & MASK_SIGN_BIT) != 0);
    A = ((A << 1) | L) & MASK_W12;
    L = tL;
}


int Decode_Instruction(INST inst)
{
    return((inst >> 9) & 0x7);
}

Address Memory_Reference_Address(Address old_PC, INST inst)
{
    /* get the addr */
    Address addr = inst & MASK_OFFSET;
    /* check for Z/C = 1 -> current page */
    if ((inst & MASK_ZC_BIT) != 0)
        addr = addr | (old_PC & ~MASK_OFFSET);
    /* check for I/D = 1 -> indirect */
    if ((inst & MASK_I_BIT) != 0)
        {
            append_opcode("I");
            addr = Fetch_Memory(addr);
            time = time + 1;
        }
    return(addr);
}

void operate_instruction_group_1(INST inst)
{
    /* CLA */
    if (inst & MASK_CLA)
        {
            append_opcode("CLA");
            A = 0;
        }

    /* CLL */
    if (inst & MASK_CLL)
        {
            append_opcode("CLL");
            L = 0;
        }

    /* CMA */
    if (inst & MASK_CMA)
        {
            append_opcode("CMA");
            A = (~A) & MASK_W12;
        }

    /* CML */
    if (inst & MASK_CML)
        {
            append_opcode("CML");
            L = 1-L;
        }

    /* IAC */
    if (inst & MASK_IAC)
        {
            append_opcode("IAC");
            A = A+1;
            Check_Overflow();  
        }

    /* RAR and RTR */
    if (inst & MASK_RAR)
        {
            rotate_right();
            if (inst & MASK_BSW)
                {
                    append_opcode("RTR");
                    rotate_right();
                }
            else
                {
                    append_opcode("RAR");
                }
        }
                    
    /* RAL and RTL */
    if (inst & MASK_RAL)
        {
            rotate_left();
            if (inst & MASK_BSW)
                {
                    append_opcode("RTL");
                    rotate_left();
                }
            else
                {
                    append_opcode("RAL");
                }
        }
    
    if (inst & MASK_BSW & !(inst & MASK_RAR) & !(inst & MASK_RAL))
        {
            append_opcode("BSW");
            A = ((A & MASK_LOWER6) << 6) | ((A & MASK_UPPER6) >> 6);
        }
}

void operate_instruction_group_2(INST inst)
{
    Boolean skip = FALSE;
    /* SMA */
    if (inst & MASK_SMA)
        {
            append_opcode("SMA");
            skip = skip || ((A & MASK_SIGN_BIT) != 0);
        }

    /* SZA */
    if (inst & MASK_SZA)
        {
            append_opcode("SZA");
            skip = skip || (A == 0);
        }

    /* SNL */
    if (inst & MASK_SNL)
        {
            append_opcode("SNL");
            skip = skip || (L != 0);
        }

    /* RSS */
    if (inst & MASK_RSS)
        {
            append_opcode("RSS");
            skip = !skip;
        }

    if (skip) PC = (PC + 1) & MASK_W12;
    
    /* CLA */
    if (inst & MASK_CLA)
        {
            append_opcode("CLA");
            A = 0;
        }
    
    /* OSR */
    if (inst & MASK_OSR)
        {
            append_opcode("OSR");
            A = A | Switch_Register;
        }
    
    /* HLT */
    if (inst & MASK_HLT)
        {
            append_opcode("HLT");
            Halted = TRUE;
        }
}


void Execute(Address old_PC, int opcode, INST inst)
{
    Address addr;
    Word value;

    /* zero the opcode name */
    clear_opcode();

    switch(opcode)
        {
        case 0: /* AND -- Logical AND     */
            append_opcode("AND");
            addr = Memory_Reference_Address(old_PC, inst);
            value = Fetch_Memory(addr);
            A = A & value;
            time = time + 2;
            break;
            
        case 1: /* TAD -- Two's complement add    */
            append_opcode("TAD");
            addr = Memory_Reference_Address(old_PC, inst);
            value = Fetch_Memory(addr);
            A = A + value;
            Check_Overflow(); 
            time = time + 2;
            break;
            
        case 2: /* ISZ -- Increment and skip if zero      */
            append_opcode("ISZ");
            addr = Memory_Reference_Address(old_PC, inst);
            value = Fetch_Memory(addr);
            value = (value + 1) & MASK_W12;
            Store_Memory(addr, value);
            if (value == 0) PC = (PC + 1) & MASK_W12;
            time = time + 2;
            break;
            
        case 3: /* DCA -- Deposit and clear accumulator   */
            append_opcode("DCA");
            addr = Memory_Reference_Address(old_PC, inst);
            Store_Memory(addr, A);
            A = 0;
            time = time + 2;
            break;
            
        case 4: /* JMS -- Jump to subroutine      */
            append_opcode("JMS");
            addr = Memory_Reference_Address(old_PC, inst);
            Store_Memory(addr, PC);
            PC = (addr + 1) & MASK_W12;
            time = time + 2;
            break;
            
        case 5: /* JMP -- Jump    */
            append_opcode("JMP");
            addr = Memory_Reference_Address(old_PC, inst);
            PC = addr;
            time = time + 1;
            break;
            
        case 6: /* IOT -- Input/Output Transfer  */
            {
                int device = (inst & MASK_IOT_DEVICE) >> 3;
                int function = (inst & MASK_IOT_FUNCTION);
                /* check for device = 3 -- Input */
                if (device == 3)
                    {
                        append_opcode("IOT 3");
                        A = getc(stdin) & MASK_W12;
                    }
                /* or device = 4 -- Output */
                else if (device == 4)
                    {
                        append_opcode("IOT 4");
                        putc((A & 0xFF), stdout); 
                    }
                else
                    {
                        append_opcode("IOT <bad-device>");
                        fprintf(stderr, "IOT function %d to unknown device %d; halting\n", function, device);
                        Halted = TRUE;
                    }
            
                time = time + 1;
                break;
            }
            
        case 7: /* Operate microcoded instructions */
            time = time + 1;
            if ((inst & 0x100) == 0)
                operate_instruction_group_1(inst);
            else
                operate_instruction_group_2(inst);
            break;
            
        }

    if (verbose)
    fprintf(stderr, "Time %lld: PC=0x%03X instruction = 0x%03X (%s), rA = 0x%03X, rL = %d\n", 
            time, old_PC, inst, get_opcode(), A, L);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void Interpreter(STRING name)
{
    Load_Object_File(name);

    while (!Halted)
        {
            Address old_PC = PC;
            INST inst = Fetch_Memory(PC);
            PC = (PC + 1) & MASK_W12;
            int opcode = Decode_Instruction(inst);
            Execute(old_PC, opcode, inst);
        }
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void scanargs(STRING s)
{
    /* check each character of the option list for
       its meaning. */

    while (*++s != '\0')
        switch (*s)
            {

            case 'D': /* debug option */
                debug = TRUE;

            case 'b': /* binary object file input */
                ASCII = FALSE;
                break;

            case 'a': /* ASCII object file input */
                ASCII = TRUE;
                break;

            case 'v': /* verbose option */
                verbose = !verbose;
                break;

            case 's': /* switch register setting */
            case 'S': /* switch register setting */
                Switch_Register = hex_value(&s[1]) & MASK_W12;
                if (debug) fprintf(stderr, "Switch Register is 0x%03X\n", Switch_Register);
                break;

            default:
                fprintf (stderr,"pdp8: Bad option %c\n", *s);
                fprintf (stderr,"usage: pdp8 [-D] file\n");
                exit(1);
            }
}



/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int main(int argc, STRING *argv)
{
    Boolean filenamenotgiven = TRUE;

    /* main driver program.  Define the input file
       from either standard input or a name on the
       command line.  Process all arguments. */

    while (argc > 1)
        {
            argc--, argv++;
            if (**argv == '-')
                scanargs(*argv);
            else
                {
                    filenamenotgiven = FALSE;
                    input = fopen(*argv,"r");
                    if (input == NULL)
                        {
                            fprintf (stderr, "Can't open %s\n",*argv);
                        }
                    else
                        {
                            Interpreter(*argv);
                            fclose(input);
                        }
                }
        }

    if (filenamenotgiven)
        {
            input = stdin;
            Interpreter(NULL);
        }

    exit(0);
}


