#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

FILE *input;

typedef short Boolean;
#define TRUE 1
#define FALSE 0

Boolean debug = FALSE;

typedef char *STRING;

#define CAST(t,e) ((t)(e))
#define TYPED_MALLOC(t) CAST(t*, malloc(sizeof(t)))
#define LINE_WIDTH 72

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

void scanargs(STRING s)
{
    /* check each character of the option list for
       its meaning. */

    while (*++s != '\0')
        switch (*s)
            {

            case 'd': /* debug option */
                debug = TRUE;
                break;

	     case 'D': // debug option 2
		  debug = TRUE;
		  break;

            default:
                fprintf (stderr,"PROGR: Bad option %c\n", *s);
                fprintf (stderr,"usage: PROGR [-D] file\n");
                exit(1);
            }
}

void encode() {

	int i, size;
	char *array;

	fseek(input, 0, SEEK_END);
	size = ftell(input);
	fseek(input, 0, SEEK_SET);

	if (size == 0) {
		exit(0);
	}

	//array = (char*) malloc(((size + 1) * sizeof(char)));		// FREE BEFORE PROGRAM CLOSE (free(array))
	array = (char*) calloc(size, sizeof(char));
	char *arrayPtr = array;
	if (array == NULL) {
		perror("Could not allocate array");
		exit(1);
	}
	else {

		fread(arrayPtr, sizeof(char), size, input);
		// some check on return value of fread?
		//arrayPtr[size] = 10;										// for EOF
		fclose(input);
	}

	//int *bits = (int*) malloc(((size + 1) * 8));				// FREE BEFORE PROGRAM CLOSE (free(bits))
	int *bits = (int*) calloc((size + 1) * 8, sizeof(int));
	int *bitsPtr = bits;
	int counter = 0;
	for (i = 0; i < (size); i++) {
		int temp = arrayPtr[i];
		bitsPtr[counter] = (temp >> 7) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 6) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 5) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 4) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 3) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 2) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 1) & 1;
		counter++;
		bitsPtr[counter] = (temp >> 0) & 1;
		counter++;
	}

	free(array);

	int newSize = (size) * 8;
	while (newSize % 5 != 0) {
		newSize++;
	}

	int *padded = (int*) calloc(newSize, sizeof(int));
	int *paddedPtr = padded;
	for (i = 0; i < ((size) * 8); i++) {
		paddedPtr[i] = bits[i];
	}
	for (; i < newSize; i++) {
		paddedPtr[i] = 0;
	}

	free(bits);

	//printf("       Padded bs: ");
	for (i = 0; i < newSize; i++) {
		if ((i - 4) % 5 == 0 && i != 0) {
			if (i % LINE_WIDTH == 0) {
				//printf("\n");
			}
			//printf("%d", paddedPtr[i]);
		}
		else {
			if (i % LINE_WIDTH == 0) {
				//printf("\n");
			}
			//printf("%d", paddedPtr[i]);
		}
	}

	//int *encoded = (int*) malloc(newSize / 5);				// FREE LATER
	int *encoded = (int*) calloc((newSize / 5), sizeof(int));
	int *encodedPtr = encoded;

	counter = 4;
	int encodedCounter = 0;
	int mask = 0;
	int curBit;
	for (i = 0; i < newSize; i++) {
		curBit = paddedPtr[i];
		mask |= (curBit << counter);
		counter--;
		if (counter < 0) {
			encodedPtr[encodedCounter] = mask;				// assumes that nothing over 8 bits will be sent in
			mask = 0;
			counter = 4;
			encodedCounter++;
		}
	}

	free(padded);

	for (i = 0; i < (newSize / 5); i++) {
		int temp = encodedPtr[i];
		if ((i % LINE_WIDTH) == 0 && i != 0) {
			printf("\n");
		}
		if (temp >= 0 && temp <= 25) {
			encodedPtr[i] += 65;
			printf("%c", encodedPtr[i]);
		}
		else {
			encodedPtr[i] += 22;
			printf("%c", encodedPtr[i]);
		}
	}

	printf("\n");

	free(encoded);

}

void decode() {

	int byte;
	int asciiNewLine = 10;

	int encodedCounter;
	int decoded;

	int eight[8];
	int bit;

	int i;
	int counter = 0;

	while((byte = getc(input)) != EOF) {

		if(byte != asciiNewLine) {
			if (byte >= 65) {
				byte -= 65;
			}
			else {
				byte -= 22;
			}

			encodedCounter = 0;

			while(encodedCounter < 5) {
				bit = (byte >> (4 - encodedCounter) & 1);

				eight[counter] = bit;
				counter++;
 				encodedCounter++;

 				if(counter == 8) {
 					decoded = 0;

 					for(i = 0; i < 8; i++) {
 						decoded |= eight[i] << (7 - i);
					}

 					printf("%c", decoded);
 					counter = 0;
 				}
			}
		}
	}
}

int main(int argc, char **argv) {

    Boolean filenamenotgiven = TRUE;

    while (argc > 1)
        {
            argc--, argv++;
	     //printf("%c %c\n", argc, **argv);
            if (**argv == '-') {
                scanargs(*argv);
            }
            else
                {
                    filenamenotgiven = FALSE;
                    input = fopen(*argv, "rb");
                    if (input == NULL)
                        {
                            fprintf (stderr, "Can't open %s\n",*argv);
                        }
			if (debug) {
				input = fopen(*argv, "r");
				decode();
				fclose(input);
			}
			else {
				encode();
			}
                }
        }

    if (filenamenotgiven)
        {
            input = stdin;
            if (debug) {
		  decode();
	     }
	     else {
	 	  encode();
  	     }
        }

    exit(0);
}
