/*

00000: A : 0
00001: B : 1
00010: C : 2
00011: D : 3
00100: E : 4
00101: F : 5
00110: G : 6
00111: H : 7
01000: I : 8
01001: J : 9
01010: K : 10
01011: L : 11
01100: M : 12
01101: N : 13
01110: O : 14
01111: P : 15
10000: Q : 16
10001: R : 17
10010: S : 18
10011: T : 19
10100: U : 20
10101: V : 21
10110: W : 22
10111: X : 23
11000: Y : 24
11001: Z : 25
11010: 0 : 26
11011: 1 : 27
11100: 2 : 28
11101: 3 : 29
11110: 4 : 30
11111: 5 : 31

1) Break bit stream into 5 bit chunks
   If last chunk is not 5 bits, pad with 0s at the end
2) Ignore any character that isn't A-Z or 0-5

http://stackoverflow.com/questions/18109458/read-from-a-text-file-and-parse-lines-into-words-in-c
http://cboard.cprogramming.com/c-programming/59164-problem-malloc-sorting-words-text-file.html
http://www.dreamincode.net/forums/topic/29898-reading-a-text-file-into-an-array-dynamically-c/
http://www.daniweb.com/software-development/c/threads/66574/read-file-with-unknown-buffer-length !!!!!

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void test() {

	 char  arr[8];
	 int cnt,i,temp=0;
	 FILE * input;

	if((input=fopen("FourScore.txt","r"))==NULL)
	{
	        fprintf(stderr,"cannot open file\n");
	        exit(1);
	}
	 //Read the 8 bytes in a character array of size 8
	 cnt = fread(arr,1,8,input);
	 for (i = 0; i < cnt; i++)
	 {
	        //Now change it to 0/1 form by substracting 48
	        arr[i] = arr[i] - '0';/* Ascii of 0 = 48*/
	        //Also Left shift..
	        arr[i] = arr[i] << (cnt - (i+1));
	        //Now Bit wise OR with the integer...
	        temp = temp | arr[i];
	 }
	  printf("The ascii value is %d and the character is %c\n", temp, temp);
	  //return 0;

}

//void printstringasbinary(char* s)
//{
//    // A small 9 characters buffer we use to perform the conversion
//    char output[9];
//
//    // Until the first character pointed by s is not a null character
//    // that indicates end of string...
//    while (*s)
//    {
//        // Convert the first character of the string to binary using itoa.
//        // Characters in c are just 8 bit integers, at least, in noawdays computers.
//        itoa(*s, output, 2);
//
//        // print out our string and let's write a new line.
//        puts(output);
//
//        // we advance our string by one character,
//        // If our original string was "ABC" now we are pointing at "BC".
//        ++s;
//    }
//}

void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

void ctoa() {						// actually itoa, but whatever
	char c = 'F';
	printf("%d", (c >> 7) & 1);
	printf("%d", (c >> 6) & 1);
	printf("%d", (c >> 5) & 1);
	printf("%d", (c >> 4) & 1);
	printf("%d", (c >> 3) & 1);
	printf("%d", (c >> 2) & 1);
	printf("%d", (c >> 1) & 1);
	printf("%d\n", (c >> 0) & 1);
}

int getEvenParity(int v) {
	// for 1-byte words
	v ^= v >> 8;
	v ^= v >> 4;
	v ^= v >> 2;
    v ^= v >> 1;
	return v & 1;
}

int getOddParity(int v) {
	// for 1-byte words
	v |= v >> 8;
	v |= v >> 4;
	v |= v >> 2;
    v |= v >> 1;
	return v & 1;
}

int main(int argc, char *argv[]) {

	FILE *input;
	int i, size;
	char *array;

	input = fopen("FourScore.txt", "rb");				// NEED TO ALLOW FOR COMMAND LINE INPUTS LATER
	if (input == NULL) {
		perror("Invalid file specified.");
		exit(1);
	}

	fseek(input, 0, SEEK_END);
	size = ftell(input);
	fseek(input, 0, SEEK_SET);
		//printf("Size in bytes: %d\n", size);
		//printf("Size of allocated array: %u bytes\n", (sizeof(int) * size));

	array = (char*) malloc(((size + 1) * sizeof(char)));		// FREE BEFORE PROGRAM CLOSE (free(array))
	if (array == NULL) {
		perror("Could not allocate array.");
		exit(1);
	}
	else {
		//input = fopen("FourScore.txt", "rb");					// NEED TO ALLOW FOR COMMAND LINE INPUTS LATER
		fread(array, sizeof(char), size, input);
		// some check on return value of fread?
		// Upon successful completion, fread() shall return the number of elements
		// 	successfully read which is less than nitems only if a read error or end-of-file
		//	is encountered. If size or nitems is 0, fread() shall return 0 and the contents
		//	of the array and the state of the stream remain unchanged. Otherwise, if a read
		//	error occurs, the error indicator for the stream shall be set, [CX] [Option Start]
		//	and errno shall be set to indicate the error.
		array[size] = 10;										// for EOF
		fclose(input);
	}

	for (i = 0; i < (size + 1); i++) {
		//printf("array[%d] == %c\n", i, ((char *)array)[i]);
		//printf("array[%d] == %d\n", i, ((char *)array)[i]);
		printf("array[%d] == %d\n", i, array[i]);
	}

	int *bits = (int*) malloc(((size + 1) * 8));				// FREE BEFORE PROGRAM CLOSE (free(bits))
	int counter = 0;
	for (i = 0; i < (size + 1); i++) {
		int temp = array[i];
		bits[counter] = (temp >> 7) & 1;
		counter++;
		bits[counter] = (temp >> 6) & 1;
		counter++;
		bits[counter] = (temp >> 5) & 1;
		counter++;
		bits[counter] = (temp >> 4) & 1;
		counter++;
		bits[counter] = (temp >> 3) & 1;
		counter++;
		bits[counter] = (temp >> 2) & 1;
		counter++;
		bits[counter] = (temp >> 1) & 1;
		counter++;
		bits[counter] = (temp >> 0) & 1;
		counter++;
	}

	printf("\nBit stream: ");
	for (i = 0; i < ((size + 1) * 8); i++) {					// MAKE SIZE * 8 A VARIABLE LATER
		printf("%d", bits[i]);
	}
	printf("\n  Expected: 0100011001101111011101010111001000100000011100110110001101101111011100100110010100001010\n\n");
	printf("FWEJIOVWEJVBWE %d\n", bits[22]);

	int newSize = size;
	while (newSize % 5 != 0) {
		newSize++;
	}

	// if newSize > size, need to pad the end with 0s; MUST IMPLEMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	int *encoded = (int*) malloc(newSize / 5);				// FREE LATER

	counter = 4;
	int encodedCounter = 0;
	int mask = 0;
	int curBit;
	for (i = 0; i < ((size + 1) * 8); i++) {
		printf("Current index value: %d\n", i);
		curBit = bits[i];
		printf("Current bit: %d\n", curBit);
		printf("Mask before shift: %d\n", mask);
		mask = mask | (curBit << counter);
		printf("Current mask: %d\n", mask);
		counter--;
		if (counter < 0) {
			printf("Completed mask: %d\n\n", mask);
			encoded[encodedCounter] = mask;				// assumes that nothing over 8 bits will be sent in
			mask = 0;
			counter = 4;
			encodedCounter++;
		}
	}

	printf("\n\nContents of encoded array:");
	for (i = 0; i < (newSize / 5); i++) {
		printf("%d", encoded[i]);
	}

	printf("OFEJIWV %d\n", bits[22]);

	// int newSize = size;
	// current size is newSize = 88
	// while (newSize % 5 != 0) { increment newSize}
	// allocate new char array of size (newSize / 5)
	//


    //printf("Even parity: %d\n", getEvenParity(0x9));
    //printf("Odd parity: %d\n", getOddParity(0x9));

	/*

	NEED TO ACCOUNT FOR EOF CHARACTER FOR STRINGS SOMEWHERE!

	01000110011011110111010101110010001000000111001101100011011011110111001001100101					   <= actual, no enter after last character
	01000110011011110111010101110010001000000111001101100011011011110111001001100101 00001101 00001010     <= actual, enter after last character
	01000110011011110111010101110010001000000111001101100011011011110111001001100101 00001010              <= expected

	 */


/*  USING INT ARRAY

 	FILE *input;
	int i, size;
	int *array;

	input = fopen("FourScore.txt", "rb");		// NEED TO ALLOW FOR COMMAND LINE INPUTS LATER
	if (input == NULL) {
		perror("Invalid file specified.");
		exit(1);
	}

	fseek(input, 0, SEEK_END);
	size = ftell(input);
	fseek(input, 0, SEEK_SET);
		printf("Size in bytes: %d\n", size);
		//printf("Size of allocated array: %u bytes\n", (sizeof(int) * size));

	array = (int*) malloc(size * sizeof(int));
	if (array == NULL) {
		perror("Could not allocate array.");
		exit(1);
	}
	else {
		input = fopen("FourScore.txt", "rb");	// NEED TO ALLOW FOR COMMAND LINE INPUTS LATER
		fread(array, sizeof(int), size, input);
		// some check on return value of fread?
		fclose(input);
	}

	for (i = 0; i < size; i++) {
		//printf("array[%d] == %c\n", i, ((char *)array)[i]);
		printf("array[%d] == %d\n", i, ((char *)array)[i]);
		//printf("array[%d] == %d\n", i, array[i]);
	}

 */

	// http://www2.lawrence.edu/fast/GREGGJ/CMSC110/dynamic/dynamic_arrays.html


	return 1;	// FIX LATER
	// 01000 11001 10111 10111 01010 11100 10001 00000 01110 01101 10001 10110 11110 11100 10011 00101 00001 010
	//     I	 Z	   X     X     K     2     R     A     O     N     R     W     4     2     T     F     B   I (pad)
	//     8    25    23    23    10    28    17     0    14    13    17    22    30    28    19     5     1   8
	// 01000110 01101111 01110101 01110010 00100000 01110011 01100011 01101111 01110010 01100101 00001010
	//        F		   o		u		 r		 sp		   S		c		 o		  r		   e	   \n

}
