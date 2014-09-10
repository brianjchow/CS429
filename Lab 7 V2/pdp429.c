/*
 ============================================================================
 Name        : Brian Chow (bc23784)
 Program	 : pdp429.c
 Description : see readme
 ============================================================================
 */

/*
 	 ERRORS TO HANDLE
 	 	 - object file
 	 	 	 - wrong magic number; first 4 bytes are not 'OBJG'; handled
 	 	 	 - EOF when more data is expected
 	 	 	 - byte count is even, or too small (1, 3)
 	 	 	 - block address plus number of bytes to load exceeds size of memory
 	 	 -

 	 BUG FIXES
 	 	 - all number format conversion methods will accept a number that overflows the bounds set by 16-bit word lengths
 	 	 - FIXED: replace hard-coded constants in main instruction handlers with their respective global constants
 	 	 - FIXED: use pointer to pointer in instructions with a specified register to reduce redundant code
 	 	 - for all instructions taking in int n as a parameter, delete parameter and use global instr[] array instead (reduce redundant code)
 	 	 - link bit is being flipped whenever overflow occurs; should just be set
 	 	 - handle link bit when divide by 0 and subtraction underflow occurs
 	 	 - no over/underflow/divide-by-zero protection in all rtri instruction methods
 	 	 - return in non_register_non_memory_instruction() currently does nothing
 	 	 - call(), push(), and pop() currently do nothing and/or are probably incorrect
 	 	 - any instruction needing to reference the instruction currently calls a conversion method; fix for efficiency
 	 	 - all methods that check a regSpecifier against the global constants should have the else statement be an error
 	 	 - push method doesn't handle the event of an uninitialized stack pointer (sp == -1)
 	 	 - divide by zero: what to do with verbose output?
 	 	 - stack underflows are unhandled
 	 	 - verbose mode: link bit changes should only be reported AFTER the entire instruction has finished, not every time it's set

 */

#define _GNU_SOURCE							// for strcasestr

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
#define MAX_NUM_ELEMENTS 65536				// 2^16 = 65536
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

int aReg;
int bReg;
int cReg;
int dReg;
char link;
int words[MAX_NUM_ELEMENTS];
int instr;
char skipFlag;

int currPC = -1;							// program counter
int sp;										// stack pointer
int spl;									// stack pointer limit
int psw;									// processor status word (indicates if running or halted)

int *aRegPtr = &aReg;
int *bRegPtr = &bReg;
int *cRegPtr = &cReg;
int *dRegPtr = &dReg;
int *currPCPtr = &currPC;
int *spPtr = &sp;
int *splPtr = &spl;
int *pswPtr = &psw;

FILE *file;									// for "reading in" the initial machine state
signed long long cycleCount = 0;			// for use with verbose; indicates how long (in cycles) an instruction took
signed long long instrCount = 1;			// primarily for use with debugging when verbose == TRUE

char instrStrArr[1000];
//char *instrStr = instrStrArr;
char regChangesArr[1000];
//char *regChanges = regChangesArr;
char stackOverflow;
char stackUnderflow;
char *instrStr;
char *regChanges;

int prevPC;									// because verbose mode is fucking stupid

// default
//boolean debug = FALSE;
//boolean readFileDebug = FALSE;
//boolean manDebug = FALSE;					// for command line-less testing
//boolean verbose = FALSE;

// only verbose output
boolean debug = FALSE;
boolean readFileDebug = FALSE;
boolean manDebug = TRUE;					// for command line-less testing
boolean verbose = TRUE;

// only debug statements
//boolean debug = TRUE;
//boolean readFileDebug = FALSE;
//boolean manDebug = TRUE;					// for command line-less testing
//boolean verbose = FALSE;

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

	void die(char *reason) {
		fprintf(stderr, "*** %s ***\n\n", reason);
		free(instrStr);			// will always be allocated even if !verbose; see initialize()
		free(regChanges);		// will always be allocated even if !verbose; see initialize()
		exit(-1);
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

		if (strcmp(instrStr, "NOP") == 0) {
			fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)\n", cycleCount, prevPC, instr, instrStr);
		}
		else {
			fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s): %s\n", cycleCount, prevPC, instr, instrStr, regChanges);
		}
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

	void update_instrStr(char *instrName, int regSpecifier, int addressingMode, int deviceNum) {
		if (strlen(instrStr) > 0) {								strcat(instrStr, " ");		}
		if (addressingMode == 1) {								strcat(instrStr, "I ");		}
		strcat(instrStr, instrName);
		if (strcmp(instrName, "IOT") == 0) {
			if (deviceNum == 3) {								strcat(instrStr, " 3");		}
			else if (deviceNum == 4) {							strcat(instrStr, " 4");		}
			else {												strcat(instrStr, " ?");		}
		}
		else {
			if (regSpecifier >= 0) {
				if (regSpecifier == A_REG_SPECIFIER) {			strcat(instrStr, "A");		}
				else if (regSpecifier == B_REG_SPECIFIER) {		strcat(instrStr, "B");		}
				else if (regSpecifier == C_REG_SPECIFIER) {		strcat(instrStr, "C");		}
				else if (regSpecifier == D_REG_SPECIFIER) {		strcat(instrStr, "D");		}
				else {											strcat(instrStr, "?");		}
			}
		}
	}

	void update_regChanges(int firstOperand, int secondOperand,
			boolean firstOperandIsAddress, boolean firstOperandIsRegister, boolean firstOperandIsValue,
			boolean secondOperandIsAddress, boolean secondOperandIsRegister, boolean secondOperandIsValue) {
		// address, address		UNKNOWN (NO?)
		// address, register	UNKNOWN (NO?)
		// address, value		CAN HAPPEN		EG "M[0x0033] -> 0x000B"
		// register, address	UNKNOWN (NO?)
		// register, register	UNKNOWN (NO?)
		// register, value		CAN HAPPEN		EG "A -> 0x0016"
		// value, address		CAN HAPPEN		EG "0x000E -> M[0xFFFF]"
		// value, register		CAN HAPPEN		EG "0x0009 -> PC"
		// value, value			CAN'T HAPPEN	USED FOR REPORTING LINK BIT STATUS AND CHANGES
		if (strlen(regChanges) > 0) {			strcat(regChanges, ", ");		}
		if (firstOperandIsAddress) {
			if (secondOperandIsValue) {
				strcat(regChanges, "M[0x");
				char *firstOperandHexStr = decToHex(firstOperand);
				char *secondOperandHexStr = decToHex(secondOperand);
				strcat(regChanges, firstOperandHexStr);
				strcat(regChanges, "] -> 0x");
				strcat(regChanges, secondOperandHexStr);
				free(firstOperandHexStr);
				free(secondOperandHexStr);
			}
			else {
				// error
				strcat(regChanges, "?");
			}
		}
		else if (firstOperandIsRegister) {
			if (secondOperandIsValue) {
				char *secondOperandHexStr = decToHex(secondOperand);
				if (firstOperand == A_REG_SPECIFIER) {
					strcat(regChanges, "A -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == B_REG_SPECIFIER) {
					strcat(regChanges, "B -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == C_REG_SPECIFIER) {
					strcat(regChanges, "C -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == D_REG_SPECIFIER) {
					strcat(regChanges, "D -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == PC_SPECIFIER) {
					strcat(regChanges, "PC -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == PSW_SPECIFIER) {
					strcat(regChanges, "PSW -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == SP_SPECIFIER) {
					strcat(regChanges, "SP -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else if (firstOperand == SPL_SPECIFIER) {
					strcat(regChanges, "SPL -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				else {
					// error
					strcat(regChanges, "? -> 0x");
					strcat(regChanges, secondOperandHexStr);
				}
				free(secondOperandHexStr);
			}
			else {
				// error
				strcat(regChanges, "?");
			}
		}
		else if (firstOperandIsValue) {
			if (secondOperandIsAddress) {
				char *firstOperandHexStr = decToHex(firstOperand);
				char *secondOperandHexStr = decToHex(secondOperand);
				strcat(regChanges, "0x");
				strcat(regChanges, firstOperandHexStr);
				strcat(regChanges, " -> M[0x");
				strcat(regChanges, secondOperandHexStr);
				strcat(regChanges, "]");
				free(firstOperandHexStr);
				free(secondOperandHexStr);
			}
			else if (secondOperandIsRegister) {
				char *firstOperandHexStr = decToHex(firstOperand);
				strcat(regChanges, "0x");
				strcat(regChanges, firstOperandHexStr);
				strcat(regChanges, " -> ");
				if (secondOperand == A_REG_SPECIFIER) {
					strcat(regChanges, "A");
				}
				else if (secondOperand == B_REG_SPECIFIER) {
					strcat(regChanges, "B");
				}
				else if (secondOperand == C_REG_SPECIFIER) {
					strcat(regChanges, "C");
				}
				else if (secondOperand == D_REG_SPECIFIER) {
					strcat(regChanges, "D");
				}
				else if (secondOperand == PC_SPECIFIER) {
					strcat(regChanges, "PC");
				}
				else if (secondOperand == PSW_SPECIFIER) {
					strcat(regChanges, "PSW");
				}
				else if (secondOperand == SP_SPECIFIER) {
					strcat(regChanges, "SP");
				}
				else if (secondOperand == SPL_SPECIFIER) {
					strcat(regChanges, "SPL");
				}
				else {
					// error
					strcat(regChanges, "?");
				}
				free(firstOperandHexStr);
			}
			else if (secondOperandIsValue) {
				// special case for link bit
				// firstOperand gives value of link bit
				// if secondOperand == 0, format is L -> 0x0000
				// if secondOperand == 1, format is 0x0000 -> L
				char *firstOperandHexStr = decToHex(firstOperand);
				if (secondOperand == 0) {
					strcat(regChanges, "L -> 0x");
					strcat(regChanges, firstOperandHexStr);
				}
				else if (secondOperand == 1) {
					strcat(regChanges, "0x");
					strcat(regChanges, firstOperandHexStr);
					strcat(regChanges, " -> L");
				}
				else {
					// error
					strcat(regChanges, "?");
				}
				free(firstOperandHexStr);
			}
			else {
				// error
				strcat(regChanges, "?");
			}
		}
		else {
			// error
			strcat(regChanges, "?");
		}
	}

	// non-register non-memory instructions are handled below

	int* determine_register(int regSpecifier) {
		if (regSpecifier == A_REG_SPECIFIER) {				return aRegPtr;			}
		else if (regSpecifier == B_REG_SPECIFIER) {			return bRegPtr;			}
		else if (regSpecifier == C_REG_SPECIFIER) {			return cRegPtr;			}
		else if (regSpecifier == D_REG_SPECIFIER) {			return dRegPtr;			}
		else if (regSpecifier == PC_SPECIFIER) {			return currPCPtr;		}
		else if (regSpecifier == PSW_SPECIFIER) {			return pswPtr;			}
		else if (regSpecifier == SP_SPECIFIER) {			return spPtr;			}
		else {												return splPtr;			}
	}

	void hlt() {
		if (debug) { printf("Instruction was HLT; now halting/exiting"); }
		if (verbose && stackOverflow != ASCII_ONE) {
			update_instrStr("HLT", -1, -1, -1);
			update_regChanges(PSW_SPECIFIER, psw, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
		}
		psw = 0;
		if (verbose) {
			update_regChanges(psw, PSW_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
			printVerbose();
		}
		free(instrStr);			// will always be allocated even if !verbose; see initialize()
		free(regChanges);		// will always be allocated even if !verbose; see initialize()
		exit(0);
	}

	void rmri_add(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) + (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_add\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this happens, then the object file is borked
			wordInt = 0;
		}

		if (verbose) { update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }

		int addedInt = regInt + wordInt;
		if (addedInt > (MAX_NUM_ELEMENTS - 1)) {
			addedInt &= 0xffff;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}

		char *regIntStr = decToBin(regInt);
		char *wordIntStr = decToBin(wordInt);
		char *addedIntStr = decToBin(addedInt);
		if (regIntStr[0] == wordIntStr[0] && addedIntStr[0] != regIntStr[0]) {
			if (verbose && link != ASCII_ONE) {
				link = ASCII_ONE;
				update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
			}
		}
		free(regIntStr);
		free(wordIntStr);
		free(addedIntStr);

		(*regSelected) = addedInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", addedInt, addedInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("ADD", regSpecifier, addressingMode, -1);
			update_regChanges(addedInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_sub(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) - (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_sub\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this happens, then the object file is borked
			wordInt = 0;
		}
		int subbedInt = regInt - wordInt;
		if (subbedInt < 0) {
			subbedInt &= 0xffff;
			//link = ASCII_ONE;
			//if (verbose) { update_regChanges(1, -1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}
		(*regSelected) = subbedInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", subbedInt, subbedInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("SUB", regSpecifier, addressingMode, -1);
			update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(subbedInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_mul(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) * (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_mul\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this happens, then the object file is borked
			wordInt = 0;
		}

		if (verbose) { update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }

		int multInt = regInt * wordInt;
		if (multInt > (MAX_NUM_ELEMENTS - 1) || multInt < 0) {
			multInt &= 0xffff;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}

		char *regIntStr = decToBin(regInt);
		char *wordIntStr = decToBin(wordInt);
		char *multIntStr = decToBin(multInt);
		if (regIntStr[0] == wordIntStr[0] && multIntStr[0] != regIntStr[0]) {
			if (verbose && link != ASCII_ONE) {
				link = ASCII_ONE;
				update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
			}
		}
		free(regIntStr);
		free(wordIntStr);
		free(multIntStr);

		(*regSelected) = multInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", multInt, multInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("MUL", regSpecifier, addressingMode, -1);
			update_regChanges(multInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_div(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) / (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_div\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this happens, then the object file is borked
			wordInt = 0;
		}

		if (verbose) {
			update_instrStr("DIV", regSpecifier, addressingMode, -1);
			update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
			if (addressingMode == INDIRECT) { update_regChanges(words[pageAddr], words[(words[pageAddr])], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
		}

		if (wordInt == 0) {
			(*regSelected) = 0;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
			if (verbose) { update_regChanges((*regSelected), regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
		}
		else {
			//int divInt = regInt / wordInt;
			int divInt = ((short) regInt) / ((short) wordInt);
			divInt &= 0xffff;
			(*regSelected) = divInt;

			if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", divInt, divInt, currPC, currPC); }

			if (verbose) {
				update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
				update_regChanges(divInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
			}
		}
		incrCurrPC();
	}

	void rmri_and(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) & (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_and\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
				//if (verbose) { update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
				//if (verbose) { update_regChanges(effAddrInt, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this occurs, then the object file is borked
			wordInt = 0;
		}
		int andInt = regInt & wordInt;
		(*regSelected) = andInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", andInt, andInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("AND", regSpecifier, addressingMode, -1);
			update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(andInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_or(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) | (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_or\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
				//if (verbose) { update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
				//if (verbose) { update_regChanges(effAddrInt, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this occurs, then the object file is borked
			wordInt = 0;
		}
		int orInt = regInt | wordInt;
		(*regSelected) = orInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", orInt, orInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("OR", regSpecifier, addressingMode, -1);
			update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(orInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_xor(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) ^ (memory operand) => (register selected)
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction is rmri_xor\nPage address is %d\nRegister contents before change: 0x%04X/%d\n", pageAddr, regInt, regInt); }

		if (verbose) {	update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);  }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this happens, then the object file is borked
			wordInt = 0;
		}
		int xorInt = regInt ^ wordInt;
		(*regSelected) = xorInt;

		if (debug) { printf("Register contents after change: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", xorInt, xorInt, currPC, currPC); }

		if (verbose) {
			update_instrStr("XOR", regSpecifier, addressingMode, -1);
			update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(xorInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_load(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (memory operand) => (register selected); overwrite contents of register selected with contents of memory operand

		int *regSelected = determine_register(regSpecifier);

		if (debug) { printf("Instruction was rmri_load\nPC is currently 0x%04X/%d\nAddressing mode: %d\nPage "
				"address: %d\nRegister contents before load: 0x%04X\n", currPC, currPC, addressingMode, pageAddr, (*regSelected)); }

		int wordInt;
		if (words[pageAddr] != -1) {
			if (addressingMode == DIRECT) {
				wordInt = words[pageAddr];
				cycleCount++;
				//if (verbose) { update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
			else {
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] != -1) {
					wordInt = words[effAddrInt];
				}
				else {
					wordInt = 0;
				}
				cycleCount += 2;
				if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
				if (verbose) { update_regChanges(effAddrInt, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			}
		}
		else {
			// if this occurs then the object file is borked
			wordInt = 0;
		}
		(*regSelected) = wordInt;

		if (debug) { printf("Register contents after load: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", (*regSelected), (*regSelected), currPC, currPC); }

		if (verbose) {
			update_instrStr("LD", regSpecifier, addressingMode, -1);
			if (addressingMode == DIRECT) {
				update_regChanges(pageAddr, wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			}
			else {
				//update_regChanges(words[pageAddr], wordInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			}
			update_regChanges(wordInt, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		incrCurrPC();
	}

	void rmri_store(int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		// (register selected) => (memory operand); overwrite contents of memory operand with contents of register selected
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);



		if (addressingMode == DIRECT) {
			words[pageAddr] = regInt;
			cycleCount++;
			if (verbose) { update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
			if (verbose) { update_regChanges(regInt, pageAddr, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE); }
		}
		else {
			int effAddrInt = words[pageAddr];
			words[effAddrInt] = regInt;//printf("0x%04X\n", pageAddr);
			if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			if (verbose) { update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
			if (verbose) { update_regChanges(regInt, effAddrInt, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE); }
			cycleCount += 2;
			//if (verbose) { update_regChanges(regInt, effAddrInt, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE); }
			//if (verbose) { update_regChanges(pageAddr, effAddrInt, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
		}

		if (debug) { printf("Instruction is rmri_store\nPage address is %d\nStored: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", pageAddr, regInt, regInt, currPC, currPC); }

		if (verbose) {	update_instrStr("ST", regSpecifier, addressingMode, -1);	}

		incrCurrPC();
	}

	void io(int regSpecifier, int deviceNum, int functionCode) {
		// ignore function code
		// if device is 3
			// do a getchar() to read a byte from stdin
			// put that byte in the register specified in the IOT instruction
			// an EOF on stdin should cause the register to be set to 0xffff
		// if device is 4
			// take low-order 8 bits from the specified register
			// output it as a byte to stdout (putchar())
		// ignore all other devices
		cycleCount++;
		int *regSelected = determine_register(regSpecifier);
		if (deviceNum == 3) {
			int c = getchar();
			if (c > (MAX_NUM_ELEMENTS - 1)) {
				c &= (MAX_NUM_ELEMENTS - 1);
			}
			if (c < 0) {
				c = (MAX_NUM_ELEMENTS - 1);
			}
			(*regSelected) = c;

			if (verbose) { update_regChanges(c, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
		}
		else if (deviceNum == 4) {
			int regInt = (*regSelected);
			char *temp = decToBin(regInt);
			char out[9];
			out[0] = temp[8];
			out[1] = temp[9];
			out[2] = temp[10];
			out[3] = temp[11];
			out[4] = temp[12];
			out[5] = temp[13];
			out[6] = temp[14];
			out[7] = temp[15];
			out[8] = '\0';
			char *outPtr = out;
			int toPrint = binToDec(outPtr);
			free(temp);
			putchar(toPrint);

			if (verbose) { update_regChanges(regSpecifier, toPrint, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		}
		else {
			// unsupported op
			//die("Unsupported instruction! Now halting/exiting");
		}

		if (verbose) {		update_instrStr("IOT", regSpecifier, -1, deviceNum);		}

		incrCurrPC();
	}

	void isz(int addressingMode, int pageMode, int pageAddr) {
		// increment and skip if zero
		// contents of the effective address are incremented by one and put back in the same memory location
		// if result of increment is zero, the next instruction is skipped

		if (addressingMode == DIRECT) {
			if (words[pageAddr] == -1) {
				// if this happens, then the object file is borked
			}
			else {
				if (verbose) { update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
				int temp = words[pageAddr];
				if (temp == (MAX_NUM_ELEMENTS - 1)) {
					temp = 0;
					skipFlag = ASCII_ONE;
				}
				else {
					temp++;
				}
				words[pageAddr] = temp;

				if (debug) { printf("Instruction was isz (increment and skip if zero)\nPage address is 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", pageAddr, pageAddr, currPC, currPC); }
				cycleCount += 2;
			}
		}
		else {
			// indirect addressing
			if (words[pageAddr] == -1) {
				// if this happens, then the object file is borked
			}
			else {
				if (verbose) { update_regChanges(pageAddr, words[words[pageAddr]], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
				int effAddrInt = words[pageAddr];
				if (words[effAddrInt] == -1) {
					// if this happens, then the object file is borked
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

				if (debug) { printf("Instruction was isz (increment and skip if zero)\nPage address is 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", effAddrInt, effAddrInt, currPC, currPC); }
				cycleCount += 3;
			}
		}

		if (verbose) {
			update_instrStr("ISZ", -1, addressingMode, -1);
			update_regChanges(words[pageAddr], pageAddr, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE);
		}

		incrCurrPC();
		if (currPC == ((prevPC + 2) & 0xffff)) {
			update_regChanges(currPC, PC_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}
	}

	void jmp(int addressingMode, int pageMode, int pageAddr) {
		// set program counter to effective address

		if (debug) { printf("Instruction was jmp\nPage address is 0x%04X/%d\nPC for this instruction before change was 0x%04X/%d\n", pageAddr, pageAddr, currPC, currPC); }

		if (verbose) { update_instrStr("JMP", -1, addressingMode, -1); }

		prevPC = currPC;

		if (addressingMode == DIRECT) {
			currPC = pageAddr;
		}
		else {
			int effAddrInt = words[pageAddr];
			currPC = effAddrInt;
			cycleCount++;
		}
		if (currPC > (MAX_NUM_ELEMENTS - 1) || currPC < 0) {
			die("Fatal error: PC out of bounds");
		}

		if (debug) { printf("PC for this instruction after change is 0x%04X/%d\n\n", currPC, currPC); }

		if (verbose) {
			if (addressingMode == INDIRECT) {
				update_regChanges(pageAddr, currPC, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			}
			update_regChanges(currPC, PC_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}
	}

	void call(int addressingMode, int pageMode, int pageAddr) {
		// push return address (PC + 1) onto stack; address of memory operand => PC

		if (debug) { printf("Instruction was call\nPage address is 0x%04X/%d\nPC for this instruction "
				"before change was 0x%04X/%d\nStack pointer for this instruction before change was 0x%04X/%d\n", pageAddr, pageAddr, currPC, currPC, sp, sp); }

		prevPC = currPC;

		words[sp] = (currPC + 1);
		sp--;

		if (verbose) {
			update_instrStr("CALL", -1, addressingMode, -1);
			if (addressingMode == INDIRECT) {
				update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			}
			update_regChanges((currPC + 1), (sp + 1), FALSE, FALSE, TRUE, TRUE, FALSE, FALSE);
			update_regChanges(sp, SP_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		int addr;
		if (addressingMode == DIRECT) {
			addr = pageAddr;
		}
		else {
			addr = words[pageAddr];
			cycleCount++;
		}
		currPC = addr;

		cycleCount++;

		if (debug) { printf("PC for this instruction after change is 0x%04X/%d\nStack pointer for this instruction after change was 0x%04X/%d\n\n", currPC, currPC, sp, sp); }

		if (verbose) { update_regChanges(currPC, PC_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void push(int addressingMode, int pageMode, int pageAddr) {
		// push address of memory operand to the stack; decrement SP
		// "set whatever the sp is pointing to to the value given by the instruction; then decrement sp"

		if (debug) { printf("Instruction was push\nPage address is 0x%04X/%d\nStack pointer before push is 0x%04X/%d\n", pageAddr, pageAddr, sp, sp); }

		if (sp < spl) {
			if (verbose) {
				update_instrStr("PUSH Stack Overflow", -1, addressingMode, -1);
				update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
				update_regChanges(PSW_SPECIFIER, psw, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
				cycleCount++;
			}
			hlt();
			//die("Fatal error: stack overflow!");
		}
		else {
			if (verbose) { update_instrStr("PUSH", -1, addressingMode, -1); }
		}

		int pageAddrCopy = pageAddr;
		if (sp == -1) {
			// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		}
		else {
			pageAddrCopy--;

			if (verbose) { update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE); }
			if (verbose) { update_regChanges(words[pageAddr], sp, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE); }

			words[sp] = words[pageAddr];
//			words[pageAddr] = pageAddrCopy;
//			words[sp] = pageAddrCopy;
			sp--;

			cycleCount += 2;

			if (verbose) { update_regChanges(sp, SP_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
		}

		if (sp < spl) {
			stackOverflow = ASCII_ONE;
			if (verbose) {
				char *spHex = decToHex(sp);
				char *splHex = decToHex(spl);
				strcat(regChanges, "\nStack Pointer = 0x");
				strcat(regChanges, spHex);
				strcat(regChanges, "; Stack Limit = 0x");
				strcat(regChanges, splHex);
			}
		}

		if (debug) { printf("Stack pointer after push is 0x%04X/%d\n\n", sp, sp); }

		incrCurrPC();
	}

	void pop(int addressingMode, int pageMode, int pageAddr) {
		// pop top of stack => memory operand; increment SP

		if (debug) { printf("Instruction was pop\nPage address is 0x%04X/%d\nPC for this instruction before pop is 0x%04X/%d\nStack pointer "
				"for this instruction before pop is 0x%04X/%d\n", pageAddr, pageAddr, currPC, currPC, sp, sp); }

		if (sp == (MAX_NUM_ELEMENTS - 1)) {
			if (verbose) {
				update_instrStr("POP Stack Underflow", -1, addressingMode, -1);
				update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
				update_regChanges(PSW_SPECIFIER, psw, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
				cycleCount++;
			}
			hlt();
			//die("Fatal error: stack underflow!");
		}

		sp++;

		if (verbose) {
			update_instrStr("POP", -1, addressingMode, -1);
			update_regChanges(SP_SPECIFIER, (sp - 1), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(sp, SP_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}

		int temp = words[sp];
		if (addressingMode == DIRECT) {
			//words[sp] = temp;
			words[pageAddr] = temp;
			cycleCount += 2;
		}
		else {
			int effAddrInt = words[pageAddr];
			//words[sp] = temp;
			words[effAddrInt] = temp;
			cycleCount += 3;
		}

		if (debug) { printf("PC for this instruction after pop is 0x%04X/%d\nStack pointer after pop is 0x%04X/%d\n\n", currPC, currPC, sp, sp); }

		if (verbose) {
			update_regChanges(sp, temp, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
			update_regChanges(temp, pageAddr, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE);
		}

		if (sp == (MAX_NUM_ELEMENTS - 1)) {
			stackUnderflow = ASCII_ONE;
//			if (verbose) {											// enable for print alert
//				char *spHex = decToHex(sp);
//				char *splHex = decToHex(spl);
//				strcat(regChanges, "\nStack Pointer = 0x");
//				strcat(regChanges, spHex);
//				strcat(regChanges, "; Stack Limit = 0x");
//				strcat(regChanges, splHex);
//			}
		}

		incrCurrPC();
	}

	void rtri_mod(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result;
		if ((*reg3Ptr) == 0) {
			result = 0;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}
		else {
			//result = (*reg2Ptr) % (*reg3Ptr);
			result = ((short) (*reg2Ptr)) % ((short) (*reg3Ptr));
			result &= 0xffff;
		}

		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_mod\nStoring results of register IDs (%d (mod) %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("MOD", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_add(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) + (*reg3Ptr);
		if (result > (MAX_NUM_ELEMENTS - 1)) {
			result &= MAX_NUM_ELEMENTS - 1;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}

		char *reg2Str = decToBin((*reg2Ptr));
		char *reg3Str = decToBin((*reg3Ptr));
		char *resultStr = decToBin(result);
		if (reg2Str[0] == reg3Str[0] && resultStr[0] != reg2Str[0]) {
			if (verbose && link != ASCII_ONE) {
				link = ASCII_ONE;
				update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
			}
		}
		free(reg2Str);
		free(reg3Str);
		free(resultStr);

		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_add\nStoring results of register IDs (%d + %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("ADD", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_sub(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) - (*reg3Ptr);
		if (result < 0) {
			result &= (MAX_NUM_ELEMENTS - 1);
			//link = ASCII_ONE;
			//if (verbose) { update_regChanges(1, -1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}
		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_sub\nStoring results of register IDs (%d - %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("SUB", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_mul(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) * (*reg3Ptr);
		if (result > (MAX_NUM_ELEMENTS - 1)) {
			result &= MAX_NUM_ELEMENTS - 1;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}

		char *reg2Str = decToBin((*reg2Ptr));
		char *reg3Str = decToBin((*reg3Ptr));
		char *resultStr = decToBin(result);
		if (reg2Str[0] == reg3Str[0] && resultStr[0] != reg2Str[0]) {
			if (verbose && link != ASCII_ONE) {
				link = ASCII_ONE;
				update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
			}
		}
		free(reg2Str);
		free(reg3Str);
		free(resultStr);

		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_mul\nStoring results of register IDs (%d * %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("MUL", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_div(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result;
		if ((*reg3Ptr) == 0) {
			result = 0;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
			//if (debug) { printf("Instruction was rtri_div\nDivide by 0; no change"); }
		}
		else {
			//result = (*reg2Ptr) / (*reg3Ptr);
			result = ((short) (*reg2Ptr)) / ((short) (*reg3Ptr));
			result &= 0xffff;
		}
		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_div\nStoring results of register IDs (%d / %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }

		if (verbose) { update_instrStr("DIV", -1, -1, -1); }

	}

	void rtri_and(int regSpecifier1, int regSpecifier2, int regSpecifier3) {

		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) & (*reg3Ptr);
		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_and\nStoring results of register IDs (%d & %d) into register ID %d\nResult "
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("AND", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_or(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) | (*reg3Ptr);
		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_or\nStoring results of register IDs (%d | %d) into register ID %d\nResult "
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("OR", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void rtri_xor(int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		int *reg1Ptr = determine_register(regSpecifier1);
		int *reg2Ptr = determine_register(regSpecifier2);
		int *reg3Ptr = determine_register(regSpecifier3);

		if (verbose) { update_regChanges(regSpecifier2, (*reg2Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }
		if (verbose) { update_regChanges(regSpecifier3, (*reg3Ptr), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int result = (*reg2Ptr) ^ (*reg3Ptr);
		(*reg1Ptr) = result;

		if (debug) { printf("Instruction was rtri_xor\nStoring results of register IDs (%d ^ %d) into register ID %d\nResult"
				" was 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", regSpecifier2, regSpecifier3, regSpecifier1, result, result, currPC, currPC); }

		if (verbose) { update_instrStr("XOR", -1, -1, -1); }

		if (verbose) { update_regChanges(result, regSpecifier1, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE); }
	}

	void sm(int regSpecifier) {
		// skip if (register) is negative (sign bit is 1)
		int *regSelected = determine_register(regSpecifier);
		char *temp = decToBin((*regSelected));
		if (temp[0] == ASCII_ONE) {
			skipFlag = ASCII_ONE;
		}
		free(temp);

		if (debug) { printf("Instruction was sm (skip if register is negative)\nskipFlag is now %c\nPC for this instruction was 0x%04X/%d\n\n", skipFlag, currPC, currPC); }

		if (verbose) {
			update_instrStr("SM", regSpecifier, -1, -1);
			update_regChanges(regSpecifier, (*regSelected), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
		}
	}

	void sz(int regSpecifier) {
		// skip if (register) is 0
		int *regSelected = determine_register(regSpecifier);
		if ((*regSelected) == 0) {
			skipFlag = ASCII_ONE;
		}

		if (debug) { printf("Instruction was sz (skip if register is zero)\nskipFlag is now %c\nPC for this instruction was 0x%04X/%d\n\n", skipFlag, currPC, currPC); }

		if (verbose) {
			update_instrStr("SZ", regSpecifier, -1, -1);
			update_regChanges(regSpecifier, (*regSelected), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
		}
	}

	void snl() {
		// skip if link bit is non-zero
		if (link == ASCII_ONE) {
			skipFlag = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 0, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}

		if (debug) { printf("Instruction was snl (skip if link bit is non-zero)\nskipFlag is now %c\nPC for this instruction was 0x%04X/%d\n\n", skipFlag, currPC, currPC); }

		if (verbose) {
			update_instrStr("SNL", -1, -1, -1);
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

		if (debug) { printf("Instruction was rss (reverse skip sense; flip skipFlag bit)\nskipFlag is now %c\nPC for this instruction was 0x%04X/%d\n\n", skipFlag, currPC, currPC); }

		if (verbose) { update_instrStr("RSS", -1, -1, -1); }
	}

	void cl(int regSpecifier) {
		// clear (register)
		int *regSelected = determine_register(regSpecifier);
		(*regSelected) = 0;

		if (debug) { printf("Instruction was cl (clear register)\nRegister contents after clear: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", (*regSelected), (*regSelected), currPC, currPC); }

		if (verbose) {
			update_instrStr("CL", regSpecifier, -1, -1);
			update_regChanges(0, regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
			//update_regChanges(regSpecifier, (*regSelected), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
		}
	}

	void cll() {
		// clear link bit
		//if (verbose) { update_regChanges((link - ASCII_ZERO), 0, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }

		if (link != ASCII_ZERO && verbose) {
			update_regChanges(0, 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
		}
		link = ASCII_ZERO;

		if (debug) { printf("Instruction was cll (clear link bit)\nLink bit is now %c\n\n", link); }

		if (verbose) {
			update_instrStr("CLL", -1, -1, -1);
			//update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
		}
		//if (verbose) { update_regChanges(0, -1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
	}

	void cm(int regSpecifier) {
		// complement register
		int *regSelected = determine_register(regSpecifier);
		char *temp = decToBin((*regSelected));

		if (debug) { printf("Instruction was cm (complement register)\nContents of register before complement: %s\n", temp); }

		if (verbose) { update_regChanges(regSpecifier, (*regSelected), FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		int i;
		for (i = 0; i < REG_LEN; i++) {
			if (temp[i] == ASCII_ONE) {
				temp[i] = ASCII_ZERO;
			}
			else {
				temp[i] = ASCII_ONE;
			}
		}

		int out = binToDec(temp);
		(*regSelected) = out;
		free(temp);

		if (debug) { printf("Contents of register after complement: %s\nPC for this instruction was 0x%04X/%d\n\n", temp, currPC, currPC); }

		if (verbose) {
			update_instrStr("CM", regSpecifier, -1, -1);
			update_regChanges((*regSelected), regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}
	}

	void cml() {
		// complement link bit
		if (verbose) { update_regChanges((link - ASCII_ZERO), 0, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		if (link == ASCII_ZERO) {
			link = ASCII_ONE;
		}
		else {
			link = ASCII_ZERO;
		}

		if (debug) { printf("Instruction was cml (complement link bit)\nLink bit is now %c\nPC for this instruction was 0x%04X/%d\n\n", link, currPC, currPC); }

		if (verbose) {
			update_instrStr("CML", -1, -1, -1);
			update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
		}
	}

	void dc(int regSpecifier) {
		// decrement (register) by one; if register is 0, complement and set link bit
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction was dc (decrement register)\nContents of register before decrement: 0x%04X/%d\n", regInt, regInt); }

		if (verbose) { update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		if (regInt == 0) {
			(*regSelected) = (MAX_NUM_ELEMENTS - 1);
			//link = ASCII_ONE;
			//if (verbose) { update_regChanges(1, -1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}
		else {
			(*regSelected) = regInt - 1;
		}

		if (debug) { printf("Contents of register after decrement: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", (*regSelected), (*regSelected), currPC, currPC); }

		if (verbose) {
			update_instrStr("DC", regSpecifier, -1, -1);
			update_regChanges((*regSelected), regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}
	}

	void ic(int regSpecifier) {
		// increment (register) by one; if register is 65535, clear and set link bit
		int *regSelected = determine_register(regSpecifier);
		int regInt = (*regSelected);

		if (debug) { printf("Instruction was ic (increment register)\nContents of register before increment: 0x%04X/%d\n", regInt, regInt); }

		if (verbose) { update_regChanges(regSpecifier, regInt, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE); }

		if (regInt == (MAX_NUM_ELEMENTS - 1)) {
			(*regSelected) = 0;
			link = ASCII_ONE;
			if (verbose) { update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE); }
		}
		else {
			(*regSelected) = regInt + 1;
		}

		char *regIntStr = decToBin(regInt);
		char *resultStr = decToBin((*regSelected));
		if (resultStr[0] != regIntStr[0]) {
			if (verbose && link != ASCII_ONE) {
				link = ASCII_ONE;
				update_regChanges((link - ASCII_ZERO), 1, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE);
			}
		}

		if (debug) { printf("Contents of register after increment: 0x%04X/%d\nPC for this instruction was 0x%04X/%d\n\n", (*regSelected), (*regSelected), currPC, currPC); }

		if (verbose) {
			update_instrStr("IN", regSpecifier, -1, -1);
			update_regChanges((*regSelected), regSpecifier, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
		}
	}

	void non_register_non_memory_instruction(int addressingMode, int pageAddr) {
		cycleCount++;
		//int temp = binToDec(instr);
		if (instr == 0) {
			// NOP; do nothing
			if (debug) { printf("Instruction was NOP; doing nothing\nPC for this instruction was 0x%04X/%d\n\n", currPC, currPC); }

			if (verbose) { update_instrStr("NOP", -1, -1, -1); }

			incrCurrPC();
		}
		else if (instr == 1) {
			// halt; low-order bit of the PSW is set to 0
			if (debug) { printf("Instruction was hlt (halt)\nPC for this instruction was 0x%04X/%d\n\n", currPC, currPC); }

			//if (verbose) { update_instrStr("HLT", -1, -1, -1); }

			hlt();
		}
		else if (instr == 2) {
			// return; pop the stack into the PC
			if (debug) { printf("Instruction was ret (return);\nPC for this instruction was 0x%04X/%d\n\n", currPC, currPC); }

			if (sp == (MAX_NUM_ELEMENTS - 1)) {
				if (verbose) {
					update_instrStr("POP Stack Underflow", -1, addressingMode, -1);
					update_regChanges(pageAddr, words[pageAddr], TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
					update_regChanges(PSW_SPECIFIER, psw, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
					cycleCount++;
				}
				hlt();
				//die("Fatal error: stack underflow!");
			}

			int temp = words[(sp + 1)];
			if (verbose) {
				update_regChanges(SP_SPECIFIER, sp, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE);
			}
			sp++;
			currPC = temp;
			if (verbose) {
				update_regChanges(sp, SP_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
				update_regChanges(sp, temp, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
				update_regChanges(temp, PC_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
				update_instrStr("RET", -1, -1, -1);
			}
			cycleCount++;

			if (sp == (MAX_NUM_ELEMENTS - 1)) {
				stackUnderflow = ASCII_ONE;
	//			if (verbose) {											// enable for print alert
	//				char *spHex = decToHex(sp);
	//				char *splHex = decToHex(spl);
	//				strcat(regChanges, "\nStack Pointer = 0x");
	//				strcat(regChanges, spHex);
	//				strcat(regChanges, "; Stack Limit = 0x");
	//				strcat(regChanges, splHex);
	//			}
			}
		}
		else {
			// unsupported op
			//die("Unsupported instruction! Now halting/exiting");
		}
	}

	void register_memory_reference_instruction(int opcode, int regSpecifier, int addressingMode, int pageMode, int pageAddr) {
		cycleCount++;
		if (opcode == RMRI_ADD) 		{			rmri_add(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_SUB) 	{			rmri_sub(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_MUL) 	{			rmri_mul(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_DIV) 	{			rmri_div(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_OR) 	{			rmri_or(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_AND) 	{			rmri_and(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_XOR) 	{			rmri_xor(regSpecifier, addressingMode, pageMode, pageAddr);			}
		else if (opcode == RMRI_LOAD) 	{			rmri_load(regSpecifier, addressingMode, pageMode, pageAddr);		}
		else if (opcode == RMRI_STORE)  {			rmri_store(regSpecifier, addressingMode, pageMode, pageAddr);		}
		else {
			// unsupported op
			//die("Unsupported instruction! Now halting/exiting");
		}
	}

	void non_register_memory_reference_instruction(int opcode, int subOpcode, int addressingMode, int pageMode, int pageAddr) {
		cycleCount++;
		if (opcode == 11 && subOpcode == 0) {		isz(addressingMode, pageMode, pageAddr);	}
		else if (opcode == 11 && subOpcode == 1) {	jmp(addressingMode, pageMode, pageAddr);	}
		else if (opcode == 11 && subOpcode == 2) {	call(addressingMode, pageMode, pageAddr);	}
		else if (opcode == 12 && subOpcode == 0) {	push(addressingMode, pageMode, pageAddr);	}
		else if (opcode == 12 && subOpcode == 1) {	pop(addressingMode, pageMode, pageAddr);	}
		else {
			// unsupported op
			//die("Unsupported instruction! Now halting/exiting");
		}
	}

	void register_to_register_instruction(int subOpcode, int regSpecifier1, int regSpecifier2, int regSpecifier3) {
		cycleCount++;
		incrCurrPC();
		if (subOpcode == RTRI_MOD) 		{			rtri_mod(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_ADD) {			rtri_add(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_SUB) {			rtri_sub(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_MUL) {			rtri_mul(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_DIV) {			rtri_div(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_AND) {			rtri_and(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_OR) 	{			rtri_or(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else if (subOpcode == RTRI_XOR) {			rtri_xor(regSpecifier1, regSpecifier2, regSpecifier3);			}
		else {
			// unsupported op
			//die("Unsupported instruction! Now halting/exiting");
		}
//		if (regSpecifier1 != PC_SPECIFIER) {
//			incrCurrPC();
//		}
	}

	void non_memory_register_instruction(int regSpecifier) {
		cycleCount++;
		char *currInstr = decToBin(words[currPC]);
		if (currInstr[6] == ASCII_ONE) {			sm(regSpecifier);			}
		if (currInstr[7] == ASCII_ONE) {
			if (skipFlag == ASCII_ZERO) {
				sz(regSpecifier);
			}
			else {
				update_instrStr("SZ", regSpecifier, -1, -1);
			}
		}
		if (currInstr[8] == ASCII_ONE) {
			if (skipFlag == ASCII_ZERO) {
				snl(regSpecifier);
			}
			else {
				update_instrStr("SNL", -1, -1, -1);
			}
		}
		if (currInstr[9] == ASCII_ONE) {			rss();						}
		if (currInstr[10] == ASCII_ONE) {			cl(regSpecifier);			}
		if (currInstr[11] == ASCII_ONE) {			cll();						}
		if (currInstr[12] == ASCII_ONE) {			cm(regSpecifier);			}
		if (currInstr[13] == ASCII_ONE) {			cml();						}
		if (currInstr[14] == ASCII_ONE) {			dc(regSpecifier);			}
		if (currInstr[15] == ASCII_ONE) {			ic(regSpecifier);			}
		if (skipFlag == ASCII_ONE) {
			int copy = currPC;
			if (copy == (MAX_NUM_ELEMENTS - 2)) {
				copy = 0;
			}
			else {
				copy += 2;
			}
			if (verbose) {	update_regChanges(copy, PC_SPECIFIER, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);	}
		}
		free(currInstr);
		incrCurrPC();
	}

	int getRegisterSpecifierLen2() {
		char *blah = decToBin(words[currPC]);
		char tempArr[3];
		char *tempArrPtr = tempArr;
		tempArrPtr[0] = blah[4];
		tempArrPtr[1] = blah[5];
		tempArrPtr[2] = '\0';
		free(blah);
		int temp = binToDec(tempArrPtr);
		if (temp == A_REG_SPECIFIER) { 				return A_REG_SPECIFIER; 	}
		else if (temp == B_REG_SPECIFIER) { 		return B_REG_SPECIFIER; 	}
		else if (temp == C_REG_SPECIFIER) { 		return C_REG_SPECIFIER; 	}
		else { 										return D_REG_SPECIFIER; 	}
	}

	int getRegisterSpecifierLen3() {
		char *blah = decToBin(words[currPC]);
		char tempArr[4];
		char *tempArrPtr = tempArr;
		tempArrPtr[0] = blah[4];
		tempArrPtr[1] = blah[5];
		tempArrPtr[2] = blah[6];
		tempArrPtr[3] = '\0';
		free(blah);
		int temp = binToDec(tempArrPtr);
		if (temp == A_REG_SPECIFIER) { 				return A_REG_SPECIFIER; 	}
		else if (temp == B_REG_SPECIFIER) { 		return B_REG_SPECIFIER; 	}
		else if (temp == C_REG_SPECIFIER) { 		return C_REG_SPECIFIER; 	}
		else if (temp == D_REG_SPECIFIER) { 		return D_REG_SPECIFIER; 	}
		else if (temp == PC_SPECIFIER) { 			return PC_SPECIFIER; 		}
		else if (temp == PSW_SPECIFIER) { 			return PSW_SPECIFIER; 		}
		else if (temp == SP_SPECIFIER) { 			return SP_SPECIFIER; 		}
		else { 										return SPL_SPECIFIER; 		}
	}

	int getSubOpcodeLen2() {
		char *blah = decToBin(words[currPC]);
		char opcodeArr[3];
		char *opcodeArrPtr = opcodeArr;
		opcodeArrPtr[0] = blah[4];
		opcodeArrPtr[1] = blah[5];
		opcodeArrPtr[2] = '\0';
		free(blah);
		return (binToDec(opcodeArrPtr));
	}

	int getSubOpcodeLen3() {
		char *blah = decToBin(words[currPC]);
		char opcodeArr[4];
		char *opcodeArrPtr = opcodeArr;
		opcodeArrPtr[0] = blah[4];
		opcodeArrPtr[1] = blah[5];
		opcodeArrPtr[2] = blah[6];
		opcodeArrPtr[3] = '\0';
		free(blah);
		return (binToDec(opcodeArrPtr));
	}

	int getOpcode() {
		char *blah = decToBin(words[currPC]);
		char opcodeArr[5];
		char *opcodePtr = opcodeArr;
		opcodePtr[0] = blah[0];
		opcodePtr[1] = blah[1];
		opcodePtr[2] = blah[2];
		opcodePtr[3] = blah[3];
		opcodePtr[4] = '\0';
		free(blah);
		return (binToDec(opcodePtr));
	}

	int get_DI_mode() {
		char *blah = decToBin(words[currPC]);
		if (blah[6] == ASCII_ZERO) {
			free(blah);
			return DIRECT;
		}
		free(blah);
		return INDIRECT;
	}

	int getPageMode() {
		char *blah = decToBin(words[currPC]);
		if (blah[7] == ASCII_ZERO) {
			free(blah);
			return ZERO_PAGE;
		}
		free(blah);
		return CURRENT_PAGE;
	}

	int getPageAddr(int pageMode) {
		char *blah = decToBin(words[currPC]);
		if (pageMode == CURRENT_PAGE) {
			char *currPCBin = decToBin(currPC);
			char temp[(REG_LEN + 1)];
			int i;
			for (i = 0; i < 8; i++) {
				temp[i] = currPCBin[i];
			}
			for (; i < REG_LEN; i++) {
				temp[i] = blah[i];
			}
			temp[REG_LEN] = '\0';
			char *tempPtr = temp;
			free(blah);
			free(currPCBin);
			return (binToDec(tempPtr));
		}
		else {
			char temp[9];
			int i;
			int start = 8;
			for (i = 0; i < 8; i++) {
				temp[i] = blah[start];
				start++;
			}
			temp[8] = '\0';
			char *tempPtr = temp;
			free(blah);
			return (binToDec(tempPtr));
		}
	}

	void process() {
		prevPC = currPC;
		instr = words[currPC];
		char *temp = decToBin(instr);
		if (debug) { printf("Current instruction: 0x%04X/%d\n", instr, instr); }

		int opcode = getOpcode();
		int subOpcode = getSubOpcodeLen2();
		int regSpecifier = getRegisterSpecifierLen2();
		int addressingMode, pageMode, pageAddr;

		if (opcode == NRNMI) {
			free(temp);
			addressingMode = get_DI_mode();
			pageMode = getPageMode();
			pageAddr = getPageAddr(pageMode);

			non_register_non_memory_instruction(addressingMode, pageAddr);
		}
		else if (opcode >= RMRI_LOWER_BOUND && opcode <= RMRI_UPPER_BOUND) {
			addressingMode = get_DI_mode();
			pageMode = getPageMode();
			pageAddr = getPageAddr(pageMode);

			register_memory_reference_instruction(opcode, regSpecifier, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == IO) {
			char deviceBinStr[8];
			deviceBinStr[0] = temp[6];
			deviceBinStr[1] = temp[7];
			deviceBinStr[2] = temp[8];
			deviceBinStr[3] = temp[9];
			deviceBinStr[4] = temp[10];
			deviceBinStr[5] = temp[11];
			deviceBinStr[6] = temp[12];
			deviceBinStr[7] = '\0';
			char *deviceBinStrPtr = deviceBinStr;
			int deviceNum = binToDec(deviceBinStrPtr);

			char functionCodeBinStr[4];
			functionCodeBinStr[0] = temp[13];
			functionCodeBinStr[1] = temp[14];
			functionCodeBinStr[2] = temp[15];
			functionCodeBinStr[3] = '\0';
			char *functionCodeBinStrPtr = functionCodeBinStr;
			int functionCode = binToDec(functionCodeBinStrPtr);

			io(regSpecifier, deviceNum, functionCode);
		}
		else if (opcode >= NRMRI_LOWER_BOUND && opcode <= NRMRI_UPPER_BOUND) {
			subOpcode = regSpecifier;
			addressingMode = get_DI_mode();
			pageMode = getPageMode();
			pageAddr = getPageAddr(pageMode);

			non_register_memory_reference_instruction(opcode, subOpcode, addressingMode, pageMode, pageAddr);
		}
		else if (opcode == RTRI) {
			subOpcode = getSubOpcodeLen3();
			char temp1[4];
			char temp2[4];
			char temp3[4];
			temp1[0] = temp[7];
			temp1[1] = temp[8];
			temp1[2] = temp[9];
			temp1[3] = '\0';
			temp2[0] = temp[10];
			temp2[1] = temp[11];
			temp2[2] = temp[12];
			temp2[3] = '\0';
			temp3[0] = temp[13];
			temp3[1] = temp[14];
			temp3[2] = temp[15];
			temp3[3] = '\0';
			char *temp1Ptr = temp1;
			char *temp2Ptr = temp2;
			char *temp3Ptr = temp3;
			int regSpecifier1 = binToDec(temp1Ptr);
			int regSpecifier2 = binToDec(temp2Ptr);
			int regSpecifier3 = binToDec(temp3Ptr);

			register_to_register_instruction(subOpcode, regSpecifier1, regSpecifier2, regSpecifier3);
		}
		else if (opcode == NMRI) {
			non_memory_register_instruction(regSpecifier);
		}
		else {
			//die("Unsupported instruction! Now halting/exiting");
		}

		if (opcode != NRNMI) {			// freed in (opcode == NRNMI) b/c that instruction handles halts
			free(temp);
		}

		if (verbose) {
			printVerbose();
			memset(instrStr, 0, sizeof(*instrStr));
			memset(regChanges, 0, sizeof(*regChanges));
			instrStr[0] = '\0';
			regChanges[0] = '\0';
		}
	}

	void setWords(int *buffer, int actualSize) {
		// take everything read in from the binary object file
		// fill words[] with that data, as according to the spec sheet

		if (debug) { printf("Magic \"number\" is %c%c%c%c\n\n", buffer[0], buffer[1], buffer[2], buffer[3]); }

		// check for magic "number"
		// add in handling for lower case later?
		if (buffer[0] != 'O' || buffer[1] != 'B' || buffer[2] != 'J' || buffer[3] != 'G') {
			die("Object file did not begin with the magic \"number\"; halting/exiting");
		}
		if (actualSize <= 9) {				// 'OBJG', EP, block size, starting address
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

				currBlockAddr++;
			}
		}

		if (debug) { printf("PC is set to start at 0x%04X/%d\n\n", currPC, currPC); }

		if (currPC == -1) {
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
			die("Object file is empty; halting/exiting");
		}

		buffer = calloc(size, sizeof(int));
		actualSize = 0;
		counter = 0;
		while ((byte = getc(file)) != EOF) {
			buffer[counter] = byte;
			actualSize++;
			counter++;
		}

		fclose(file);

		if (readFileDebug) { printf("Read in %d data bytes\nBuffer contents: [%d", (actualSize - 9), buffer[0]); int i; for(i = 1; i < actualSize; i++) { printf(", %d", buffer[i]); } printf("]\n\n"); }

//		if ((actualSize - 9) % 2 != 0) {
//			free(buffer);
//			freeWords();
//			die("Odd number of bytes encountered; halting/exiting");
//		}

		setWords(buffer, actualSize);

		free(buffer);
	}

	void initialize() {
		// "initialize" the "components/machine"
		// should only be called once in entire program

		int i;
		for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
			words[i] = -1;
		}

		instr = 0;

		aReg = 0;
		bReg = 0;
		cReg = 0;
		dReg = 0;

		link = ASCII_ZERO;
		skipFlag = ASCII_ZERO;

		sp = -1;
		spl = 0;
		psw = 1;

		instrStr = calloc(1000, sizeof(char));
		regChanges = calloc(1000, sizeof(char));
		instrStr[0] = '\0';
		regChanges[0] = '\0';

		stackOverflow = ASCII_ZERO;
		stackUnderflow = ASCII_ZERO;

		prevPC = 0;
	}

	int main (int argc, char **argv) {

		initialize();

		if (manDebug) {
			readFile("spover.obj");

			while (currPC >= 0 && currPC < MAX_NUM_ELEMENTS) {
				process();
			}

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
					process();
				}
			}
			else {
				die("No object file specified!");
			}
		}
		return 0;
	}
