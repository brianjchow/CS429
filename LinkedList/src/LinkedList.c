/*
 ============================================================================
 Name        : LinkedList.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

// a simpler linked list for Lab 3
// each node holds three strings and a pointer to the next node
// singly-linked, non-circular

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
struct Node *newNode(void) {
	return (struct Node *) malloc(sizeof(struct Node));
}

// make a new LinkedList
struct LinkedList *newLinkedList(void) {
	return (struct LinkedList *) malloc(sizeof(struct LinkedList));
}

// no error checking implemented here
void add(char *name, char *property, char *value, struct LinkedList *list) {

	// should not occur, but just in case
	if (list == NULL) {
		list = newLinkedList();
		if (list == NULL) {
			exit(-1);
		}
		else {
			(*list).head = NULL;
			(*list).tail = NULL;
			(*list).size = 0;
		}
	}

	struct Node *toAdd = newNode();
	if (toAdd == NULL) {
		// out of memory
		exit(-1);
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
		(*list).size += 1;
	}
}

// returns a copy of the Node at index index in LinkedList list
// to be primarily used for gaining access to all three data fields at once
// the returned copy's next field is set to NULL
Node *getNode(int index, struct LinkedList *list) {

	if (list == NULL) {
		return NULL;
	}
	if (index < 0 || index >= (*list).size) {
		return NULL;
	}
	else {
		int counter = 0;
		Node *temp = (*list).head;
		while (counter < index) {
			temp = temp->next;
			counter++;
		}
		Node *out = newNode();
		if (out == NULL) {
			// out of memory
			exit(-1);
		}
		else {
			(*out).name = (*temp).name;
			(*out).property = (*temp).property;
			(*out).value = (*temp).value;
			(*out).next = NULL;
			return out;
		}
	}
}

void get(char *name, char *property, struct LinkedList* list) {

	int elementsMatched = 0;
	struct Node* temp = (*list).head;
	while (temp != NULL) {
		if ((strcmp((*temp).name, name) == 0) && (strcmp((*temp).property, property) == 0)) {
			printf("F %s: %s = %s\n", name, property, (*temp).value);
			elementsMatched++;
		}
		temp = (*temp).next;
	}
	if (elementsMatched == 0) {
		printf("No entry with name '%s' and property '%s' was found.\n", name, property);
	}
}

int main(void) {

	struct LinkedList *list = newLinkedList();
	if (list == NULL) {
		// out of memory
		exit(-1);
	}
	else {
		// initialize variables
		(*list).head = NULL;
		(*list).tail = NULL;
		(*list).size = 0;
	}

	char *name = "Corporations";
	char *property = "Apple";
	char *value = "iPod";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	name = "Corporations";
	property = "Apple";
	value = "Mac";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	name = "Fruits";
	property = "Banana";
	value = "Yellow";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	name = "Fruits";
	property = "Granny Smith Apple";
	value = "Green";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	name = "Corporations";
	property = "Microsoft";
	value = "Windows";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	name = "Fruits";
	property = "Tomato";
	value = "Red";
	add(name, property, value, list);
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Added name = %s, property = %s, value = %s\n", name, property, value);
	printf("Current size of list: %d\n\n", (*list).size);

	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("Testing that next was set correctly (format is [name, property, value]):\n");
	int counter = 0;
	struct Node *temp = (*list).head;
	while (temp != NULL) {
		printf("[%s, %s, %s]", temp->name, temp->property, temp->value);
		if (counter < (*list).size - 1) {
			printf("---");
		}
		temp = temp->next;
		counter++;
	}

	printf("\n\n------------------------------------------------------------------------------------------------------------------\n");
	printf("Fetching all property/value combos for name = 'Corporation':\n");
	counter = 0;
	char *delimit = "Corporations";
	temp = (*list).head;
	while (temp != NULL) {
		if (strcmp(delimit, temp->name) == 0) {
			printf("[%s, %s, %s]", temp->name, temp->property, temp->value);
			if (counter < (*list).size - 1) {
				printf("---");
			}
		}
		temp = temp->next;
		counter++;
	}

	printf("\n\n------------------------------------------------------------------------------------------------------------------\n");
	printf("Fetching all property/value combos for name = 'Fruits':\n");
	counter = 0;
	delimit = "Fruits";
	temp = (*list).head;
	while (temp != NULL) {
		if (strcmp(delimit, temp->name) == 0) {
			printf("[%s, %s, %s]", temp->name, temp->property, temp->value);
			if (counter < (*list).size - 1) {
				printf("---");
			}
		}
		temp = temp->next;
		counter++;
	}

	printf("\n\n------------------------------------------------------------------------------------------------------------------\n");
	printf("Fetching all property/value combos for name = 'Corporation' and property = 'Apple':\n");
	counter = 0;
	char *nameDelimit = "Corporations";
	char *propDelimit = "Apple";
	temp = (*list).head;
	while (temp != NULL) {
		if (strcmp(nameDelimit, (*temp).name) == 0 && strcmp(propDelimit, (*temp).property) == 0) {
			printf("[%s, %s, %s]", (*temp).name, (*temp).property, (*temp).value);
			if (counter < (*list).size - 1) {
				printf("---");
			}
		}
		temp = temp->next;
		counter++;
	}

	Node* currNode = (*list).head;
	while (currNode != NULL) {
		temp = currNode;
		currNode = (*currNode).next;
		free(temp);
	}

	free(list);

	return EXIT_SUCCESS;
}