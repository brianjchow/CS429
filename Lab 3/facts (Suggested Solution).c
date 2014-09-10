
/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE *facts_file;
FILE *quest_file;

typedef short Boolean;
#define TRUE 1
#define FALSE 0

Boolean debug = FALSE;

typedef char *STRING;
#define STRING_EQ(a,b) (strcmp(a,b) == 0)

#define CAST(t,e) ((t)(e))
#define TYPED_MALLOC(t) CAST(t*, malloc(sizeof(t)))

void DoIt(void);

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void panic(STRING why)
{
    fprintf(stderr, "%s\n", why);
    exit(-1);
}

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

Boolean fail = FALSE;

void scanargs(STRING s)
{
    /* check each character of the option list for
       its meaning. */

    while (*++s != '\0')
        switch (*s)
            {

            case 'D': /* debug option */
                debug = TRUE;
                break;

            default:
                fprintf (stderr,"facts: Bad option %c\n", *s);
                fprintf (stderr,"usage: facts [-D] file\n");
                fail = TRUE;
            }
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int which_file_name = 0;


int main(int argc, STRING *argv)
{

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
                    which_file_name += 1;
                    if (which_file_name == 1)
                        {
                            facts_file = fopen(*argv,"r");
                            if (facts_file == NULL)
                                {
                                    fprintf (stderr, "Can't open facts file %s\n",*argv);
                                    fail = TRUE;
                                }
                        }
                    else if (which_file_name == 2)
                        {
                            quest_file = fopen(*argv,"r");
                            if (quest_file == NULL)
                                {
                                    fprintf (stderr, "Can't open question file %s\n",*argv);
                                    fail = TRUE;
                                }
                        }
                    else 
                        {
                            fprintf (stderr, "Two many files: %s\n",*argv);
                            fail = TRUE;
                        }
                }
        }

    if (which_file_name == 1)
        {
            quest_file = stdin;
        }

    if (fail)
        exit(-1);

    DoIt();
    exit(0);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* Symbol table data structure */
/* Until we have some reason to do otherwise, we just keep a
   simple linked list of all the facts.  To add an element, it
   goes at the front of the list. */

struct ste
{
    struct ste *next;
    STRING x;
    STRING y;
    STRING z;
};

struct ste *ST = NULL;

struct ste *search_ST(STRING x, STRING y)
{
    if (debug) fprintf(stderr, "search_ST for (%s,%s)\n", x, y);
    struct ste *s = ST;
    while (s != NULL) 
        {
            if (debug) fprintf(stderr, "compare with (%s,%s)\n", s->x, s->y);
            if (STRING_EQ(s->x, x) && STRING_EQ(s->y, y))
                return(s);
            s = s->next;
        }
    return(NULL);
}

void insert_ST(STRING x, STRING y, STRING z)
{
    struct ste *s = TYPED_MALLOC(struct ste);
    if (s == NULL) panic("ran out of memory for symbol table entry");
    s->x = remember_string(x);
    s->y = remember_string(y);
    s->z = remember_string(z);
    
    /* simple linked list */
    s->next = ST;
    ST = s;
}


void empty_symbol_table(void)
{
    struct ste *s = ST;
    while (s != NULL)
        {
            /* save the next pointer */
            struct ste *next = s->next;
            /* free the symbol table element and all it's
               contents. */
            free(s->x);
            free(s->y);
            free(s->z);
            free(s);
            s = next;
        }
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void save_fact(STRING x, STRING y, STRING z)
{
    if (debug)
        fprintf(stderr, "new fact -- %s: %s = %s\n", x, y, z);

    /* check if new fact is already there */
    struct ste *s = search_ST(x,y);
    if (s != NULL)
        {
            /* remove old fact and save new one */
            free(s->z);
            s->z = remember_string(z);
        }
    else
        {
            /* add new fact to list */
            insert_ST(x,y,z);
        }

    free(x);
    free(y);
    free(z);
}

void find_and_print_fact(STRING x, STRING y)
{
    if (debug)
        fprintf(stderr, "new question -- %s: %s\n", x, y);

    /* find fact */
    struct ste *s = search_ST(x,y);
    if (s == NULL)
        {
            fprintf(stdout, "F %s: %s = unknown\n", x, y);
        }
    else
        {
            fprintf(stdout, "F %s: %s = %s\n", s->x, s->y, s->z);
        }

    free(x);
    free(y);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

Boolean first_letter(FILE *f, char marker)
{
    int c;

    /* skip leading space, blank lines, and comment lines,
       until we get an actual input line */

    while ((c = getc(f)) != EOF)
        {
            if (isspace(c)) continue;

            /* comment */
            if (c == '#')
                {
                    while (((c = getc(f)) != EOF) && (c != '\n')) /* skip comment line */;
                    continue;
                }

            if (c != marker) 
                {
                    /* bad leading character */
                    fprintf(stderr, "Fact line does not begin with %c (%c)\n", marker, c);
                    fail = TRUE;
                    return(FALSE);
                }
            return(TRUE);
        }

    /* EOF */
    return(FALSE);
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

STRING token_buffer = NULL;
int tb_length = 0;

STRING get_next_token(FILE *f, char delimiter)
{
    int token_length = 0;
    int c;

    /* keep reading until we get end of file, end of line, or the delimiter */
    while (((c = getc(f)) != EOF) && (c != '\n') && (c != delimiter))
        {
            /* skip any leading blanks */
            if (isblank(c) && (token_length == 0)) continue;

            /* make sure we have space for the new character in the buffer */
            token_length += 1;
            if (token_length > tb_length-1)
                {
                    int new_length = (tb_length == 0) ? 128 : (3*tb_length);
                    if (debug)
                        fprintf(stderr, "extend input buffer to %d bytes\n", new_length);

                    token_buffer = realloc(token_buffer, new_length);
                    if (token_buffer == NULL)
                        panic("ran out of memory for token buffer");
                    tb_length = new_length;
                }

            /* put the character into the buffer */
            token_buffer[token_length-1] = c;
            token_buffer[token_length] = '\0';
        }

    /* trim any trailing blanks */
    while ((token_length > 0) && isblank(token_buffer[token_length-1]))
        token_length -= 1;
    token_buffer[token_length] = '\0';

    if ((c == EOF) || (c != delimiter) || (token_length == 0))
        return(NULL);

    /* copy the string from the input buffer to its own memory */
    STRING tok = remember_string(token_buffer);
    return(tok);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int get_next_fact(STRING *x, STRING *y, STRING *z)
{
    if (!first_letter(facts_file, 'F')) return(EOF);

    *x = get_next_token(facts_file,':');
    if (*x != NULL) *y = get_next_token(facts_file,'=');
    if (*y != NULL) *z = get_next_token(facts_file,'\n');
    if ((*x != NULL) && (*y != NULL) && (*z != NULL)) return(1);
    if ((*x != NULL) || (*y != NULL) || (*z != NULL))
        fprintf(stderr, "Missing field for fact: %s: %s = %s\n", *x, *y, *z);
    return(EOF);
}

int get_next_question(STRING *x, STRING *y)
{
    if (!first_letter(quest_file, 'Q')) return(EOF);

    *x = get_next_token(quest_file,':');
    if (*x != NULL) *y = get_next_token(quest_file,'\n');
    if ((*x != NULL) && (*y != NULL)) return(1);
    if ((*x != NULL) || (*y != NULL))
        fprintf(stderr, "Missing field for question: %s: %s\n", *x, *y);
    return(EOF);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void DoIt(void)
{
    /* read all the facts */
    STRING x, y, z;
    while (get_next_fact(&x,&y,&z) != EOF)
        {
            save_fact(x,y,z);
        }

    /* now process all the questions */
    while (get_next_question(&x,&y) != EOF)
        {
            find_and_print_fact(x,y);
        }

    /* free up all dynamically allocated memory */
    fclose(facts_file);
    if (which_file_name == 2) fclose(quest_file);

    empty_symbol_table();
    free(token_buffer);
}

