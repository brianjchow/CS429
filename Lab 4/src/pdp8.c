/*
 ============================================================================
 Name        : Brian Chow (bc23784)
 Program	 : pdp8.c
 Description : see readme
 ============================================================================
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
#define SIZEOF_PAGE 128
#define MAX_NUM_ELEMENTS 4096
#define DIRECT 0
#define INDIRECT 1
#define ZERO_PAGE 0
#define CURRENT_PAGE 1

#define AND 0
#define TAD 1
#define ISZ 2
#define DCA 3
#define JMS 4
#define JMP 5
#define IOT 6

boolean debug = FALSE;
boolean readFileDebug = TRUE;
boolean verbose = FALSE;

int pcStart = -1;
int currPC = -1;
char *reg;
char link;
char *words[MAX_NUM_ELEMENTS];
char *instr;
char skipFlag;						// for use with SMA, SZA, SNL, RSS

FILE *file;
size_t buflen = 20;					// size of buffer during file read-in
signed long long cycleCount = 0;	// for use with verbose; indicates how long (in cycles) an instruction took
signed long long instrCount = 1;	// primarily for use with debugging when verbose == TRUE

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
		int i;
		for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
			if (words[i] != NULL) {
				free(words[i]);
			}
		}
	}

	void die(char *reason) {
		fprintf(stderr, "*** %s ***\n\n", reason);
		freeWords();
		exit(-1);
	}

	void hlt() {
		// halt
		if (debug) { printf("Instruction was HLT; now halting/exiting"); }
		freeWords();
		exit(0);
	}

	char* decToHex(int n) {
		if (n == 0) {
			char *out = calloc(4, sizeof(char));
			out[0] = ASCII_ZERO;
			out[1] = ASCII_ZERO;
			out[2] = ASCII_ZERO;
			out[3] = '\0';
			return out;
		}
		else {
			int i, j, r[12];
			i = 0;
			j = 0;
			while (n > 0) {
				r[i] = n % 16;
				n /= 16;
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
				char *temp = calloc(4, sizeof(char));
				temp[0] = ASCII_ZERO;
				temp[1] = ASCII_ZERO;
				temp[2] = out[0];
				temp[3] = '\0';
				free(out);
				return temp;
			}
			else if (strlen(out) == 2) {
				char *temp = calloc(4, sizeof(char));
				temp[0] = ASCII_ZERO;
				temp[1] = out[0];
				temp[2] = out[1];
				temp[3] = '\0';
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

		if (strlen(out) < 12) {
			char *temp = calloc(13, sizeof(char));
			temp[0] = '\0';
			int padding = 12 - strlen(out);
			for (i = 0; i < padding; i++) {
				strcat(temp, "0");
			}
			int start = 0;
			for (; i < 12; i++) {
				temp[i] = out[start];
				start++;
			}
			temp[12] = '\0';
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
		if (strlen(temp2) < 12) {
			char* temp3 = calloc(13, sizeof(char));
			temp3[0] = '\0';
			int padding = 12 - strlen(temp2);
			int i;
			for (i = 0; i < padding; i++) {
				strcat(temp3, "0");
			}
			int start = 0;
			for (; i < 12; i++) {
				temp3[i] = temp2[start];
				start++;
			}
			temp3[12] = '\0';
			free(temp1);
			free(temp2);
			return temp3;
		}
		else {
			free(temp1);
			return temp2;
		}
	}

	void set(int address, char *word) {
		if (address < 0 || address > MAX_NUM_ELEMENTS || word == NULL || strlen(word) != 3) {
			die("Failed precondition check in set()");
		}

		if (readFileDebug) { printf("Attempting to set index/address %d with word \'%s\'...\n\n", address, word); }

		char *copy = malloc(4 * sizeof(int));
		copy[0] = word[0];
		copy[1] = word[1];
		copy[2] = word[2];
		copy[3] = '\0';

		words[address] = copy;
	}

	void toString() {
		int i;
		printf("\nPrinting contents of *words, format [Address, Word]\n\t");
		for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
			if (words[i] != NULL) {
				printf("[%d, %s] ", i, words[i]);
			}
		}
		printf("\n");
	}

	void printVerbose(int opcode, int addressingMode, int pageMode, int pageAddr) {

		char *instrStr;

		if (addressingMode == INDIRECT && opcode != IOT && opcode != 7) {
			cycleCount++;
		}
		if (opcode == AND || opcode == TAD || opcode == ISZ || opcode == DCA || opcode == JMS) {
			cycleCount += 2;
		}
		if (opcode == JMP || opcode == IOT || opcode == 7) {
			cycleCount++;
		}

		if (opcode == AND) { if (addressingMode == INDIRECT) { instrStr = "AND I"; } else { instrStr = "AND"; } }
		else if (opcode == TAD) { if (addressingMode == INDIRECT) { instrStr = "TAD I"; } else { instrStr = "TAD"; } }
		else if (opcode == ISZ) { if (addressingMode == INDIRECT) { instrStr = "ISZ I"; } else { instrStr = "ISZ"; } }
		else if (opcode == DCA) { if (addressingMode == INDIRECT) { instrStr = "DCA I"; } else { instrStr = "DCA"; } }
		else if (opcode == JMS) { if (addressingMode == INDIRECT) { instrStr = "JMS I"; } else { instrStr = "JMS"; } }
		else if (opcode == JMP) { if (addressingMode == INDIRECT) { instrStr = "JMP I"; } else { instrStr = "JMP"; } }
		else if (opcode == IOT) {
			char device[7];
			int start = 3;
			int i;
			for (i = 0; i < 6; i++) {
				device[i] = instr[start];
				start++;
			}
			device[6] = '\0';
			char *devicePtr = device;
			int deviceNum = binToDec(devicePtr);
			if (deviceNum == 3) {
				instrStr = "IOT 3";
			}
			else if (deviceNum == 4) {
				instrStr = "IOT 4";
			}
//			else {
//				hlt();
//			}
		}
		else {
			char temp[50];
			instrStr = temp;
			instrStr[0] = '\0';
			if (instr[3] == ASCII_ZERO) {
				if (instr[4] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "CLA"); } else { strcat(instrStr, " CLA"); } }
				if (instr[5] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "CLL"); } else { strcat(instrStr, " CLL"); } }
				if (instr[6] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "CMA"); } else { strcat(instrStr, " CMA"); } }
				if (instr[7] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "CML"); } else { strcat(instrStr, " CML"); } }
				if (instr[11] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "IAC"); } else { strcat(instrStr, " IAC"); } }
				if (instr[8] == ASCII_ONE && instr[10] == ASCII_ZERO) { if (strlen(instrStr) == 0) { strcat(instrStr, "RAR"); } else { strcat(instrStr, " RAR"); } }
				if (instr[9] == ASCII_ONE && instr[10] == ASCII_ZERO) { if (strlen(instrStr) == 0) { strcat(instrStr, "RAL"); } else { strcat(instrStr, " RAL"); } }
				if (instr[8] == ASCII_ONE && instr[10] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "RTR"); } else { strcat(instrStr, " RTR"); } }
				if (instr[9] == ASCII_ONE && instr[10] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "RTL"); } else { strcat(instrStr, " RTL"); } }
			}
			else {
				if (instr[5] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "SMA"); } else { strcat(instrStr, " SMA"); } }
				if (instr[6] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "SZA"); } else { strcat(instrStr, " SZA"); } }
				if (instr[7] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "SNL"); } else { strcat(instrStr, " SNL"); } }
				if (instr[8] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "RSS"); } else { strcat(instrStr, " RSS"); } }
				if (instr[4] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "CLA"); } else { strcat(instrStr, " CLA"); } }
				if (instr[9] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "OSR"); } else { strcat(instrStr, " OSR"); } }
				if (instr[10] == ASCII_ONE) { if (strlen(instrStr) == 0) { strcat(instrStr, "HLT"); } else { strcat(instrStr, " HLT"); } }
			}
		}

		if (debug) {
			fprintf(stderr, "Instruction # %lld: time %lld PC=0x%03X instruction = 0x%03X (%s), rA = 0x%03X, rL = %d\n", (instrCount + 1), cycleCount, currPC, binToDec(instr), instrStr, binToDec(reg), (link - ASCII_ZERO));
		}
		else {
			fprintf(stderr, "Time %lld: PC=0x%03X instruction = 0x%03X (%s), rA = 0x%03X, rL = %d\n", cycleCount, currPC, binToDec(instr), instrStr, binToDec(reg), (link - ASCII_ZERO));
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

	void parseLine(char *string) {

		int indexOfFirstColon = -1;
		char *ptrToFirstColon = strchr(string, ':');

		if (ptrToFirstColon == NULL) {
			die("Malformed line encountered in obj file; exiting/halting");
		}

		indexOfFirstColon = ptrToFirstColon - string;

		if (readFileDebug) { printf("Index of first colon in string \'%s\' is: %d\n", string, indexOfFirstColon); }

		if (strlen(string) != 7 && strlen(string) != 8) {
			die("Malformed line encountered in obj file; exiting/halting");
		}

		if (indexOfFirstColon != 2 && indexOfFirstColon != 3) {
			die("Malformed line encountered in obj file; exiting/halting");
		}

		if (string[indexOfFirstColon + 1] != ' ') {
			die("Malformed line encountered in obj file; exiting/halting");
		}

		if (indexOfFirstColon == 2 && pcStart == -1) {
			if (string[0] != 'E' || string[1] != 'P') {
				die("Malformed line encountered in obj file; exiting/halting");
			}
			char startAddress[4];
			startAddress[0] = string[4];
			startAddress[1] = string[5];
			startAddress[2] = string[6];
			startAddress[3] = '\0';
			char *startAddressPtr = startAddress;
			pcStart = hexToDec(startAddressPtr);
		}
		else if (indexOfFirstColon == 2 && pcStart != -1) {
			die("Multiple EP lines detected in obj file; exiting/halting");
		}
		else {					// indexOfFirstColon == 3

			int i = 0;
			while (i < 8) {
				if (islower((int) string[i])) {				// to handle lowercase hex letters
					string[i] = toupper((int) string[i]);
				}
				if ((string[i] < ASCII_ZERO || string[i] > 57) && (string[i] < 65 || string[i] > 70)) {		// char is not a hex letter
					die("Malformed line encountered in obj file; exiting/halting");
				}
				i++;
				if (i == 3) {		// skip the colon and space
					i = 5;
				}
			}

			char address[4];
			char word[4];

			address[0] = string[0];
			address[1] = string[1];
			address[2] = string[2];
			address[3] = '\0';

			word[0] = string[5];
			word[1] = string[6];
			word[2] = string[7];
			word[3] = '\0';

			char *addressPtr = address;
			char *wordPtr = word;

			if (readFileDebug) { printf("Obtained address \'%s\' and word \'%s\' from line \'%s\'\n", addressPtr, word, string); }

			set(hexToDec(addressPtr), wordPtr);
		}
	}

	int getOpcode() {
		char temp[4];
		temp[0] = instr[0];
		temp [1] = instr[1];
		temp[2] = instr[2];
		temp[3] = '\0';
		char *tempPtr = temp;
		if (debug) { printf("Opcode is %s\n", tempPtr); }
		return (binToDec(tempPtr));
	}

	int get_DI_mode() {
		if (instr[3] == ASCII_ZERO) {
			if (debug) { printf("Addressing mode is bit 3 is 0 is DIRECT\n"); }
			return DIRECT;
		}
		if (debug) { printf("Addressing mode is bit 3 is 1 is INDIRECT\n"); }
		return INDIRECT;
	}

	int getPageMode() {
		if (instr[4] == ASCII_ZERO) {
			if (debug) { printf("Page number is bit 4 is 0 is ZERO\n"); }
			return ZERO_PAGE;
		}
		if (debug) { printf("Page number is bit 4 is 1 is CURRENT PAGE\n"); }
		return CURRENT_PAGE;
	}

	int getPageAddr(int pageMode) {
		if (pageMode == CURRENT_PAGE) {
			char *currPCBin = decToBin(currPC);
			if (debug) { printf("Current PC is %d is %s\n", currPC, currPCBin); }
			char temp[13];
			temp[0] = currPCBin[0];
			temp[1] = currPCBin[1];
			temp[2] = currPCBin[2];
			temp[3] = currPCBin[3];
			temp[4] = currPCBin[4];

			int i;
			for (i = 5; i < 12; i++) {
				temp[i] = instr[i];
			}
			temp[12] = '\0';
			char *tempPtr = temp;
			if (debug) { printf("Page address is %s\n", tempPtr); }
			free(currPCBin);
			return (binToDec(tempPtr));
		}
		else {
			char temp[8];
			int i;
			int start = 5;
			for (i = 0; i < 7; i++) {
				temp[i] = instr[start];
				start++;
			}
			temp[7] = '\0';
			char *tempPtr = temp;
			if (debug) { printf("Page address is %s\n", tempPtr); }
			return (binToDec(tempPtr));
		}
	}

	int indexOfPageModePageAddr(int pageAddr) {
		char *pageAddrBin = decToBin(pageAddr);
		char pageNumBin[6];
		pageNumBin[0] = pageAddrBin[0];
		pageNumBin[1] = pageAddrBin[1];
		pageNumBin[2] = pageAddrBin[2];
		pageNumBin[3] = pageAddrBin[3];
		pageNumBin[4] = pageAddrBin[4];
		pageNumBin[5] = '\0';
		char *pageNumBinPtr = pageNumBin;
		int pageNum = binToDec(pageNumBinPtr);

		char addrInPage[8];
		addrInPage[0] = pageAddrBin[5];
		addrInPage[1] = pageAddrBin[6];
		addrInPage[2] = pageAddrBin[7];
		addrInPage[3] = pageAddrBin[8];
		addrInPage[4] = pageAddrBin[9];
		addrInPage[5] = pageAddrBin[10];
		addrInPage[6] = pageAddrBin[11];
		addrInPage[7] = '\0';
		char *addrInPagePtr = addrInPage;
		int numInPage = binToDec(addrInPagePtr);

		int startIndex;
		int stopIndex = (pageNum + 1) * 128 - 1;
		int counter = 0;
		for (startIndex = pageNum * 128; startIndex < stopIndex; startIndex++) {
			if (counter == numInPage) {
				pageAddr = startIndex;
			}
			counter++;
		}
		free(pageAddrBin);
		if (debug) { printf("Z/C bit was 1 was CURRENT PAGE; calculated index (address) was %d\n", pageAddr); }
		return pageAddr;
	}

	void cla() {
		// clear the A register; set it to zero

		int i;
		for (i = 0; i < 12; i++) {
			reg[i] = ASCII_ZERO;
		}
	}

	void and(int addressingMode, int pageMode, int pageAddr) {
		// logical AND

		int regInt = binToDec(reg);
		int wordInt;

		if (words[pageAddr] != NULL) {
			if (addressingMode == DIRECT) {
				wordInt = hexToDec(words[pageAddr]);
			}
			else {
				int effAddrInt = hexToDec(words[pageAddr]);
				if (words[effAddrInt] != NULL) {
					wordInt = hexToDec(words[effAddrInt]);
				}
				else {
					wordInt = 0;
				}
			}
		}
		else {
			wordInt = 0;
		}

		int andedInt = regInt & wordInt;
		char *anded = decToBin(andedInt);

		int i;
		int start = 0;
		if (strlen(anded) != 12) {
			int padding = 12 - strlen(anded);
			for (i = 0; i < padding; i++) {
				reg[i] = ASCII_ZERO;
			}
		}
		else {
			i = 0;
		}
		for (; i < 12; i++) {
			reg[i] = anded[start];
			start++;
		}
		free(anded);
	}

	void tad(int addressingMode, int pageMode, int pageAddr) {
		// two's complement add

		int regInt = binToDec(reg);
		int wordInt;

		if (words[pageAddr] != NULL) {
			if (addressingMode == DIRECT) {
				wordInt = hexToDec(words[pageAddr]);
			}
			else {
				int effAddrInt = hexToDec(words[pageAddr]);
				if (words[effAddrInt] != NULL) {
					wordInt = hexToDec(words[effAddrInt]);
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
			added = decToBin(addedInt % MAX_NUM_ELEMENTS);		// wrap around to 0; complement link bit
			if (link == ASCII_ZERO) {
				link = ASCII_ONE;
			}
			else {
				link = ASCII_ZERO;
			}
		}
		else {
			added = decToBin(addedInt);
		}
		int i = 0;
		for (; i < 12; i++) {
			reg[i] = added[i];
		}
		free(added);
	}

	void isz(int addressingMode, int pageMode, int pageAddr) {
		// increment and skip if zero
		// contents of the effective address are incremented by one and put back in the same memory location
		// if result of increment is zero, the next instruction is skipped

		if (addressingMode == DIRECT) {
			if (words[pageAddr] == NULL) {
				// do something?
			}
			else {
				char *temp = words[pageAddr];
				int tempInt = hexToDec(temp);
				if (tempInt == (MAX_NUM_ELEMENTS - 1)) {
					tempInt = 0;

					skipFlag = ASCII_ONE;
				}
				else {
					tempInt++;
				}
				char *incremented = decToHex(tempInt);
				if (debug) { printf("Instruction was ISZ; previous contents were %s, new contents are %s\n", words[pageAddr], incremented); }
				free(words[pageAddr]);
				words[pageAddr] = incremented;
			}
		}
		else {
			// indirect addressing
			if (words[pageAddr] == NULL) {
				// do something?
			}
			else {
				int effAddrInt = hexToDec(words[pageAddr]);
				if (words[effAddrInt] == NULL) {
					// do something?
				}
				else {
					int tempInt = hexToDec(words[effAddrInt]);
					if (tempInt == (MAX_NUM_ELEMENTS - 1)) {
						tempInt = 0;
						skipFlag = ASCII_ONE;
					}
					else {
						tempInt++;
					}
					char *incremented = decToHex(tempInt);
					if (debug) { printf("Instruction was ISZ; previous contents were %s, new contents are %s\n", words[pageAddr], incremented); }
					free(words[effAddrInt]);
					words[effAddrInt] = incremented;
				}
			}
		}
	}

	void dca(int addressingMode, int pageMode, int pageAddr) {
		// store the contents of the A register in the effective address; clear the A register

		int regInt = binToDec(reg);
		char *temp = decToHex(regInt);

		if (addressingMode == DIRECT) {
			if (words[pageAddr] != NULL) {
				free(words[pageAddr]);
			}
			words[pageAddr] = temp;
		}
		else {
			// indirect addressing

			int effAddrInt = hexToDec(words[pageAddr]);
			if (words[effAddrInt] != NULL) {
				free(words[effAddrInt]);
			}
			words[effAddrInt] = temp;
		}
		cla();
	}

	void jms(int opcode, int addressingMode, int pageMode, int pageAddr) {
		// the address of the next location (PC + 1) is stored at the effective address
		// program counter is set to the effective address plus 1

		if (addressingMode == DIRECT) {
			int pcPlusOne = currPC + 1;
			char *temp = decToHex(pcPlusOne);
			if (words[pageAddr] != NULL) {
				free(words[pageAddr]);
			}
			words[pageAddr] = temp;
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			currPC = pageAddr + 1;
		}
		else {
			// indirect addressing

			int effAddrInt = hexToDec(words[pageAddr]);
			int pcPlusOne = currPC + 1;
			char *temp = decToHex(pcPlusOne);
			if (words[effAddrInt] != NULL) {
				free(words[effAddrInt]);
			}
			words[effAddrInt] = temp;
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			currPC = effAddrInt + 1;
		}
		if (currPC > (MAX_NUM_ELEMENTS - 1) || currPC < 0) {
			//die("Fatal error: PC out of bounds");
			hlt();
		}
		if (debug) { printf("Operation was JMS; PC now set to %d\n", currPC); }
	}

	void jmp(int opcode, int addressingMode, int pageMode, int pageAddr) {
		// program counter is set to the effective address

		if (addressingMode == DIRECT) {
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			currPC = pageAddr;
		}
		else {
			// indirect addressing
			int effAddrInt = hexToDec(words[pageAddr]);
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			currPC = effAddrInt;
		}
		if (currPC > (MAX_NUM_ELEMENTS - 1) || currPC < 0) {
			//die("Fatal error: PC out of bounds");
			hlt();
		}
		if (debug) { printf("Instruction was JMP; program counter set at index %d (address)\n", currPC); }
	}

	void cll() {
		// clear the link bit
		link = ASCII_ZERO;
	}

	void cma() {
		// complement the A register (bit by bit; change 0 to 1 and 1 to 0)

		if (debug) { printf("Executing cma(); reg before is "); int i; for (i = 0; i < 12; i++) { printf("%c", reg[i]); } printf("\n"); }
		int i;
		for (i = 0; i < 12; i++) {
			if (reg[i] == ASCII_ZERO) {
				reg[i] = ASCII_ONE;
			}
			else {
				reg[i] = ASCII_ZERO;
			}
		}
		if (debug) { printf("Executing cma(); reg after is "); int i; for (i = 0; i < 12; i++) { printf("%c", reg[i]); } printf("\n"); }
	}

	void cml() {
		// complement the link bit

		if (link == ASCII_ZERO) {
			link = ASCII_ONE;
		}
		else {
			link = ASCII_ZERO;
		}
	}

	void rar() {
		// rotate right by 1 bit

		char copyOfLink = link;
		link = reg[11];
		int i;
		for (i = 10; i >= 0; i--) {
			reg[i + 1] = reg[i];
		}
		reg[0] = copyOfLink;
	}

	void ral() {
		// rotate left by 1 bit

		char copyOfLink = link;
		link = reg[0];
		int i;
		for (i = 1; i < 12; i++) {
			reg[i - 1] = reg[i];
		}
		reg[11] = copyOfLink;
	}

	void rtr() {
		// rotate right by 2 bits
		rar();
		rar();
	}

	void rtl() {
		// rotate left by 2 bits
		ral();
		ral();
	}

	void iac() {
		// add 1 to the A register

		int regInt = binToDec(reg);
		if (regInt == (MAX_NUM_ELEMENTS - 1)) {
			cla();
			if (link == ASCII_ZERO) {
				link = ASCII_ONE;
			}
			else {
				link = ASCII_ZERO;
			}
		}
		else {
			regInt++;
			char *temp = decToBin(regInt);

			int i = 0;
			for (; i < 12; i++) {
				reg[i] = temp[i];
			}
			free(temp);
		}
	}

	void sma() {
		// skip on Minus A; if A register is negative, skip next instruction
		if (reg[0] == ASCII_ONE) {
			skipFlag = ASCII_ONE;
		}
		if (debug) { printf("In SMA; skip flag is now %c\n", skipFlag); }
	}

	void sza() {
		// skip on Zero A; if A register == 0, skip next instruction

		if (binToDec(reg) == 0) {
			skipFlag = ASCII_ONE;
		}
		if (debug) { printf("In SZA; skip flag is now %c\n", skipFlag); }
	}

	void snl() {
		// skip on nonzero link; if link bit == 1, skip next instruction

		if (link == ASCII_ONE) {
			skipFlag = ASCII_ONE;
		}
		if (debug) { printf("In SNL; skip flag is now %c\n", skipFlag); }
	}

	void rss() {
		// reverse skip sense; if instr[8] is 1, the SMA, SZA, and SNL subinstrs will skip on the opposite condition

		if (skipFlag == ASCII_ZERO) {
			skipFlag = ASCII_ONE;
		}
		else {
			skipFlag = ASCII_ZERO;
		}
		if (debug) { printf("In RSS; skip flag is now %c\n", skipFlag); }
	}

	void osr() {
		// OR from the switch register; contents of the switch register on the console are ORed into the A register
		// in this program, treat as NOP

		// do nothing
	}

	void iot(int opcode, int addressingMode, int pageMode, int pageAddr) {
		// handle opcode 6
		// for this program
			// ignore the function code
			// if device is 3 (TTY keyboard)
				// do a getchar() to read a character from stdin
				// put that character into the A register
			// if device is 4 (TTY printer)
				// take the low-order 8 bits from the A register; output it as an ASCII character to stdout (putchar())
			// ignore all other devices

		char device[7];
		int start = 3;
		int i;
		for (i = 0; i < 6; i++) {
			device[i] = instr[start];
			start++;
		}
		device[6] = '\0';
		char *devicePtr = device;
		int deviceNum = binToDec(devicePtr);
		if (deviceNum != 3 && deviceNum != 4) {
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			hlt();
		}
		if (deviceNum == 3) {
			int c = getchar();
			cla();
			if (c > (MAX_NUM_ELEMENTS - 1)) {
				c &= (MAX_NUM_ELEMENTS - 1);
			}
			if (c < 0) {							// treats any negative number as EOF
				c = MAX_NUM_ELEMENTS - 1;
			}
			char *temp = decToBin(c);
			int start = 0;
			int i = 12 - strlen(temp);
			for (; i < 12; i++) {
				reg[i] = temp[start];
				start++;
			}
			free(temp);
		}
		if (deviceNum == 4) {
			char *temp = calloc(10, sizeof(char));
			strncpy(temp, reg + 3, 10);
			int tempInt = binToDec(temp);
			free(temp);
			putchar(tempInt);
		}
	}

	void microinstr(int opcode, int addressingMode, int pageMode, int pageAddr) {
		// handle opcode 7

		int i;
		int numInstrsSet = 0;
		for (i = 4; i < 12; i++) {
			if (instr[i] == ASCII_ONE) {
				numInstrsSet++;
			}
		}

		// no operation found
		if (numInstrsSet == 0) {
			if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
			incrCurrPC();
		}
		else {
			// check group number
			if (instr[3] == ASCII_ZERO) {
				// group 1

				// both RAL and RAR are set
				if (instr[8] == ASCII_ONE && instr[9] == ASCII_ONE) {
					if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
					hlt();
				}

				if (instr[4] == ASCII_ONE) {		cla();		}
				if (instr[5] == ASCII_ONE) {		cll();		}
				if (instr[6] == ASCII_ONE) {		cma();		}
				if (instr[7] == ASCII_ONE) {		cml();		}
				if (instr[11] == ASCII_ONE) {		iac();		}
				if (instr[8] == ASCII_ONE && instr[10] == ASCII_ZERO) {		rar();		}
				if (instr[9] == ASCII_ONE && instr[10] == ASCII_ZERO) {		ral();		}
				if (instr[8] == ASCII_ONE && instr[10] == ASCII_ONE) {		rtr();		}
				if (instr[9] == ASCII_ONE && instr[10] == ASCII_ONE) {		rtl();		}

				if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
				incrCurrPC();
			}
			else {
				// group 2

				// low-order bit must always be 0
				if (instr[11] == ASCII_ONE) {
					if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }
					hlt();
				}

				if (instr[5] == ASCII_ZERO && instr[6] == ASCII_ZERO && instr[7] == ASCII_ZERO) { skipFlag = ASCII_ZERO; }
				if (instr[5] == ASCII_ONE) {		sma();		}
				if (instr[6] == ASCII_ONE) {		sza();		}
				if (instr[7] == ASCII_ONE) {		snl();		}
				if (instr[8] == ASCII_ONE) {		rss();		}
				if (instr[4] == ASCII_ONE) {		cla();		}
				if (instr[9] == ASCII_ONE) {		osr();		}
				if (instr[10] == ASCII_ONE) {		if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); } hlt();		}
				if (debug) { printf("Group 2 op; skip flag is %c\n", skipFlag); }

				if (verbose) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }

				if (skipFlag == ASCII_ONE) {
					if (currPC == (MAX_NUM_ELEMENTS - 2)) {
						currPC = 0;
					}
					else if (currPC == (MAX_NUM_ELEMENTS - 1)) {
						currPC = 1;
					}
					else {
						currPC += 2;
					}
					skipFlag = ASCII_ZERO;
				}
				else {
					incrCurrPC();
				}
			}
		}
	}

	void parseInstr(int address) {
		char *temp = words[address];
		char copy[4];
		copy[0] = temp[0];
		copy[1] = temp[1];
		copy[2] = temp[2];
		copy[3] = '\0';
		char *copyPtr = copy;

		char *copyOfInstr = hexToBin(copyPtr);
		if (debug) { printf("Current instruction is %s\nInstruction count #: %lld\nCurrent PC is index (address) %d\nCurrent skip flag is %c (should be 0)\n", copyOfInstr, instrCount, currPC, skipFlag); instrCount++; }
		int i;
		for (i = 0; i < 12; i++) {
			instr[i] = copyOfInstr[i];
		}
		free(copyOfInstr);

		int opcode = getOpcode();
		int addressingMode = get_DI_mode();
		int pageMode = getPageMode();
		int pageAddr = getPageAddr(pageMode);

		if (pageMode == CURRENT_PAGE) {
			pageAddr = indexOfPageModePageAddr(pageAddr);
		}

		if (debug && opcode != 7) { printf("Sending opcode %d, addressing mode %d, page mode %d, page address %d to instruction for processing...\n", opcode, addressingMode, pageMode, pageAddr); }

		if (opcode == AND) {
			and(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == TAD) {
			tad(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == ISZ) {
			isz(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == DCA) {
			dca(addressingMode, pageMode, pageAddr);
		}
		else if (opcode == JMS) {
			jms(opcode, addressingMode, pageMode, pageAddr);
			// DO NOT INCREMENT PC HERE
		}
		else if (opcode == JMP) {
			jmp(opcode, addressingMode, pageMode, pageAddr);
			// DO NOT INCREMENT PC HERE
		}
		else if (opcode == IOT) {
			iot(opcode, addressingMode, pageMode, pageAddr);
		}
		else {
			microinstr(opcode, addressingMode, pageMode, pageAddr);
		}

		if (verbose && opcode != JMP && opcode != JMS && opcode != 7) { printVerbose(opcode, addressingMode, pageMode, pageAddr); }

		if (opcode == AND || opcode == TAD || opcode == ISZ || opcode == DCA || opcode == IOT) {
			incrCurrPC();
		}

		if (debug) { printf("Register is "); int i; for (i = 0; i < 12; i++) { printf("%c", reg[i]); } printf(" (%d), link is %c\n\n", binToDec(reg), link); }
	}

	void readFile(char *filename) {

		file = fopen(filename, "rb");

		if (file == NULL) {
			fprintf(stderr, "Error: cannot open file \'%s\'!\n", filename);
			exit(-1);
		}

		int byte;
		int counter = 0;
		char *buffer = calloc(buflen, sizeof(char));
		if (buffer == NULL) {
			die("Out of memory");
		}
		char *end = buffer + buflen - 1;
		char *bufPtr = buffer;
		int numLinesRead = 0;		// for checking if the file only contained an EP line

		while ((byte = getc(file)) != EOF && bufPtr < end) {
			if (byte == '\n') {
				*bufPtr++ = '\0';
				if (readFileDebug) { printf("Read in line \'%s\' from file\n", buffer); }
				parseLine(buffer);
				memset(buffer, 0, sizeof(*buffer));
				bufPtr = buffer;
				counter = 0;
				numLinesRead++;
			}
			else {
				*bufPtr++ = byte;
				counter++;

				// buffer overflow imminent; skip the rest of this line in the file
				if (counter == buflen - 1 && byte != '\n') {
					while ((byte = getc(file)) != EOF) {
						if (byte == '\n') {
							break;
						}
					}
					memset(buffer, 0, sizeof(*buffer));
					bufPtr = buffer;
					counter = 0;
				}
			}
		}

		fclose(file);
		free(buffer);

		if (pcStart == -1 || numLinesRead == 1) {		// redundant
			freeWords();
			die("Invalid EP line specified or obj file only consists of EP line; halting/exiting");
		}

		if (debug) { printf("PC begins at index (address) %d\n\n", pcStart); }

		currPC = pcStart;

	}

int main(int argc, char **argv) {

	// initialize PDP-8 "components"
	int i;
	for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
		words[i] = NULL;
	}

	char regArr[13];
	char instrArr[13];
	reg = regArr;
	instr = instrArr;
	for (i = 0; i < 12; i++) {
		reg[i] = ASCII_ZERO;
		instr[i] = ASCII_ZERO;
	}
	reg[12] = '\0';
	instr[12] = '\0';
	link = ASCII_ZERO;
	skipFlag = ASCII_ZERO;

	// if argc is greater than 3, all obj files after the first will be ignored

// enable for command line handling

//	boolean filenameNotGiven = TRUE;
//	while (argc > 1) {
//		argc--, argv++;
//		if (**argv == '-') {
//			scanArgs(*argv);
//		}
//		else {
//			filenameNotGiven = FALSE;
//			readFile(*argv);
//		}
//	}
//	if (!filenameNotGiven) {
//		while (currPC >= 0 && currPC < MAX_NUM_ELEMENTS) {
//			parseInstr(currPC);
//		}
//	}
//	else {
//		die("No object file specified");
//	}


// enable for manual testing

	readFile("all.obj");

	while (currPC < MAX_NUM_ELEMENTS && currPC >= 0) {
		parseInstr(currPC);
	}

	exit(0);
}
