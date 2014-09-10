/*
 ============================================================================
 Name        : facts.c
 Author      : Brian Chow (bc23784)
 Date		 : 10/11/13
 Description : See tester.c for general procedure and Lab3.htm in src folder
 ============================================================================
*/

/*
 	 Possible bugfixes
 	 	 - in store and retrieveFact: strncpy doesn't add in the \0 character to terminate the string; modify?
 	 	 	 the name/prop/value arrays have one more space, but I didn't fill them in with \0; need to check their contents first
 	 	 - scanFile() doesn't have any precondition checks
 	 	 - scanFile(): if EOF character is encountered before actual end of text, it will terminate; can be fixed by using SEEK_SIZE and ftell
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef short boolean;
#define TRUE 1
#define FALSE 0

boolean useExitZero = FALSE;				// because apparently all exit values should be 0 for this assignment
boolean useFormatting = FALSE;				// just adds spaces to the output results if true
boolean debug = FALSE;
boolean debugRetrieveAndStore = FALSE;

double reallocWhenLoadFactorEquals = 0.7;
size_t buflen = 20000;
int buflenInt = 20000;

// *****************************************************************************************************************************************************************************

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

//	void scanArgs(STRING s) {
//		/* check each character of the option list for
//		   its meaning. */
//
//		while (*++s != '\0') {
//			switch (*s) {
//
//				// no cases available for this program at this time
//
//				case 'd': /* debug option */
//					debug = TRUE;
//					break;
//
//				case 'D': // debug option 2
//					debug = TRUE;
//					break;
//
//				default:
//					fprintf (stderr,"PROGR: Bad option %c\n", *s);
//					exit(-1);
//			}
//		}
//	}

// end procedures for handling command line input
// ----------------------------------------------

// *****************************************************************************************************************************************************************************

// -------------------------------------------------
// begin miscellaneous procedures for main procedure

	void die(char *reason) {
		fprintf(stderr, "*** %s ***\n\n", reason);
		if (useExitZero) { exit(0); }
		exit(-1);
	}

// end miscellaneous procedures for main procedure
// -----------------------------------------------

// *****************************************************************************************************************************************************************************

// --------------------------
// begin embedded linked list

	typedef struct Node Node;
	typedef struct LinkedList LinkedList;

	struct Node {
		char *name;
		char *property;
		char *value;
		struct Node *next;
	};

	struct LinkedList {
		struct Node *head;
		struct Node *tail;
		int size;
	};

	// make a new Node
	Node *newNode(void) {
		return (Node *) malloc(sizeof(struct Node));
	}

	// make a new LinkedList
	LinkedList *newLinkedList(void) {
		LinkedList* out = (LinkedList *) malloc(sizeof(struct LinkedList));
		if (out == NULL) {
			die("Out of memory");
		}
		(*out).head = NULL;
		(*out).tail = NULL;
		(*out).size = 0;
		return out;
	}

	void freeLinkedList(LinkedList* list) {
		Node* temp = (*list).head;
		Node* currNode = (*list).head;
		while (currNode != NULL) {
			temp = currNode;
			currNode = (*currNode).next;
			free((*temp).name);
			free((*temp).property);
			free((*temp).value);
			free(temp);
		}
		free(list);
	}

	void toString(LinkedList* list) {
		Node* temp = (*list).head;
		int counter = 0;
		while (temp != NULL) {
			fprintf(stdout, "[%s, %s, %s]", (*temp).name, (*temp).property, (*temp).value);
			counter++;
			if (temp != (*list).tail) {
				fprintf(stdout, "---");
				if (counter % 5 == 0) {
					fprintf(stdout, "\n");
				}
			}
			temp = (*temp).next;
		}
	}

	int get(char *name, char *property, LinkedList* list) {

		if (name == NULL || property == NULL || list == NULL) {
			return FALSE;
		}

		int elementFound = 0;
		struct Node* temp = (*list).head;
		while (temp != NULL && elementFound == 0) {
			if ((strcmp((*temp).name, name) == 0) && (strcmp((*temp).property, property) == 0)) {
				if (useFormatting) {
					fprintf(stdout, "F %s: %s = %s\n", name, property, (*temp).value);
				}
				else {
					fprintf(stdout, "F %s: %s=%s\n", name, property, (*temp).value);
				}
				elementFound = 1;
			}
			temp = (*temp).next;
		}
		if (elementFound == 0) {
			//fprintf(stdout, "Error: no entry with name '%s' and property '%s' was found.\n", name, property);
			if (useFormatting) {
				fprintf(stdout, "F %s: %s = unknown\n", name, property);
			}
			else {
				fprintf(stdout, "F %s: %s=unknown\n", name, property);
			}
			return FALSE;
		}
		return TRUE;
	}

	int add(char *name, char *property, char *value, LinkedList *list) {

		if (name == NULL || property == NULL || value == NULL || list == NULL) {
			return FALSE;
		}

		// check to see if name and property already exist
		int elementFound = 0;
		Node* temp = (*list).head;
		while (temp != NULL && elementFound == 0) {
			if ((strcmp((*temp).name, name) == 0) && (strcmp((*temp).property, property) == 0)) {
				free((*temp).value);
				free(name);
				free(property);
				(*temp).value = value;
				elementFound = 1;
			}
			temp = (*temp).next;
		}
		if (elementFound == 1) {
			return TRUE;
		}

		Node *toAdd = newNode();
		if (toAdd == NULL) {
			// out of memory
			free(name);
			free(property);
			free(value);
			freeLinkedList(list);
			die("Out of memory");
		}
		else {
			// initialize variables
			(*toAdd).name = name;
			(*toAdd).property = property;
			(*toAdd).value = value;
			(*toAdd).next = NULL;
		}

		// append to end of list
		if ((*list).size == 0) {
			// list is empty
			(*list).head = toAdd;
			(*list).tail = toAdd;
			(*list).size++;
		}
		else {
			(list->tail)->next = toAdd;
			(*list).tail = toAdd;
			(*list).size++;
		}

		return TRUE;
	}

// end embedded linked list
// ------------------------

// *****************************************************************************************************************************************************************************

// ---------------------------------
// begin procedure for storing facts

	int storeFact(char *string, LinkedList* list) {

		int length = strlen(string);
		if (string == NULL || length <= 0) {
			//fprintf(stdout, "No string specified. Moving on to next line...\n");
			return FALSE;
		}

		int i = 0;
		int indexOfFirstColon = -1;
		int indexOfFirstEquals = -1;
		int indexOfFirstChar = -1;
		int indexOfStartName = -1;
		int indexOfEndName = -1;
		int indexOfStartProp = -1;
		int indexOfEndProp = -1;
		int indexOfStartVal = -1;
		int indexOfEndVal = -1;

		while (i < length && isspace((int) string[i])) {
			i++;
		}
		indexOfFirstChar = i;
		if (string[i] != 'F' && string[i] != 'f') {
			//fprintf(stdout, "'%s' is not a valid string. Moving on to next line...\n", string);
			return FALSE;
		}
		i++;

		// get index of first colon
		char *ptrToFirstColon = strchr(string, ':');
		if (ptrToFirstColon == NULL) {
			//fprintf(stdout, "'%s' is not a valid string. Moving on to next line...\n", string);
			return FALSE;
		}
		indexOfFirstColon = ptrToFirstColon - string;

		// find index of first equals sign after first colon
		i = indexOfFirstColon + 1;
		while (i < length && indexOfFirstEquals == -1) {
			int temp = (int) string[i];
			if (temp == 61) {
				indexOfFirstEquals = i;
			}
			i++;
		}
		if (indexOfFirstEquals == -1) {
			//fprintf(stdout, "'%s' is not a valid string. Moving on to next line...\n", string);
			return FALSE;
		}

		// find start and stop indices of the name
		i = indexOfFirstChar + 1;
		while (i < indexOfFirstColon && isspace((int) string[i])) {
			i++;
		}
		indexOfStartName = i;
		i = indexOfFirstColon - 1;
		while (i > indexOfStartName && isspace((int) string[i])) {
			i--;
		}
		indexOfEndName = i;

		// find start and stop indices of the property
		i = indexOfFirstColon + 1;
		while (i < indexOfFirstEquals && isspace((int) string[i])) {
			i++;
		}
		indexOfStartProp = i;
		i = indexOfFirstEquals - 1;
		while (i > indexOfStartProp && isspace((int) string[i])) {
			i--;
		}
		indexOfEndProp = i;

		// find start and stop indices of the value
		i = indexOfFirstEquals + 1;
		while (i < length && isspace((int) string[i])) {
			i++;
		}
		indexOfStartVal = i;
		i = length - 1;
		while (i > indexOfStartVal && isspace((int) string[i])) {
			i--;
		}
		indexOfEndVal = i;

		// error check all indices?

		// edge case where string has just the three required delimiters and nothing else
		i = 0;
		int charCount = 0;
		while (i < length) {
			if (string[i] != 'F' && string[i] != 'f' && (int) string[i] != 58 && (int) string[i] != 61 && !isspace((int) string[i])) {
				charCount++;
			}
			i++;
		}
		if (charCount == 0) {
			//fprintf(stdout, "'%s' has nothing to store. Moving on to next line...\n", string);
			return FALSE;
		}

		char *name = calloc(indexOfFirstColon - indexOfFirstChar, sizeof(*string));
		char *property = calloc(indexOfFirstEquals - indexOfFirstColon, sizeof(*string));
		char *value = calloc(length - indexOfFirstEquals, sizeof(*string));
		if (name == NULL || property == NULL || value == NULL) {
			// out of memory
			free(name);
			free(property);
			free(value);
			freeLinkedList(list);
			die("Out of memory");
		}

		strncpy(name, string + indexOfStartName, indexOfEndName - indexOfStartName + 1);
		strncpy(property, string + indexOfStartProp, indexOfEndProp - indexOfStartProp + 1);
		strncpy(value, string + indexOfStartVal, indexOfEndVal - indexOfStartVal + 1);

		if (debugRetrieveAndStore) {
			fprintf(stdout, "In function 'store':\n\tInput string: %s\n\tLength of string: %d\n\tIndex of first char: %d\n\t"
					"Start and stop of name indices: %d, %d\n\tStart and stop of property indices: %d, %d\n\tStart and "
					"stop of value indices: %d, %d\n\tParsed name: \'%s\'\n\tParsed property: \'%s\'\n\tParsed value: "
					"\'%s\'\n\n", string, length, indexOfFirstChar, indexOfStartName, indexOfEndName, indexOfStartProp,
					indexOfEndProp, indexOfStartVal, indexOfEndVal, name, property, value);
		}

		return add(name, property, value, list);
	}

// end procedure for storing facts
// -------------------------------

// *****************************************************************************************************************************************************************************

// ------------------------------------
// begin procedure for retrieving facts

	int retrieveFact(char *string, LinkedList* list) {

		int length = strlen(string);
		if (string == NULL || length <= 0) {
			//fprintf(stdout, "No string specified. Moving on to next line...\n");
			return FALSE;
		}

		int i = 0;
		int indexOfFirstColon = -1;
		int indexOfFirstChar = -1;
		int indexOfStartName = -1;
		int indexOfEndName = -1;
		int indexOfStartProp = -1;
		int indexOfEndProp = -1;

		while (i < length && isspace((int) string[i])) {
			i++;
		}
		indexOfFirstChar = i;
		if (string[i] != 'Q' && string[i] != 'q') {
			//fprintf(stdout, "'%s' is not a valid string. Moving on to next line...\n", string);
			return FALSE;
		}

		// get index of first colon
		char *ptrToFirstColon = strchr(string, ':');
		if (ptrToFirstColon == NULL) {
			//fprintf(stdout, "'%s' is not a valid string. Moving on to next line...\n", string);
			return FALSE;
		}
		indexOfFirstColon = ptrToFirstColon - string;

		// get start and stop indices of the name
		i = indexOfFirstChar + 1;
		while (i < indexOfFirstColon && isspace((int) string[i])) {
			i++;
		}
		indexOfStartName = i;
		i = indexOfFirstColon - 1;
		while (i > indexOfStartName && isspace((int) string[i])) {
			i--;
		}
		indexOfEndName = i;

		// get start and stop indices of the property
		i = indexOfFirstColon + 1;
		while (i < length && isspace((int) string[i])) {
			i++;
		}
		indexOfStartProp = i;
		i = length - 1;
		while (i > indexOfStartProp && isspace((int) string[i])) {
			i--;
		}
		indexOfEndProp = i;

		// edge case where string has just the two required delimiters and nothing else
		i = 0;
		int charCount = 0;
		while (i < length) {
			if (string[i] != 'Q' && string[i] != 'q' && (int) string[i] != 58 && !isspace((int) string[i])) {
				charCount++;
			}
			i++;
		}
		if (charCount == 0) {
			//fprintf(stdout, "'%s' has nothing to retrieve. Moving on to next line...\n", string);
			return FALSE;
		}

		char *name = calloc(indexOfFirstColon - indexOfFirstChar, sizeof(*string));
		char *property = calloc(length - indexOfFirstColon, sizeof(*string));
		if (name == NULL || property == NULL) {
			// out of memory
			free(name);
			free(property);
			freeLinkedList(list);
			die("Out of memory");
		}

		strncpy(name, string + indexOfStartName, indexOfEndName - indexOfStartName + 1);
		strncpy(property, string + indexOfStartProp, indexOfEndProp - indexOfStartProp + 1);

		if (debugRetrieveAndStore) {
			fprintf(stdout, "In function 'retrieveFact':\n\tInput string: %s\n\tLength of string: %d\n\tIndex of first char: %d\n\t"
					"Start and stop of name indices: %d, %d\n\tStart and stop of property indices: %d, %d\n\t"
					"Parsed name: \'%s\'\n\tParsed property: \'%s\'\n\tResult of query: ", string, length, indexOfFirstChar, indexOfStartName,
					indexOfEndName,	indexOfStartProp, indexOfEndProp, name, property);
		}

		boolean getWasSuccessful = get(name, property, list);
		free(name);
		free(property);

		return getWasSuccessful;
	}

// end procedure for retrieving facts
// ----------------------------------

// *****************************************************************************************************************************************************************************

// -------------------------------------------------
// begin miscellaneous procedures for main procedure

	void scanFile(FILE *stream, boolean isFactFile, LinkedList* facts) {

		int byte;
		int counter = 0;
		char *buffer = calloc(buflen, sizeof(char));
		if (buffer == NULL) {
			die("Out of memory");
		}
		char *end = buffer + buflen - 1;
		char *bufPtr = buffer;
		//boolean hasBeenReallocd = FALSE;

		while ((byte = getc(stream)) != EOF && bufPtr < end) {
			if (byte == '\n') {
				// save '\0' to buffer
				// send buffer to store
				// reset buffer
				*bufPtr++ = '\0';

				if (debug) {
					fprintf(stdout, "\n\nNewline encountered.\nBuffer being sent for storing: \'%s\'", buffer);
				}

				if (isFactFile) {
					storeFact(buffer, facts);
				}
				else {
					retrieveFact(buffer, facts);
				}

				memset(buffer, 0, sizeof(*buffer));
				bufPtr = buffer;
				counter = 0;
			}

//			else if (counter >= (buflenDbl * reallocWhenLoadFactorEquals) && !hasBeenReallocd) {
//				// realloc buffer; double its previous size
//				hasBeenReallocd = TRUE;
//
//				if (debug) { printf("\n\n\nSize of buffer before reallocation: %lu\n\n\n", sizeof(buffer)); }
//
//				char *temp = realloc(buffer, 2 * sizeof(*buffer));
//				if (temp == NULL) {
//					die("Out of memory");
//				}
//				else {
//					memset(temp, 0, sizeof(*temp));
//					char *tempPtr = buffer;
//					int i;
//					for (i = 0; i < counter; i++) {
//						temp[i] = tempPtr[i];
//					}
//					tempPtr = NULL;
//					buffer = temp;
//					temp = NULL;
//				}
//
//				if (debug) { printf("\n\n\nSize of buffer before reallocation: %lu\n\n\n", sizeof(buffer)); }
//
////				buffer = realloc(buffer, 2 * sizeof(*buffer));
////				if (buffer == NULL) {
////					die("Out of memory");
////				}
//
//				end = buffer + buflen - 1;
//			}

			else {
				// store byte to buffer
				*bufPtr++ = byte;
				counter++;

				// buffer overflow imminent; skip the rest of this line in the file
				// can increase the buffer size to prevent if desired
				if (counter == buflenInt - 1 && byte != '\n') {
					fprintf(stderr, "Warning: abnormally long string detected. Discarding...\n");
					while ((byte = getc(stream)) != EOF) {
						// do nothing; keep advancing until newline is reached and start reading next line
						if (byte == '\n') {
							break;
						}

						if (debug) {
							fprintf(stdout, "Large string detected; now skipping character \'%c\' ", byte);
						}
					}
					memset(buffer, 0, sizeof(*buffer));
					bufPtr = buffer;
					counter = 0;
				}
			}
		}
		free(buffer);
	}

	void processFactsFile(char *filename, LinkedList* facts) {

		FILE *factsFile;

		factsFile = fopen(filename, "rb");
		if (factsFile == NULL) {
			fprintf(stderr, "Error: cannot open file \'%s\'!\n", filename);
			freeLinkedList(facts);
			if (useExitZero) { exit(0); }
			exit(-1);
		}

		scanFile(factsFile, TRUE, facts);

		if (debug) {
			fprintf(stdout, "\n\nCurrent linked list:\n");
			toString(facts);
			fprintf(stdout, "\n\n");
		}

		fclose(factsFile);
	}

	void processQuestionsFile(char *filename, LinkedList* facts) {

		FILE *qFile;

		if (filename == NULL) {
			qFile = stdin;
		}
		else {
			qFile = fopen(filename, "rb");
			if (qFile == NULL) {
				fprintf(stderr, "Error: cannot open file \'%s\'!\n", filename);
				freeLinkedList(facts);
				if (useExitZero) { exit(0); }
				exit(-1);
				//return;							// enable if program should continue reading files while they exist
			}
		}

		scanFile(qFile, FALSE, facts);

		fclose(qFile);
	}

// end miscellaneous procedures for main procedure
// -----------------------------------------------

// *****************************************************************************************************************************************************************************

	int main(int argc, char **argv) {

		// standard syntax: facts computers.fax computers.q
		// if arguments other than filenames are desired (e.g., '-d' for debug), enable the scanargs function and edit its parameters

		if (useFormatting) { printf("\n"); }

		if (debug) {
			// random tests here

			LinkedList* facts = newLinkedList();
			processFactsFile("random.fax", facts);
			processQuestionsFile("random.q", facts);
			freeLinkedList(facts);
		}

		else {
			// too few arguments to continue
			if (argc < 2) {
				die("Error: you must specify a facts file!");
			}

			LinkedList* facts = newLinkedList();

			// only the facts file was provided; take questions from the command line
			if (argc == 2) {
				argv++;
				processFactsFile(*argv, facts);
				processQuestionsFile(NULL, facts);
			}

			// the facts file was provided, as well as one or more questions files
			else {
				argc--, argv++;
				processFactsFile(*argv, facts);
				while (argc > 1) {
					argc--, argv++;
					processQuestionsFile(*argv, facts);
				}
			}

			if (useFormatting) { printf("\n"); }

			freeLinkedList(facts);
		}

		return 0;
	}
