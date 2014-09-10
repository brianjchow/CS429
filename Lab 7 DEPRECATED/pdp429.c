/*
 ============================================================================
 Name        : Brian Chow (bc23784)
 Program	 : pdp429.c
 Description : see readme
 ============================================================================
 */

/*
 NOTES

	- use pages of 256 words (65,536 16-bit words of memory possible)
	- assume each instruction takes 1 cycle for every memory reference (including instruction fetch), but no additional time for any computation
	- the link bit is set whenever an arithmetic operation causes an overflow (add/sub/incr/decr/mul) or divide by zero
	- anything in parentheses below means contents of whatever is in the parentheses

 	 STACK
 	 	 - the stack represents an area of memory (words[]) that will be used as a stack
 	 	 - the stack grows down in memory (larger-numbered memory addresses are on top)
 	 	 - the stack pointer initially points to an empty location; the first push will set the stack pointer
 	 	 - the stack pointer represents the beginning of the stack
 	 	 - the stack pointer limit represents the end of the stack
 	 	 - pushing to the stack stores in the word that the SP points to; SP is then decremented by one
 	 	 - popping from the stack first increments the SP by one, and then uses the word that the SP points at
 	 	 - errors
 	 	 	 - stack overflow: if SP < SPL when stack push starts
 	 	 	 - stack underflow: if SP wraps around to 0 (SP == 0xffff when a stack pop starts)

 	 INSTRUCTIONS
 	 	 - 6 classes of instructions

 	 	 	 - non-register, non-memory instructions
 	 	 	 	 - high-order 6 bits are ALWAYS 0 (000000)
 	 	 	 	 - 000000.0000000000 == NOP
 	 	 	 	 - 000000.0000000001 == HLT; low-order bit of the PSW is set to 0; halt
 	 	 	 	 - 000000.0000000010 == RET; pop the stack into the PC

 	 	 	 - register memory reference instructions
 	 	 	 	 - 4-bit opcode, 2-bit register selector, D/I bit, Z/C bit, 8-bit page offset (in that order, HOB to LOB)
 	 	 	 	 - opcodes
 	 	 	 	 	 - 0001 == ADD: (register selected) + (memory operand) => (register selected)
 	 	 	 	 	 - 0010 == SUB: (register selected) - (memory operand) => (register selected)
 	 	 	 	 	 - 0011 == MUL: (register selected) * (memory operand) => (register selected)
 	 	 	 	 	 - 0100 == DIV: (register selected) / (memory operand) => (register selected)
 	 	 	 	 	 - 0101 == AND: (register selected) & (memory operand) => (register selected)
 	 	 	 	 	 - 0110 == OR:  (register selected) | (memory operand) => (register selected)
 	 	 	 	 	 - 0111 == XOR: (register selected) ^ (memory operand) => (register selected)
 	 	 	 	 	 - 1000 == LD:  (memory operand) => (register selected); overwrite contents of register selected with contents of memory operand
 	 	 	 	 	 - 1001 == ST:  (register selected) => (memory operand); overwrite contents of memory operand with contents of register selected
 	 	 	 	 - register selector
 	 	 	 	 	 - 00 == A register
 	 	 	 	 	 - 01 == B register
 	 	 	 	 	 - 10 == C register
 	 	 	 	 	 - 11 == D register

 	 	 	 - i/o transfer instructions
 	 	 	 	 - 4-bit opcode of 1010, 2-bit register selector, 7-bit device number, 3-bit function field (in that order, HOB to LOB)

 	 	 	 - non-register memory reference instructions
 	 	 	 	 - 6-bit opcode, D/I bit, Z/C bit, 8-bit page offset (in that order, HOB to LOB)
 	 	 	 	 - 6-bit opcode is actually the 4-bit opcode and 2-bit register fields of the register memory reference instructions combined
 	 	 	 	 - opcodes
 	 	 	 	 	 - 1011.00 == ISZ: (memory operand) + 1 => (memory operand); if (memory operand) == 0, PC++
 	 	 	 	 	 - 1011.01 == JMP: address of memory operand => PC
 	 	 	 	 	 - 1011.10 == CALL: push return address (== PC + 1) on stack; address of memory operand => PC
 	 	 	 	 	 - 1100.00 == PUSH: push address of memory operand to the stack
 	 	 	 	 	 - 1100.01 == POP: pop top of stack => (memory operand)

 	 	 	 - register-to-register instructions
 	 	 	 	 - operate ONLY on registers
 	 	 	 	 - 4-bit opcode of 1110, 3-bit sub-opcode, three 3-bit register specifiers (i, j, and k, respectively) (in that order, HOB to LOB)
 	 	 	 	 - instructions roughly take the format (jReg [op] kReg => iReg)
 	 	 	 	 - register specifiers
 	 	 	 	 	 - 000 == A register
 	 	 	 	 	 - 001 == B register
 	 	 	 	 	 - 010 == C register
 	 	 	 	 	 - 011 == D register
 	 	 	 	 	 - 100 == PC
 	 	 	 	 	 - 101 == PSW
 	 	 	 	 	 - 110 == SP
 	 	 	 	 	 - 111 == SPL
 	 	 	 	 - instructions
 	 	 	 	 	 - 1110.000 == MOD: (reg[j]) % (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.001 == ADD: (reg[j]) + (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.010 == SUB: (reg[j]) - (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.011 == MUL: (reg[j]) * (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.100 == DIV: (reg[j]) / (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.101 == AND: (reg[j]) & (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.110 == OR:  (reg[j]) | (reg[k]) => (reg[i])
 	 	 	 	 	 - 1110.111 == XOR: (reg[j]) ^ (reg[k]) => (reg[i])

 	 	 	 - non-memory register instructions
 	 	 	 	 - similar to microinstructions on the PDP-8
 	 	 	 	 - 4-bit opcode of 1111, 2-bit register selector, SM, SZ, SNL, RSS, CL, CLL, CM, CML, DC, IC (all one bit) (in that order, HOB to LOB)
 	 	 	 	 - instructions
 	 	 	 	 	 - SM == skip if (register) is negative (sign bit is 1)
 	 	 	 	 	 - SZ == skip if (register) is zero
 	 	 	 	 	 - SNL == skip if link bit is non-zero
 	 	 	 	 	 - RSS == reverse skip sense; flip skip flag bit
 	 	 	 	 	 - CL == clear (register)
 	 	 	 	 	 - CLL == clear link bit
 	 	 	 	 	 - CM == complement (register)
 	 	 	 	 	 - CML == complement link bit
 	 	 	 	 	 - DC == decrement (register) by one
 	 	 	 	 	 - IC == increment (register) by one
 	 	 	 	 - *** EVALUATION OF INSTRUCTIONS OCCURS IN THE ORDER LISTED ABOVE ***
 	 	 -

 	 ERRORS TO HANDLE
 	 	 - object file
 	 	 	 - wrong magic number; first 4 bytes are not 'OBJG'; handled
 	 	 	 - EOF when more data is expected
 	 	 	 - byte count is even, or too small (1, 3)
 	 	 	 - block address plus number of bytes to load exceeds size of memory
 	 	 -

FIRST LINE IN PRIME.OBJ
O.B.J.G||0.4||131||0.0|0.40|0.1|0
- read in OBJG magic "number"
- next two bytes store the 16-bit starting address for the object file
	- so 00000000.00000100 == start at address 0x04 == EP
- every two numbers after that represents a block
	- each block is a set of 2 bytes
	- the first byte (here, 131) is the number of bytes in the entire block, INCLUDING the first byte
	- after this first byte will be an address (here, 0.0)
	- starting at that address, store the blocks, continuing word after word, until we have exhausted the number of BYTES given at the beginning of the block
		- so, store (0.40), then (0.1), etc

 */

/*
 	 BUG FIXES
 	 	 - all number format conversion methods will accept a number that overflows the bounds set by 16-bit word lengths
 	 	 - replace hard-coded constants in main instruction handlers with their respective global constants
 	 	 - use pointer to pointer in instructions with a specified register to reduce redundant code
 	 	 - for all instructions taking in int n as a parameter, delete parameter and use global instr[] array instead (reduce redundant code)
 	 	 - link bit is being flipped whenever overflow occurs; should just be set
 	 	 - handle link bit when divide by 0 and subtraction underflow occurs
 	 	 - no over/underflow/divide-by-zero protection in all rtri instruction methods
 	 	 - return in non_register_non_memory_instruction() currently does nothing
 	 	 - call(), push(), and pop() currently do nothing and/or are probably incorrect
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef short boolean;
#define TRUE 1
#define FALSE 0
#define ASCII_ZERO 48
#define ASCII_ONE 49
#define SIZEOF_PAGE 256
#define MAX_NUM_ELEMENTS 65536	// 2^16 = 65536
#define REG_LEN 16
#define DIRECT 0
#define INDIRECT 1
#define ZERO_PAGE 0
#define CURRENT_PAGE 1

#define NRNMI 0								// non-register non-memory instructions
#define RMRI_LOWER_BOUND 1					// register memory reference instructions
#define RMRI_UPPER_BOUND 9
#define IO 10								// input/output instructions
#define NRMRI_LOWER_BOUND 11				// non-register memory instructions
#define NRMRI_UPPER_BOUND 12
#define RTRI 14								// register-to-register instructions
#define NMRI 15								// non-memory register instructions

#define RMRI_ADD 1
#define RMRI_SUB 2
#define RMRI_MUL 3
#define RMRI_DIV 4
#define RMRI_AND 5
#define RMRI_OR  6
#define RMRI_XOR 7
#define RMRI_LOAD 8
#define RMRI_STORE 9

#define RTRI_MOD 0
#define RTRI_ADD 1
#define RTRI_SUB 2
#define RTRI_MUL 3
#define RTRI_DIV 4
#define RTRI_AND 5
#define RTRI_OR  6
#define RTRI_XOR 7

#define A_REG_SPECIFIER 0
#define B_REG_SPECIFIER 1
#define C_REG_SPECIFIER 2
#define D_REG_SPECIFIER 3
#define PC_SPECIFIER 4
#define PSW_SPECIFIER 5
#define SP_SPECIFIER 6
#define SPL_SPECIFIER 7

//int pcStart = -1;
int currPC = -1;
char *aReg;
char *bReg;
char *cReg;
char *dReg;
char link;
int *words[MAX_NUM_ELEMENTS];
char *instr;
char skipFlag;

int sp;										// stack pointer
int spl;									// stack pointer limit
int psw;									// processor status word (indicates if running or halted)

FILE *file;									// for "reading in" the initial machine state
size_t buflen = 20;							// size of buffer during file read-in
signed long long cycleCount = 0;			// for use with verbose; indicates how long (in cycles) an instruction took
signed long long instrCount = 1;			// primarily for use with debugging when verbose == TRUE

boolean debug = TRUE;
boolean readFileDebug = TRUE;
boolean manDebug = TRUE;					// for command line-less testing
boolean verbose = TRUE;

// ------------------------------------------------
// begin procedures for handling command line input

	typedef char *STRING;

	#define CAST(t,e) ((t)(e))
	#define TYPED_MALLOC(t) CAST(t*, malloc(sizeof(t)))
	#define LINE_WIDTH 72

	/* MALLOC space for a string and copy it */

	STRING remember_string(const STRING name) {
		size_t n;
		STRING p;

		if (name == NULL) return(NULL);

		/* get memory to remember file name */
		n = strlen(name) + 1;
		p = CAST(STRING, malloc(n));
		strcpy(p, name);
		return(p);
	}

	void scanArgs(STRING s) {
		/* check each character of the option list for
		   its meaning. */

		while (*++s != '\0') {
			switch (*s) {

				case 'v': /* debug option */
					verbose = TRUE;
					break;

				case 'V': // debug option 2
					verbose = TRUE;
					break;

				default:
					fprintf (stderr,"PROGR: Bad option %c\n", *s);
					exit(-1);
			}
		}
	}

// end procedures for handling command line input
// ----------------------------------------------

// -------------------------------------------------
// begin miscellaneous procedures for main procedure

	void freeWords() {
//		int i;
//		for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
//			if (words[i] != NULL) {
//				free(words[i]);
//			}
//		}
	}

	void die(char *reason) {
		fprintf(stderr, "*** %s ***\n\n", reason);
		freeWords();
		exit(-1);
	}

	void hlt() {
		if (debug) { printf("Instruction was HLT; now halting/exiting"); }
		//psw[REG_LEN] = ASCII_ZERO;
		freeWords();
		exit(0);
	}

	char* decToHex(int n) {									// will attempt to convert even if n is out of bounds (16 bits)
		if (n == 0) {
			char *out = calloc(5, sizeof(char));
			out[0] = ASCII_ZERO;
			out[1] = ASCII_ZERO;
			out[2] = ASCII_ZERO;
			out[3] = ASCII_ZERO;
			out[4] = '\0';
			return out;
		}
		else {
			int i, j, r[REG_LEN];
			i = 0;
			j = 0;
			while (n > 0) {
				r[i] = n % REG_LEN;
				n /= REG_LEN;
				i++;
				j++;
			}
			char *out = calloc(j + 1, sizeof(char));
			j = 0;

			for (i--; i >= 0; i--) {
				if (r[i] == 10) { 			out[j] = 'A';		}
				else if (r[i] == 11) {		out[j] = 'B';		}
				else if (r[i] == 12) {		out[j] = 'C';		}
				else if (r[i] == 13) {		out[j] = 'D';		}
				else if (r[i] == 14) {		out[j] = 'E';		}
				else if (r[i] == 15) {		out[j] = 'F';		}
				else {
					out[j] = (char) (r[i] + ASCII_ZERO);
				}
				j++;
			}
			out[j] = '\0';

			if (strlen(out) == 1) {
				char *temp = calloc(5, sizeof(char));
				temp[0] = ASCII_ZERO;
				temp[1] = ASCII_ZERO;
				temp[2] = ASCII_ZERO;
				temp[3] = out[0];
				temp[4] = '\0';
				free(out);
				return temp;
			}
			else if (strlen(out) == 2) {
				char *temp = calloc(5, sizeof(char));
				temp[0] = ASCII_ZERO;
				temp[1] = ASCII_ZERO;
				temp[2] = out[0];
				temp[3] = out[1];
				temp[4] = '\0';
				free(out);
				return temp;
			}
			else if (strlen(out) == 3) {
				char *temp = calloc(5, sizeof(char));
				temp[0] = ASCII_ZERO;
				temp[1] = out[0];
				temp[2] = out[1];
				temp[3] = out[2];
				temp[4] = '\0';
				free(out);
				return temp;
			}
			return out;
		}
	}

	int binToDec(char *binString) {

		char *temp = binString;
		int num = 0;
		do {
			int b = *temp == '1' ? 1 : 0;
			num = (num << 1) | b;
			temp++;
		}
		while (*temp);
		return num;
	}

	char* hexToBin(char *hexString) {
		int len = strlen(hexString);
		char *out = calloc((len * 4 + 1), sizeof(char));

		int i;
		int outCounter = 0;
		for (i = 0; i < len; i++) {
			char temp = hexString[i];
			if (temp == '0') {
				out[outCounter] = '0';	out[outCounter + 1] = '0';	out[outCounter + 2] = '0';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == '1') {
				out[outCounter] = '0';	out[outCounter + 1] = '0';	out[outCounter + 2] = '0';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == '2') {
				out[outCounter] = '0';	out[outCounter + 1] = '0';	out[outCounter + 2] = '1';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == '3') {
				out[outCounter] = '0';	out[outCounter + 1] = '0';	out[outCounter + 2] = '1';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == '4') {
				out[outCounter] = '0';	out[outCounter + 1] = '1';	out[outCounter + 2] = '0';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == '5') {
				out[outCounter] = '0';	out[outCounter + 1] = '1';	out[outCounter + 2] = '0';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == '6') {
				out[outCounter] = '0';	out[outCounter + 1] = '1';	out[outCounter + 2] = '1';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == '7') {
				out[outCounter] = '0';	out[outCounter + 1] = '1';	out[outCounter + 2] = '1';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == '8') {
				out[outCounter] = '1';	out[outCounter + 1] = '0';	out[outCounter + 2] = '0';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == '9') {
				out[outCounter] = '1';	out[outCounter + 1] = '0';	out[outCounter + 2] = '0';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == 'A' || temp == 'a') {
				out[outCounter] = '1';	out[outCounter + 1] = '0';	out[outCounter + 2] = '1';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == 'B' || temp == 'b') {
				out[outCounter] = '1';	out[outCounter + 1] = '0';	out[outCounter + 2] = '1';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == 'C' || temp == 'c') {
				out[outCounter] = '1';	out[outCounter + 1] = '1';	out[outCounter + 2] = '0';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == 'D' || temp == 'd') {
				out[outCounter] = '1';	out[outCounter + 1] = '1';	out[outCounter + 2] = '0';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else if (temp == 'E' || temp == 'e') {
				out[outCounter] = '1';	out[outCounter + 1] = '1';	out[outCounter + 2] = '1';	out[outCounter + 3] = '0';	outCounter += 4;
			}
			else if (temp == 'F' || temp == 'f') {
				out[outCounter] = '1';	out[outCounter + 1] = '1';	out[outCounter + 2] = '1';	out[outCounter + 3] = '1';	outCounter += 4;
			}
			else {
				die("Invalid character encountered in hexToBin()");
			}
		}
		out[len * 4] = '\0';

		if (strlen(out) < REG_LEN) {
			char *temp = calloc((REG_LEN + 1), sizeof(char));
			temp[0] = '\0';
			int padding = REG_LEN - strlen(out);
			for (i = 0; i < padding; i++) {
				strcat(temp, "0");
			}
			int start = 0;
			for (; i < REG_LEN; i++) {
				temp[i] = out[start];
				start++;
			}
			temp[REG_LEN] = '\0';
			free(out);
			return temp;
		}
		return out;
	}

	int hexToDec(char *hexString) {
		char *binString = hexToBin(hexString);
		int num = binToDec(binString);
		free(binString);
		return num;
	}

	char* decToBin(int n) {
		char *temp1 = decToHex(n);
		char *temp2 = hexToBin(temp1);
		if (strlen(temp2) < REG_LEN) {
			char* temp3 = calloc((REG_LEN + 1), sizeof(char));
			temp3[0] = '\0';
			int padding = REG_LEN - strlen(temp2);
			int i;
			for (i = 0; i < padding; i++) {
				strcat(temp3, "0");
			}
			int start = 0;
			for (; i < REG_LEN; i++) {
				temp3[i] = temp2[start];
				start++;
			}
			temp3[REG_LEN] = '\0';
			free(temp1);
			free(temp2);
			return temp3;
		}
		else {
			free(temp1);
			return temp2;
		}
	}

	char *mergeBytes(int i, int j) {
		char *out = calloc((REG_LEN + 1), sizeof(int));
		char *high8 = decToBin(i);
		char *low8 = decToBin(j);
		int counter;
		for (counter = 0; counter < 8; counter++) {
			out[counter] = high8[(counter + 8)];
		}
		for (; counter < REG_LEN; counter++) {
			out[counter] = low8[counter];
		}
		out[(REG_LEN + 1)] = '\0';
		free(high8);
		free(low8);
		return out;
	}

	void toString() {
		int temp = 0;
		for (; temp < MAX_NUM_ELEMENTS; temp++) {
			if (words[temp] != -1) {
				if (temp % 20 == 0 && temp != 0) {
					printf("\n\t");
				}
				printf("(%d, %d) ", temp, words[temp]);
			}
		}
		printf("]");
	}

	void printVerbose() {
		// time (in cycles) after each instruction is executed, instruction's PC, instruction's hex representation, instruction's symbolic opcode(s), and
		// 	names and values of all memory and registers that are referenced by the execution of the instruction
		// ex.: "Time 1684: PC=0x011B instruction = 0xB515 (JMP): 0x0115 -> PC"

		// fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)", time, old_PC, inst, get_opcode_names());

	}

// end miscellaneous procedures for main procedure
// -----------------------------------------------

	void incrCurrPC() {
		if (currPC == (MAX_NUM_ELEMENTS - 2)) {
			if (skipFlag == ASCII_ONE) {
				currPC = 0;
			}
			else {
				currPC++;
			}
		}
		else if (currPC == (MAX_NUM_ELEMENTS - 1)) {
			if (skipFlag == ASCII_ONE) {
				currPC = 1;
			}
			else {
				currPC = 0;
			}
		}
		else {
			if (skipFlag == ASCII_ONE) {
				currPC += 2;
			}
			else {
				currPC++;
			}
		}
		skipFlag = ASCII_ZERO;
	}

	// non-register non-memory instructions are handled below

	void rmri_add(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) + (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int addedInt = wordInt + regInt;
		char *added;
		if (addedInt > (MAX_NUM_ELEMENTS - 1)) {
			//added = decToBin(addedInt % MAX_NUM_ELEMENTS);
			addedInt &= 0xffff;
			link = ASCII_ONE;
//			if (link == ASCII_ZERO) {
//				link = ASCII_ONE;
//			}
//			else {
//				link = ASCII_ZERO;
//			}
		}
		added = decToBin(addedInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = added[i];
		}
		free(added);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_sub(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) - (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int subbedInt = regInt - wordInt;
		if (subbedInt < 0) {
			subbedInt &= 0xffff;
			link = ASCII_ONE;
		}
//		if (wordInt <= regInt) {
//			subbedInt = regInt - wordInt;
//		}
//		else {
//			subbedInt = 0;
//			link = ASCII_ONE;
//		}
		char *subbed;
		subbed = decToBin(subbedInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = subbed[i];
		}
		free(subbed);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_mul(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) * (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int multInt = wordInt * regInt;
		char *mult;
		if (multInt > (MAX_NUM_ELEMENTS - 1)) {
			//mult = decToBin(multInt % MAX_NUM_ELEMENTS);
			multInt &= 0xffff;
			link = ASCII_ONE;
//			if (link == ASCII_ZERO) {
//				link = ASCII_ONE;
//			}
//			else {
//				link = ASCII_ZERO;
//			}
		}
		mult = decToBin(multInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = mult[i];
		}
		free(mult);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_div(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) / (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		if (wordInt == 0) {
			link = ASCII_ONE;
		}
		else {
			int divInt;
			if (wordInt == 0) {
				divInt = 0;
				link = ASCII_ONE;
			}
			else {
				divInt = regInt / wordInt;
			}
			char *div = decToBin(divInt);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				(*(*regSpecified))[i] = div[i];
			}
			free(div);
		}
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_and(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) & (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int andInt = regInt & wordInt;
		char *and = decToBin(andInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = and[i];
		}
		free(and);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_or(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) | (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int orInt = regInt | wordInt;
		char *or = decToBin(orInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = or[i];
		}
		free(or);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_xor(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) ^ (memory operand) => (register selected)
		int regInt = binToDec((*(*regSpecified)));
		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		int xorInt = regInt ^ wordInt;
		char *xor = decToBin(xorInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSpecified))[i] = xor[i];
		}
		free(xor);
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void rmri_load(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (memory operand) => (register selected); overwrite contents of register selected with contents of memory operand

		if (debug) { printf("Instruction was rmri_load() (0x%04X)\nAddressing mode: %d\nPage address: %d\nRegister before load: %s\n", binToDec(instr), addressingMode, pageAddr, (*(*regSpecified))); }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = (words[pageAddr]);
			}
			else {
				int effAddrInt = (words[pageAddr]);
				if (words[effAddrInt] != -1) {
					wordInt = (words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}
		char *temp = decToBin(wordInt);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			//printf("Iteration %d: store %c\n", i, temp[i]);
			(*(*regSpecified))[i] = temp[i];
		}
		free(temp);
		(*(*regSpecified))[REG_LEN] = '\0';

		if (debug) { printf("Register after load: %s\nPC is currently 0x%04X/%d\n\n", (*(*regSpecified)), currPC, currPC); }

		incrCurrPC();
	}

	void rmri_store(char ***regSpecified, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) => (memory operand); overwrite contents of memory operand with contents of register selected
		int regInt = binToDec((*(*regSpecified)));
		if (addressingMode == DIRECT) {
			words[pageAddr] = regInt;
		}
		else {
			int effAddrInt = words[pageAddr];
			words[effAddrInt] = regInt;
		}
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void io(char ***regSpecified, int deviceNum, int functionCode) {
		// ignore function code
		// if device is 3
			// do a getchar() to read a byte from stdin
			// put that byte in the register specified in the IOT instruction
			// an EOF on stdin should cause the register to be set to 0xffff
		// if device is 4
			// take low-order 8 bits from the specified register
			// output it as a byte to stdout (putchar())
		// ignore all other devices
		if (deviceNum == 3) {
			int c = getchar();
//			int i;
//			for (i = 0; i < REG_LEN; i++) {
//				(*(*regSpecified))[i] = ASCII_ZERO;
//			}
			if (c > (MAX_NUM_ELEMENTS - 1)) {
				c &= (MAX_NUM_ELEMENTS - 1);
			}
			if (c < 0) {
				c = (MAX_NUM_ELEMENTS - 1);
			}
			char *temp = decToBin(c);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				(*(*regSpecified))[i] = temp[i];
			}
			free(temp);
		}
		if (deviceNum == 4) {
			char *temp = calloc(10, sizeof(char));
			strncpy(temp, (*(*regSpecified)) + 7, 10);
			int tempInt = binToDec(temp);
			free(temp);
			putchar(tempInt);
		}
		(*(*regSpecified))[REG_LEN] = '\0';

		incrCurrPC();
	}

	void isz(int addressingMode, int pageMode, int pageAddr) {
		// increment and skip if zero
		// contents of the effective address are incremented by one and put back in the same memory location
		// if result of increment is zero, the next instruction is skipped

		if (addressingMode == DIRECT) {
			if (words[pageAddr] == -1) {
				// do something?
			}
			else {
				int temp = words[pageAddr];
				if (temp == (MAX_NUM_ELEMENTS - 1)) {
					temp = 0;
					skipFlag = ASCII_ONE;
				}
				else {
					temp++;
				}
				words[pageAddr] = temp;
			}
		}
		else {
			// indirect addressing
			if (words[pageAddr] == -1) {
				// do something?
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] == -1) {
					// do something?
				}
				else {
					int temp = words[effAddrInt];
					if (temp == (MAX_NUM_ELEMENTS - 1)) {
						temp = 0;
						skipFlag = ASCII_ONE;
					}
					else {
						temp++;
					}
					words[effAddrInt] = temp;
				}
			}
		}
		incrCurrPC();
	}

	void jmp(int addressingMode, int pageMode, int pageAddr) {
		// set program counter to effective address
		if (addressingMode == DIRECT) {
			currPC = pageAddr;
		}
		else {
			int effAddrInt = words[pageAddr];
			currPC = effAddrInt;
		}
		if (currPC > (MAX_NUM_ELEMENTS - 1) || currPC < 0) {
			die("Fatal error: PC out of bounds");
		}
	}

	void call(int addressingMode, int pageMode, int pageAddr) {
		// push return address (PC + 1) onto stack; address of memory operand => PC
		int addr;
		if (addressingMode == DIRECT) {
			addr = pageAddr;
		}
		else {
			addr = words[pageAddr];
		}
		currPC = addr;
		//incrCurrPC();
	}

	void push(int addressingMode, int pageMode, int pageAddr) {
		// push address of memory operand to the stack; decrement SP
		// "set whatever the sp is pointing to to the value given by the instruction; then decrement sp"

		if (debug) { printf("Instruction was push\nPage address is 0x%04X/%d\nStack pointer before push is %d\n", pageAddr, pageAddr, sp); }

		int pageAddrCopy = pageAddr;
		if (sp == -1) {
			// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		}
		else {
			pageAddrCopy--;
			words[pageAddr] = pageAddrCopy;
			words[sp] = pageAddrCopy;
			sp--;
		}
//		else {
//			int addr;
//			if (addressingMode == DIRECT) {
//				addr = pageAddr;
//			}
//			else {
//				addr = words[pageAddr];
//			}
//			words[sp] = addr;
//			sp--;
//		}

		if (debug) { printf("Instruction was push\nStack pointer after push is %d\n\n", sp); }

		//incrCurrPC();
	}

	void pop(int addressingMode, int pageMode, int pageAddr) {
		// pop top of stack => memory operand; increment SP
		sp++;
		int temp = words[sp];
		if (addressingMode == DIRECT) {
			words[pageAddr] = temp;
		}
		else {
			int effAddrInt = words[pageAddr];
			words[effAddrInt] = temp;
		}
		//incrCurrPC();
	}

	int blah(int regSpecifier) {
		int out;
		if (regSpecifier == A_REG_SPECIFIER) {
			return (binToDec(aReg));
		}
		else if (regSpecifier == B_REG_SPECIFIER) {
			return (binToDec(bReg));
		}
		else if (regSpecifier == C_REG_SPECIFIER) {
			return (binToDec(cReg));
		}
		else if (regSpecifier == D_REG_SPECIFIER) {
			return (binToDec(dReg));
		}
		else if (regSpecifier == 4) {
			// PC
			out = currPC;
			return out;
		}
		else if (regSpecifier == 5) {
			// PSW
			out = psw;
			return out;
		}
		else if (regSpecifier == 6) {
			// SP
			out = sp;
			return out;
		}
		else {
			// SPL
			out = spl;
			return out;
		}
	}

	void rtri_store(int result, int regSpecifier1) {
		if (regSpecifier1 == A_REG_SPECIFIER) {
			char *temp = decToBin(result);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				aReg[i] = temp[i];
			}
			free(temp);
		}
		else if (regSpecifier1 == B_REG_SPECIFIER) {
			char *temp = decToBin(result);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				bReg[i] = temp[i];
			}
			free(temp);
		}
		else if (regSpecifier1 == C_REG_SPECIFIER) {
			char *temp = decToBin(result);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				cReg[i] = temp[i];
			}
			free(temp);
		}
		else if (regSpecifier1 == D_REG_SPECIFIER) {
			char *temp = decToBin(result);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				dReg[i] = temp[i];
			}
			free(temp);
		}
		else if (regSpecifier1 == 4) {
			// PC
			currPC = result;
		}
		else if (regSpecifier1 == 5) {
			// PSW
			psw = result;
		}
		else if (regSpecifier1 == 6) {
			// SP
			sp = result;
		}
		else {
			// SPL
			spl = result;
		}
	}

	void rtri_mod(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) % (blah(regSpecifier3));
		rtri_store(result, regSpecifier1);
	}

	void rtri_add(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) + (blah(regSpecifier3));
		if (result > (MAX_NUM_ELEMENTS - 1)) {
			result &= MAX_NUM_ELEMENTS - 1;
			link = ASCII_ONE;
		}
		rtri_store(result, regSpecifier1);
	}

	void rtri_sub(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) - (blah(regSpecifier3));
		if (result < 0) {
			//result = 0;
			result &= 0xffff;
			link = ASCII_ONE;
		}
		rtri_store(result, regSpecifier1);
	}

	void rtri_mul(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) * (blah(regSpecifier3));
		if (result > (MAX_NUM_ELEMENTS - 1)) {
			result &= MAX_NUM_ELEMENTS - 1;
			link = ASCII_ONE;
		}
		rtri_store(result, regSpecifier1);
	}

	void rtri_div(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result;
		if (blah(regSpecifier3) == 0) {
			result = 0;
			link = ASCII_ONE;
		}
		else {
			result = (blah(regSpecifier2)) / (blah(regSpecifier3));
		}
		rtri_store(result, regSpecifier1);
	}

	void rtri_and(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) & (blah(regSpecifier3));
		int i;
		for (i = 0; i < REG_LEN; i++) {
			printf("Iteration %d: %c\n", i, aReg[i]);
		}
		if (debug) { printf("Instruction was rtri_and()\nStoring results of register IDs (%d & %d) into register ID %d\nResult was %d\nPC is currently 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, currPC, currPC); }
		rtri_store(result, regSpecifier1);
	}

	void rtri_or(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		if (debug) { printf("Instruction was rtri_or()\nStoring results of register IDs (%d | %d) into register ID %d\nPC is currently 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, currPC, currPC); }
		int result = (blah(regSpecifier2)) | (blah(regSpecifier3));
		rtri_store(result, regSpecifier1);
	}

	void rtri_xor(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int result = (blah(regSpecifier2)) ^ (blah(regSpecifier3));
		rtri_store(result, regSpecifier1);
	}

	void sm(char ***regSelected) {
		// skip if (register) is negative (sign bit is 1)
		if ((*(*regSelected))[0] == ASCII_ONE) {
			skipFlag = ASCII_ONE;
		}
	}

	void sz(char ***regSelected) {
		// skip if (register) is 0
		if (binToDec(*(*regSelected)) == 0) {
			skipFlag = ASCII_ONE;
		}
	}

	void snl() {
		// skip if link bit is non-zero
		if (link == ASCII_ONE) {
			skipFlag = ASCII_ONE;
		}
	}

	void rss() {
		// reverse skip sense
		if (skipFlag == ASCII_ZERO) {
			skipFlag = ASCII_ONE;
		}
		else {
			skipFlag = ASCII_ZERO;
		}
	}

	void cl(char ***regSelected) {
		// clear (register)
		int i;
		for (i = 0; i < REG_LEN; i++) {
			(*(*regSelected))[i] = ASCII_ZERO;
		}
	}

	void cll() {
		// clear link bit
		link = ASCII_ZERO;
	}

	void cm(char ***regSelected) {
		// complement register
		int i;
		for (i = 0; i < REG_LEN; i++) {
			if ((*(*regSelected))[i] == ASCII_ZERO) {
				(*(*regSelected))[i] = ASCII_ONE;
			}
			else {
				(*(*regSelected))[i] = ASCII_ZERO;
			}
		}
	}

	void cml() {
		// complement link bit
		if (link == ASCII_ZERO) {
			link = ASCII_ONE;
		}
		else {
			link = ASCII_ZERO;
		}
	}

	void dc(char ***regSelected) {
		// decrement (register) by one
		int regInt = binToDec(*(*regSelected));
		if (regInt == 0) {
			cm(regSelected);
			link = ASCII_ONE;
//			if (link == ASCII_ZERO) {
//				link = ASCII_ONE;
//			}
//			else {
//				link = ASCII_ZERO;
//			}
		}
		else {
			regInt--;
			char *temp = decToBin(regInt);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				(*(*regSelected))[i] = temp[i];
			}
			free(temp);
		}
	}

	void ic(char ***regSelected) {
		// increment (register) by one
		int regInt = binToDec(*(*regSelected));
		//printf("REGINT 1 = %d\n", regInt);
		if (regInt == (MAX_NUM_ELEMENTS - 1)) {
			cl(regSelected);
			link = ASCII_ONE;
//			if (link == ASCII_ZERO) {
//				link = ASCII_ONE;
//			}
//			else {
//				link = ASCII_ZERO;
//			}
		}
		else {
			regInt++;
			char *temp = decToBin(regInt);
			//printf("%s\n", temp);
			int i;
			for (i = 0; i < REG_LEN; i++) {
				(*(*regSelected))[i] = temp[i];
			}
			free(temp);
		}
	}

	void non_register_non_memory_instruction() {
		int temp = binToDec(instr);
		if (temp == 0) {
			// NOP; do nothing
			if (debug) { printf("Instruction was NOP; doing nothing\nPC is currently 0x%04X/%d\n\n", currPC, currPC); }
		}
		else if (temp == 1) {
			// halt; low-order bit of the PSW is set to 0
			char *temp = decToBin(psw);
			temp[(REG_LEN - 1)] = ASCII_ZERO;
			psw = binToDec(temp);
			free(temp);
		}
		else if (temp == 2) {
			// return; pop the stack into the PC
		}
		incrCurrPC();
	}

	void register_memory_reference_instruction(int opcode, int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		char **regSpecified;
		if (regSpecifier == A_REG_SPECIFIER) {
			regSpecified = &aReg;
		}
		else if (regSpecifier == B_REG_SPECIFIER) {
			regSpecified = &bReg;
		}
		else if (regSpecifier == C_REG_SPECIFIER) {
			regSpecified = &cReg;
		}
		else {
			regSpecified = &dReg;
		}

		if (opcode == RMRI_ADD) {
			rmri_add(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_SUB) {
			rmri_sub(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_MUL) {
			rmri_mul(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_DIV) {
			rmri_div(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_OR) {
			rmri_or(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_XOR) {
			rmri_xor(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RMRI_LOAD) {
			rmri_load(&regSpecified, addressingMode, pageMode, pageAddr);
		}
		else {
			rmri_store(&regSpecified, addressingMode, pageMode, pageAddr);
		}
	}

	void input_output_instruction() {

	}

	void non_register_memory_reference_instruction(int opcode, int subOpcode, int addressingMode, int pageMode, int pageAddr) {
		if (opcode == 11 && subOpcode == 0) {
			// isz
			isz(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == 11 && subOpcode == 1) {
			// jmp
			jmp(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == 11 && subOpcode == 2) {
			// call
			call(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == 12 && subOpcode == 0) {
			// push
			push(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == 12 && subOpcode == 1) {
			// pop
			pop(addressingMode, pageMode, pageAddr);
		}
	}

	void register_to_register_instruction(int subOpcode, int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		if (subOpcode == 0) {
			// mod
			rtri_mod(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 1) {
			// add
			rtri_add(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 2) {
			// sub
			rtri_sub(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 3) {
			// mul
			rtri_mul(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 4) {
			// div
			rtri_div(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 5) {
			// and
			rtri_and(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 6) {
			// or
			rtri_or(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (subOpcode == 7) {
			// xor
			rtri_xor(regSpecifier1, regSpecifier2, regSpecifier3);
		}
		incrCurrPC();
	}

	void non_memory_register_instruction(int regSpecifier) {

		char **regSelected;
		if (regSpecifier == A_REG_SPECIFIER) {
			regSelected = &aReg;
		}
		else if (regSpecifier == B_REG_SPECIFIER) {
			regSelected = &bReg;
		}
		else if (regSpecifier == C_REG_SPECIFIER) {
			regSelected = &cReg;
		}
		else {
			regSelected = &dReg;
		}

		if (instr[6] == ASCII_ONE) {			sm(&regSelected);			}
		if (instr[7] == ASCII_ONE) {			sz(&regSelected);			}
		if (instr[8] == ASCII_ONE) {			snl();						}
		if (instr[9] == ASCII_ONE) {			rss();						}
		if (instr[10] == ASCII_ONE) {			cl(&regSelected);			}
		if (instr[11] == ASCII_ONE) {			cll();						}
		if (instr[12] == ASCII_ONE) {			cm(&regSelected);			}
		if (instr[13] == ASCII_ONE) {			cml();						}
		if (instr[14] == ASCII_ONE) {			dc(&regSelected);			}
		if (instr[15] == ASCII_ONE) {			ic(&regSelected);			}
		incrCurrPC();
	}

	int getRegisterSpecifierLen2() {
		char tempArr[3];
		char *tempArrPtr = tempArr;
		tempArrPtr[0] = instr[4];
		tempArrPtr[1] = instr[5];
		tempArrPtr[2] = '\0';
		int temp = binToDec(tempArrPtr);
		if (temp == A_REG_SPECIFIER) { return A_REG_SPECIFIER; }
		else if (temp == B_REG_SPECIFIER) { return B_REG_SPECIFIER; }
		else if (temp == C_REG_SPECIFIER) { return C_REG_SPECIFIER; }
		else { return D_REG_SPECIFIER; }
	}

	int getRegisterSpecifierLen3() {
		char tempArr[4];
		char *tempArrPtr = tempArr;
		tempArrPtr[0] = instr[4];
		tempArrPtr[1] = instr[5];
		tempArrPtr[2] = instr[6];
		tempArrPtr[3] = '\0';
		int temp = binToDec(tempArrPtr);
		if (temp == A_REG_SPECIFIER) { return A_REG_SPECIFIER; }
		else if (temp == B_REG_SPECIFIER) { return B_REG_SPECIFIER; }
		else if (temp == C_REG_SPECIFIER) { return C_REG_SPECIFIER; }
		else if (temp == D_REG_SPECIFIER) { return D_REG_SPECIFIER; }
		else if (temp == PC_SPECIFIER) { return PC_SPECIFIER; }
		else if (temp == PSW_SPECIFIER) { return PSW_SPECIFIER; }
		else if (temp == SP_SPECIFIER) { return SP_SPECIFIER; }
		else { return SPL_SPECIFIER; }
	}

	int getSubOpcodeLen2() {
		char opcodeArr[3];
		char *opcodeArrPtr = opcodeArr;
		opcodeArrPtr[0] = instr[4];
		opcodeArrPtr[1] = instr[5];
		opcodeArrPtr[2] = '\0';
		return (binToDec(opcodeArrPtr));
	}

	int getSubOpcodeLen3() {
		char opcodeArr[4];
		char *opcodeArrPtr = opcodeArr;
		opcodeArrPtr[0] = instr[4];
		opcodeArrPtr[1] = instr[5];
		opcodeArrPtr[2] = instr[6];
		opcodeArrPtr[3] = '\0';
		return (binToDec(opcodeArrPtr));
	}

	int getOpcode() {
		char opcodeArr[5];
		char *opcodePtr = opcodeArr;
		opcodePtr[0] = instr[0];
		opcodePtr[1] = instr[1];
		opcodePtr[2] = instr[2];
		opcodePtr[3] = instr[3];
		opcodePtr[4] = '\0';
		return (binToDec(opcodePtr));
	}

	int get_DI_mode() {
		if (instr[6] == ASCII_ZERO) {
			return DIRECT;
		}
		return INDIRECT;
	}

	int getPageMode() {
		if (instr[7] == ASCII_ZERO) {
			return ZERO_PAGE;
		}
		return CURRENT_PAGE;
	}

	int getPageAddr(int pageMode) {
		// 256 pages of 256 words each
		if (pageMode == CURRENT_PAGE) {
			char *currPCBin = decToBin(currPC);
			char temp[(REG_LEN + 1)];
			int i;
			for (i = 0; i < 8; i++) {
				temp[i] = currPCBin[i];
			}
			for (; i < REG_LEN; i++) {
				temp[i] = instr[i];
			}
			temp[REG_LEN] = '\0';
			char *tempPtr = temp;
			free(currPCBin);
			return (binToDec(tempPtr));
		}
		else {
			char temp[9];
			int i;
			int start = 8;
			for (i = 0; i < 8; i++) {
				temp[i] = instr[start];
				start++;
			}
			temp[8] = '\0';
			char *tempPtr = temp;
			return (binToDec(tempPtr));
		}
	}

	int indexOfPageModePageAddr(int pageAddr) {
		char *pageAddrBin = decToBin(pageAddr);
		char pageNumBin[9];
		int i;
		for (i = 0; i < 9; i++) {
			pageNumBin[i] = pageAddrBin[i];
		}
		pageNumBin[8] = '\0';
		char *pageNumBinPtr = pageNumBin;
		int pageNum = binToDec(pageNumBinPtr);

		char addrInPage[9];
		int start = 8;
		for (i = 0; i < 9; i++) {
			addrInPage[i] = pageAddrBin[start];
		}
		addrInPage[8] = '\0';
		char *addrInPagePtr = addrInPage;
		int numInPage = binToDec(addrInPagePtr);

		int startIndex;
		int stopIndex = (pageNum + 1) * 256 - 1;
		int counter = 0;
		for (startIndex = pageNum * 256; startIndex < stopIndex; startIndex++) {
			if (counter == numInPage) {
				pageAddr = startIndex;
			}
			counter++;
		}
		free(pageAddrBin);
		return pageAddr;
	}

	void process() {
		int currInstr = words[currPC];
		char *temp = decToBin(currInstr);
		int i;
		for (i = 0; i < REG_LEN; i++) {
			instr[i] = temp[i];
		}
		free(temp);

		int opcode = getOpcode();
		int subOpcode = getSubOpcodeLen2();
		int regSpecifier = getRegisterSpecifierLen2();
		int addressingMode, pageMode, pageAddr;

		if (opcode == NRNMI) {																// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// 6-bit opcode, 10-bit data field
			non_register_non_memory_instruction();
		}
		else if (opcode >= RMRI_LOWER_BOUND && opcode <= RMRI_UPPER_BOUND) {
			// 4-bit opcode, 2-bit reg selector, D/I bit, Z/C bit, 8-bit page offset
			addressingMode = get_DI_mode();
			pageMode = getPageMode();
			pageAddr = getPageAddr(pageMode);
			//if (pageMode == CURRENT_PAGE) { pageAddr = indexOfPageModePageAddr(pageAddr); }

			register_memory_reference_instruction(opcode, regSpecifier, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == IO) {															// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// 4-bit opcode, 2-bit reg selector, 7-bit device num, 3-bit function field
			char deviceBinStr[8];
			deviceBinStr[0] = instr[6];
			deviceBinStr[1] = instr[7];
			deviceBinStr[2] = instr[8];
			deviceBinStr[3] = instr[9];
			deviceBinStr[4] = instr[10];
			deviceBinStr[5] = instr[11];
			deviceBinStr[6] = instr[12];
			deviceBinStr[7] = '\0';
			char *deviceBinStrPtr = deviceBinStr;
			int deviceNum = binToDec(deviceBinStrPtr);

			char functionCodeBinStr[4];
			functionCodeBinStr[0] = instr[13];
			functionCodeBinStr[1] = instr[14];
			functionCodeBinStr[2] = instr[15];
			functionCodeBinStr[3] = '\0';
			char *functionCodeBinStrPtr = functionCodeBinStr;
			int functionCode = binToDec(functionCodeBinStrPtr);

			char **regSpecified;
			if (regSpecifier == A_REG_SPECIFIER) {
				regSpecified = &aReg;
			}
			else if (regSpecifier == B_REG_SPECIFIER) {
				regSpecified = &bReg;
			}
			else if (regSpecifier == C_REG_SPECIFIER) {
				regSpecified = &cReg;
			}
			else {
				regSpecified = &dReg;
			}

			io(&regSpecified, deviceNum, functionCode);
		}
		else if (opcode >= NRMRI_LOWER_BOUND && opcode <= NRMRI_UPPER_BOUND) {
			// 4-bit opcode, 2-bit reg selector (serving as the sub-opcode), D/I bit, Z/C bit, 8-bit page offset
			subOpcode = regSpecifier;
			addressingMode = get_DI_mode();
			pageMode = getPageMode();
			pageAddr = getPageAddr(pageMode);
			//if (pageMode == CURRENT_PAGE) { pageAddr = indexOfPageModePageAddr(pageAddr); }

			non_register_memory_reference_instruction(opcode, subOpcode, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RTRI) {
			// 4-bit opcode, 3-bit sub-opcode, three 3-bit reg selectors (7 8 9, 10 11 12, 13 14 15)
			subOpcode = getSubOpcodeLen3();
			char temp1[4];
			char temp2[4];
			char temp3[4];
			temp1[0] = instr[7];
			temp1[1] = instr[8];
			temp1[2] = instr[9];
			temp1[3] = '\0';
			temp2[0] = instr[10];
			temp2[1] = instr[11];
			temp2[2] = instr[12];
			temp2[3] = '\0';
			temp3[0] = instr[13];
			temp3[1] = instr[14];
			temp3[2] = instr[15];
			temp3[3] = '\0';
			char *temp1Ptr = temp1;
			char *temp2Ptr = temp2;
			char *temp3Ptr = temp3;
			int regSpecifier1 = binToDec(temp1Ptr);
			int regSpecifier2 = binToDec(temp2Ptr);
			int regSpecifier3 = binToDec(temp3Ptr);

			register_to_register_instruction(subOpcode, regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else {
			// 4-bit opcode, 2-bit reg selector, SM, SZ, SNL, RSS, CL, CLL, CM, CML, DC, IC
			non_memory_register_instruction(regSpecifier);
		}

	}

	void setWords(int *buffer, int actualSize) {
		// take everything read in from the binary object file
		// fill words[] with that data, as according to the spec sheet

		if (debug) { printf("Magic \"number\" is %c%c%c%c\n\n", buffer[0], buffer[1], buffer[2], buffer[3]); }

		// check for magic "number"
		// add in handling for lower case later?
		if (buffer[0] != 'O' || buffer[1] != 'B' || buffer[2] != 'J' || buffer[3] != 'G') {
			freeWords();
			die("Object file did not begin with the magic \"number\"; halting/exiting");
		}
		if (actualSize <= 9) {				// 'OBJG', EP, block size, starting address
			freeWords();
			die("Object file did not have enough data to proceed; halting/exiting");
		}

		char *ep = mergeBytes(buffer[4], buffer[5]);
		currPC = binToDec(ep);
		free(ep);

		int counter;
		int blockSize;
		int blockSizeCtr;

		counter = 6;

		while (counter < actualSize) {
			blockSize = buffer[counter];
			counter++;
			char *binStartAddr = mergeBytes(buffer[counter], buffer[counter + 1]);
			counter += 2;
			int currBlockAddr = binToDec(binStartAddr);
			free(binStartAddr);
			blockSizeCtr = 3;

			if (currBlockAddr + blockSize >= MAX_NUM_ELEMENTS - 1) {
				freeWords();
				die("Error: buffer overrun imminent; halting/exiting");
			}

			if (debug) { printf("Block size %d, starting address of block %d:\n\n", blockSize, currBlockAddr); }

			while (blockSizeCtr < blockSize && counter < actualSize) {
				int byte1 = buffer[counter];
				blockSizeCtr++, counter++;
				int byte2 = buffer[counter];
				blockSizeCtr++, counter++;
				char *currInstr = mergeBytes(byte1, byte2);
				int temp = binToDec(currInstr);
				free(currInstr);
				words[currBlockAddr] = temp;

				if (debug) { printf("Storing 0x%04X/%d into address 0x%04X/%d...\nCurrent block size counter: %d/%d\nCurrent buffer counter: %d/%d\n\n", temp, temp, currBlockAddr, currBlockAddr, blockSizeCtr, blockSize, counter, actualSize); }
				//if (debug) { printf("Storing %d into address %d...\nCurrent block size counter: %d\nCurrent buffer counter: %d\n\n", temp, currBlockAddr, blockSizeCtr, counter); }
				currBlockAddr++;
			}
		}




//		int counter = 4;
//		int blockSize;
//
//		while (counter < ((actualSize - 7) / 2)) {
//
//			char *binStartAddr = mergeBytes(buffer[counter], buffer[(counter + 1)]);
//			int startAddr = binToDec(binStartAddr);
//			if (currPC == -1) {
//				currPC = startAddr;
//			}
//			free(binStartAddr);
//			counter += 2;
//			blockSize = buffer[counter];
//			counter++;
//
//			if (debug) { printf("Block size %d, starting address %d:\n", blockSize, startAddr); }
//
//			if ((startAddr + blockSize >= MAX_NUM_ELEMENTS)) {
//				freeWords();
//				die("Error: buffer overrun imminent; halting/exiting");
//			}
//
//			int blockSizeCtr = 0;
//
//			if (blockSize > ((actualSize - 7) / 2)) {
//				blockSize = ((actualSize - 7) / 2);
//			}
//
//			for (; blockSizeCtr < blockSize; blockSizeCtr++) {
//				char *temp1 = mergeBytes(buffer[counter], buffer[(counter + 1)]);
//				int temp2 = binToDec(temp1);
//				free(temp1);
//				counter += 2;
//				if (debug) { printf("Storing %d into address %d...\n", temp2, startAddr); }
//				words[startAddr] = temp2;
//				startAddr++;
//
//			}
//			if (debug) { printf("\n"); }
//		}
		//if (debug) { printf("\n\nContents of words[] array (address, (int) contents):\n\t[ "); toString(); printf("\n\n"); }

		if (debug) { printf("PC is set to start at 0x%04X/%d\n\n", currPC, currPC); }

		if (currPC == -1) {
			freeWords();
			die("Error: no entry point specified; halting/exiting");
		}
	}

	void readFile(char *filename) {

		file = fopen(filename, "rb");

		if (file == NULL) {
			fprintf(stderr, "Error: cannot open file \'%s\'!\n", filename);
			exit(-1);
		}

		int size;
		int actualSize;
		int counter;
		int byte;
		int *buffer;

		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (size == 0) {
			freeWords();
			die("Object file is empty; halting/exiting");
		}

		buffer = calloc(size, sizeof(int));
		actualSize = 0;
		counter = 0;
		while ((byte = getc(file)) != EOF) {
			if (byte != '\n') {
				buffer[counter] = byte;
				actualSize++;
				counter++;
			}
		}

		fclose(file);

		if (readFileDebug) { printf("Read in %d data bytes\nBuffer contents: [%d", (actualSize - 9), buffer[0]); int i; for(i = 1; i < actualSize; i++) { printf(", %d", buffer[i]); } printf("]\n\n"); }

//		if ((actualSize - 9) % 2 != 0) {
//			free(buffer);
//			freeWords();
//			die("Odd number of bytes encountered; halting/exiting");
//		}

		setWords(buffer, actualSize);
	}

	void initialize() {
		// "initialize" the "components/machine"
		// should only be called once in entire program
		int i;
		for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
			words[i] = -1;
		}

		char aRegArr[(REG_LEN + 1)];
		char bRegArr[(REG_LEN + 1)];
		char cRegArr[(REG_LEN + 1)];
		char dRegArr[(REG_LEN + 1)];
		//char pswArr[(REG_LEN + 1)];
		char instrArr[(REG_LEN + 1)];
		for (i = 0; i < (REG_LEN); i++) {
			aRegArr[i] = ASCII_ZERO;
			bRegArr[i] = ASCII_ZERO;
			cRegArr[i] = ASCII_ZERO;
			dRegArr[i] = ASCII_ZERO;
			//pswArr[i] = ASCII_ZERO;
			instrArr[i] = ASCII_ZERO;
			//printf("Iteration %d\n", i);
		}
		aRegArr[REG_LEN] = '\0';
		bRegArr[REG_LEN] = '\0';
		cRegArr[REG_LEN] = '\0';
		dRegArr[REG_LEN] = '\0';
		//pswArr[(REG_LEN)] = '\0';
		instrArr[REG_LEN] = '\0';
		aReg = aRegArr;
		bReg = bRegArr;
		cReg = cRegArr;
		dReg = dRegArr;
		//psw = pswArr;
		instr = instrArr;

		link = ASCII_ZERO;
		skipFlag = ASCII_ZERO;

		sp = -1;
		spl = 0;
		psw = 0;
	}

	int main (int argc, char **argv) {

		initialize();

		if (manDebug) {
			readFile("spover.obj");

			process();
			process();
			process();
			process();
			process();
			process();



//			printf("%d\n", binToDec(aReg));
//			char **blah = &aReg;
//			ic(&blah);
//			//printf("%d\n", binToDec(aReg));
//			printf("%s\n", aReg);

//			char *blah = "hello";
//			char **blah1 = &blah;
//			printf("%s\n", (*blah1));

//			while (currPC < MAX_NUM_ELEMENTS && currPC >= 0) {
//				parseInstr(currPC);
//			}

//			file = fopen("prime.obj", "rb");
//			int byte;
//			int counter = 0;
//			while ((byte = getc(file)) != EOF) {
//				if (byte == '\n' || byte == EOF) {
//					printf("\n");
//				}
//				else {
//					printf("%d.", byte);
//					counter++;
//				}
//			}
//			printf("\nRead in %d numbers, not counting initial setup numbers\n", (counter - 9));

		}

		else {
			boolean filenameNotGiven = TRUE;
			while (argc > 1) {
				argc--, argv++;
				if (**argv == '-') {
					scanArgs(*argv);
				}
				else {
					filenameNotGiven = FALSE;
					readFile(*argv);
				}
			}
			if (!filenameNotGiven) {
				while (currPC >= 0 && currPC < MAX_NUM_ELEMENTS) {
					// parseInstr(currPC);
				}
			}
			else {
				die("No object file specified!");
			}
		}


		return 0;
	}















