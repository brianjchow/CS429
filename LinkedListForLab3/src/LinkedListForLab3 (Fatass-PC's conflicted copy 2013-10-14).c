/*
 ============================================================================
 Name        : LinkedListForLab3.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

// for development purposes

/*

 the top-level linked list (NameLinkedList) holds NameNodes, which hold three things:
 	 - a pointer to a name (the name of an object)
 	 - a pointer to the next NameNode
 	 - a pointer to a PropLinkedList, which holds the properties of the name [of an object] stored in this node

 the bottom-level linked list (PropLinkedList) holds PropNodes, which hold three things:
 	 - a pointer to one specific property of this Propnode's parent node (a NameNode)
 	 - a pointer to the next PropNode
 	 - a pointer to the value specified by the property in this PropNode

 traversing both LinkedLists is currently a one-way ordeal: it is only possible to move to the next NameNode
 	 and PropNode in their respective LinkedLists
 it is also only one-way from a NameNode: it is possible to go NameNode -> PropLinkedList of that NameNode -> PropNode -> property,
 	 but not back up

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct NameNode NameNode;
typedef struct NameLinkedList NameLinkedList;
typedef struct PropNode PropNode;
typedef struct PropLinkedList PropLinkedList;

struct NameNode {
	char *name;
	NameNode *next;
	PropLinkedList *properties;
};

struct NameLinkedList {
	NameNode *head, *tail;
	int size;
};

struct PropNode {
	char *property;
	PropNode *next;
	char *value;
};

struct PropLinkedList {
	PropNode *head, *tail;
	int size;
};

//NameLinkedList names;

NameNode* newNameNode(char* name) {
	// char *name, NameNode *next, PropLinkedList *properties

	NameNode* out = malloc(sizeof(struct NameNode));
	if (name != NULL) {
		(*out).name = name;
	}
	return out;
}

NameLinkedList* newNameLinkedList() {
	// NameNode *head, NameNode *tail, int size

	NameLinkedList* out = malloc(sizeof(struct NameLinkedList));
	(*out).size = 0;
	return out;
}

// what should happen if either value or property are null when sent in? should never happen though
PropNode* newPropNode(char *value, char* property) {
	// char *value, char *property, PropNode *next

	PropNode* out = malloc(sizeof(struct PropNode));
	(*out).value = value;
	(*out).property = property;
	return out;
}

PropLinkedList* newPropLinkedList() {
	// PropNode *head, PropNode *tail, int size

	PropLinkedList* out = malloc(sizeof(struct PropLinkedList));
	(*out).size = 0;
	return out;
}

// assumes that everything that can't be null in each struct is indeed not null
void addNNtoNLL(NameNode* toAdd, NameLinkedList* list) {

	if (toAdd == NULL || list == NULL) {
		return;
	}
	if ((*list).size == 0) {
		(*list).head = toAdd;
		(*list).tail = toAdd;
		(*list).size++;
	}
	else {

		(list->tail)->next = toAdd;
		(*list).tail = toAdd;
		(*list).size++;

//		NameNode* tempTail = (*list).tail;
//		(*tempTail).name = (*toAdd).name;
//		(*tempTail).next = toAdd;
//		(list->tail)->next = toAdd;
//		(*list).tail = toAdd;
//		(list->tail)->name = (*toAdd).name;
//		(*list).size++;
	}
}

void addPNtoPLL(PropNode* toAdd, PropLinkedList* list) {

	if (toAdd == NULL || list == NULL) {
		return;
	}
	if ((*list).size == 0) {
		(*list).head = toAdd;
		(*list).tail = toAdd;
		(*list).size++;
	}
	else {

		(list->tail)->next = toAdd;
		(*list).tail = toAdd;
		(*list).size++;

//		PropNode* tempTail = (*list).tail;
//		(*tempTail).value = (*toAdd).value;
//		(*tempTail).next = toAdd;
//		(list->tail)->next = toAdd;
//		(*list).tail = toAdd;
//		(list->tail)->value = (*toAdd).value;
//		(*list).size++;
	}
}






int main(void) {

	NameLinkedList* nll = newNameLinkedList();
	PropLinkedList* pll = newPropLinkedList();

	// BEGIN TESTING NAMENODE

	printf("TESTING NAMENODE -----------------------------------------------------\n");

	char *name = "Corporations";
	NameNode* corps = newNameNode(name);

	name = "Fruits";
	NameNode* fruits = newNameNode(name);

	printf("Name stored in corps NameNode: %s\n", (*corps).name);
	printf("Name stored in fruits NameNode: %s\n\n", (*fruits).name);

	// END TESTING NAMENODE

	// BEGIN TESTING PROPNODE

	printf("TESTING PROPNODE ------------------------------------------------------\n");

	char* value = "Apple";
	char* prop = "Mac";
	PropNode* pn1 = newPropNode(value, prop);

	value = "Microsoft";
	prop = "Windows";
	PropNode* pn2 = newPropNode(value, prop);

	printf("Value and property stored in pn1: %s, %s\n", (*pn1).value, (*pn1).property);
	printf("Value and property stored in pn2: %s, %s\n\n", (*pn2).value, (*pn2).property);

	// END TESTING PROPNODE

	// BEGIN TESTING NAMELINKEDLIST

	printf("TESTING addNNtoNLL ------------------------------------------------\n");

	printf("Adding corps to empty NLL:\n");
	addNNtoNLL(corps, nll);
	printf("Size of NLL after adding: %d\n\n", (*nll).size);

	printf("Adding fruits to NLL of current size 1:\n");
	addNNtoNLL(fruits, nll);
	printf("Size of NLL after adding: %d\n\n", (*nll).size);

	printf("Test that head was set correctly: %s (should be 'Corporations')\n", (nll->head)->name);
	printf("Test that tail was set correctly: %s (should be 'Fruits')\n", (nll->tail)->name);

	// END TESTING NAMELINKEDLIST

	// need to test adding a new property/value combo to an existing PLL (in method, check if property exists first)
	// need to test attaching a PLL to a NameNode


	return EXIT_SUCCESS;
}
