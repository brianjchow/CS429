/*
 ============================================================================
 Name        : LinkedListForLab3.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description :
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
	struct NameNode *next;
	struct PropLinkedList *properties;
};

struct NameLinkedList {
	struct NameNode *head, *tail;
	int size;
};

struct PropNode {
	char *property;
	struct PropNode *next;
	char *value;
};

struct PropLinkedList {
	struct PropNode *head, *tail;
	int size;
};

//NameLinkedList names;

NameNode* newNameNode(char* name) {
	// char *name, NameNode *next, PropLinkedList *properties

	NameNode* out = calloc(3, sizeof(struct NameNode));
	if (name != NULL) {
		(*out).name = name;
	}
	return out;
}

NameLinkedList* newNameLinkedList() {
	// NameNode *head, NameNode *tail, int size

	NameLinkedList* out = calloc(3, sizeof(struct NameLinkedList));
	(*out).size = 0;
	return out;
}

// what should happen if either value or property are null when sent in? should never happen though
PropNode* newPropNode(char *property, char* value) {
	// char *value, char *property, PropNode *next

	PropNode* out = calloc(3, sizeof(struct PropNode));
	(*out).value = value;
	(*out).property = property;
	return out;
}

PropLinkedList* newPropLinkedList() {
	// PropNode *head, PropNode *tail, int size

	PropLinkedList* out = calloc(3, sizeof(struct PropLinkedList));
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
		(list->tail)->next = NULL;
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
		(list->tail)->next = NULL;
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

// no precondition checks; assumes all parameters are valid
void add(char *name, char *property, char *value, NameLinkedList* list) {
	// scan list for *name
		// if found
			// pull that NameNode's PLL
				// if null (which shouldn't happen)
					// create new PLL
					// create new PN, attach to the new PLL
					// attach the new PLL to the parent NameNode
				// else
					// scan the PLL for *property
						// if found
							// set the PN's value field = *value
						// else
							// create new PN, attach to the PLL
		// else (not found)
			// create new NameNode
			// create new PLL
			// create new PN using *property and *value, attach to the new PLL
			// attach the new PLL to the new NameNode
			// attach the new NameNode to the end of list


	if ((*list).size == 0) {
		NameNode* newName = newNameNode(name);
		PropNode* newPN = newPropNode(property, value);
		PropLinkedList* newPLL = newPropLinkedList();
		addPNtoPLL(newPN, newPLL);
		(*newName).properties = newPLL;
		addNNtoNLL(newName, list);
	}
	else {
		NameNode* temp = (*list).head;
//		if ((*list).size == 1) {
//			if (strcmp((*temp).name, name) == 0) {
//				PropLinkedList* pllFound = (*temp).properties;
//				if (pllFound == NULL) {	// shouldn't happen within the constraints of this program
//					PropLinkedList* newPLL = newPropLinkedList();
//					PropNode* newPN = newPropNode(property, value);
//					addPNtoPLL(newPN, newPLL);
//					(*temp).properties = newPLL;
//				}
//				else if ((*pllFound).size == 0) {
//					PropNode* newPN = newPropNode(property, value);
//					addPNtoPLL(newPN, pllFound);
//				}
//				else {
//					int propExists = 0;
//					PropNode* tempPLLHead = (*pllFound).head;
//					PropNode* pnFound;
//					while (tempPLLHead != NULL && propExists == 0) {
//						if (strcmp((*tempPLLHead).property, property) == 0) {
//							propExists = 1;
//							pnFound = tempPLLHead;
//						}
//						tempPLLHead = (*tempPLLHead).next;
//					}
//					if (propExists == 1) {
//						(*pnFound).value = value;
//					}
//					else {
//						PropNode* toAdd = newPropNode(value, property);
//						addPNtoPLL(toAdd, (*temp).properties);
//					}
//				}
//			}
//			else {
//				NameNode* newNN = newNameNode(name);
//				PropLinkedList* newPLL = newPropLinkedList();
//				PropNode* newPN = newPropNode(property, value);
//				addPNtoPLL(newPN, newPLL);
//				(*newNN).properties = newPLL;
//				addNNtoNLL(newNN, list);
//			}
//		}
//		else {
			// scan for *name
			int nameExists = 0;
			NameNode* nnFound;
			PropLinkedList* pllFound;
			while (temp != NULL && nameExists == 0) {
				if (strcmp((*temp).name, name) == 0) {
					nameExists = 1;
					nnFound = temp;
				}
				temp = (*temp).next;
			}
			if (nameExists == 1) {
				pllFound = (*nnFound).properties;
				if (pllFound == NULL) {		// shouldn't happen within the constraints of this program
					// create new PLL; create new PN, attach to the new PLL; attach the new PLL to the parent NameNode
					PropLinkedList* newPLL = newPropLinkedList();
					PropNode* newPN = newPropNode(property, value);
					addPNtoPLL(newPN, newPLL);
					(*nnFound).properties = newPLL;
				}
				else {
					// scan the PLL for *property
					PropNode* tempPLLHead = (*pllFound).head;
					//if ((*pllFound).size == 1) {

					//}
					//else {
					int propExists = 0;
					PropNode* pnFound;
					while (tempPLLHead != NULL && propExists == 0) {
						if (strcmp((*tempPLLHead).property, property) == 0) {
							propExists = 1;
							pnFound = tempPLLHead;
						}
						tempPLLHead = (*tempPLLHead).next;
					}
					if (propExists == 1) {
						// set the PN's value field = *value
						(*pnFound).value = value;
					}
					else {
						// property not found; create new PN, attach to the PLL
						PropNode* toAdd = newPropNode(value, property);
						addPNtoPLL(toAdd, (*nnFound).properties);
					}
					//}
				}
			}
			else {
				// name not found
				NameNode* newNN = newNameNode(name);
				PropLinkedList* newPLL = newPropLinkedList();
				PropNode* newPN = newPropNode(property, value);
				addPNtoPLL(newPN, newPLL);
				(*newNN).properties = newPLL;
				addNNtoNLL(newNN, list);
			}
//		}
	}
}

NameNode* getNN(int index, NameLinkedList* list) {
	int i;
	NameNode* out = (*list).head;
	if (index >= (*list).size || index < 0) {
		return NULL;
	}
	else {
		for (i = 0; i < index; i++) {
			out = (*out).next;
		}
		return out;
	}

}

void testOverallAdd(char *name, char *property, char *value, NameLinkedList* list) {
	printf("Adding name = %s, property = %s, value = %s\n", name, property, value);
	printf("Previous size: %d; ", (*list).size);
	add(name, property, value, list);
	printf("New size: %d\n", (*list).size);
	int i;
//
//	printf("Contents of entire NLL: ");
//
//	for (i = 0; i < (*list).size; i++) {
//		NameNode* nn = getNN(i, list);
//		printf("[%s: ", (*nn).name);
//		PropLinkedList* pll = (*nn).properties;
//		PropNode* pn = (*pll).head;
//		while (pn != NULL) {
//			if (i == (*list).size - 1) {
//				printf("(%s, %s)]\n", (*pn).property, (*pn).value);
//			}
//			else {
//				printf("(%s, %s), ", (*pn).property, (*pn).value);
//			}
//			pn = (*pn).next;
//		}
//	}

	NameNode* firstName = getNN(0, list);
	printf("Name of first node: %s\n", (*firstName).name);
	PropLinkedList* firstProps = (*firstName).properties;
	PropNode* temp = (*firstProps).head;
	printf("Property/value combos of %s: ", (*firstName).name);
	while (temp != NULL) {
		printf("(%s, %s) ", (*temp).property, (*temp).value);
		temp = (*temp).next;
	}
	NameNode* secondName = getNN(1, list);
	printf("Name of second node: %s\n", (*secondName).name);
	PropLinkedList* secondProps = (*secondName).properties;
	temp = (*secondProps).head;
	printf("Property/value combos of %s: ", (*secondName).name);
	while (temp != NULL) {
		printf("(%s, %s) ", (*temp).property, (*temp).value);
		temp = (*temp).next;
	}

}

int main(void) {

	NameLinkedList* nll = newNameLinkedList();
	PropLinkedList* pll = newPropLinkedList();

	// BEGIN TESTING NAMENODE

	printf("TESTING NAMENODE -----------------------------------------------------\n\n");

	char *name = "Corporations";
	NameNode* corps = newNameNode(name);

	name = "Fruits";
	NameNode* fruits = newNameNode(name);

	printf("Name stored in corps NameNode: %s\n", (*corps).name);
	printf("Name stored in fruits NameNode: %s\n\n", (*fruits).name);

	// END TESTING NAMENODE

	// BEGIN TESTING PROPNODE

	printf("TESTING PROPNODE ------------------------------------------------------\n\n");

	char* value = "Apple";
	char* prop = "Mac";
	PropNode* pn1 = newPropNode(value, prop);

	value = "Microsoft";
	prop = "Windows";
	PropNode* pn2 = newPropNode(value, prop);

	printf("Property and value stored in pn1: %s, %s\n", (*pn1).property, (*pn1).value);
	printf("Property and value stored in pn2: %s, %s\n\n", (*pn2).property, (*pn2).value);

	// END TESTING PROPNODE

	// BEGIN TESTING ADDNNTONLL AND NAMELINKEDLIST

	printf("TESTING addNNtoNLL AND NAMELINKEDLIST ---------------------------------\n\n");

	printf("Adding corps to empty NLL\n");
	addNNtoNLL(corps, nll);
	printf("Size of NLL after adding: %d\n\n", (*nll).size);

	printf("Adding fruits to NLL of current size 1\n");
	addNNtoNLL(fruits, nll);
	printf("Size of NLL after adding: %d\n\n", (*nll).size);

	printf("Test that head was set correctly: %s (should be 'Corporations')\n", (nll->head)->name);
	printf("Test that tail was set correctly: %s (should be 'Fruits')\n", (nll->tail)->name);
	printf("Test that head's next is tail: %s (should be 'Fruits')\n\n", ((nll->head)->next)->name);

	// END TESTING ADDNNTONLL AND NAMELINKEDLIST

	// BEGIN TESTING ADDPNTOPLL AND PROPLINKEDLIST

	printf("TESTING addPNtoPLL AND PROPLINKEDLIST ---------------------------------\n\n");

	printf("Adding pn1 to empty PLL\n");
	addPNtoPLL(pn1, pll);
	printf("Size of PLL after adding: %d\n\n", (*pll).size);

	printf("Adding pn2 to PLL of current size 1\n");
	addPNtoPLL(pn2, pll);
	printf("Size of PLL after adding: %d\n\n", (*nll).size);

	printf("Test that head was set correctly: %s (should be 'Mac')\n", (pll->head)->property);
	printf("Test that tail was set correctly: %s (should be 'Windows')\n", (pll->tail)->property);
	printf("Test that head's next is tail: %s (should be 'Windows')\n\n", ((pll->head)->next)->property);

	// END TESTING ADDPNTOPLL AND PROPLINKEDLIST

	// BEGIN TESTING ADD FUNCTION

	// scan list for *name
		// if found
			// pull that NameNode's PLL
				// if null (which shouldn't happen)
					// create new PLL
					// create new PN, attach to the new PLL
					// attach the new PLL to the parent NameNode
				// else
					// scan the PLL for *property
						// if found
							// set the PN's value field = *value
						// else
							// create new PN, attach to the PLL
		// else (not found)
			// create new NameNode
			// create new PLL
			// create new PN using *property and *value, attach to the new PLL
			// attach the new PLL to the new NameNode
			// attach the new NameNode to the end of list

	printf("TESTING ADD FUNCTION --------------------------------------------------\n\n");

	name = "Corporations";
	char* property = "Apple";
	value = "iPod";

	testOverallAdd(name, property, value, nll);




	return EXIT_SUCCESS;
}
