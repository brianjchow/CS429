
/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *input;

typedef short Boolean;
#define TRUE 1
#define FALSE 0

Boolean debug = FALSE;
Boolean decode = FALSE;

typedef char *STRING;

int map[256];
int demap[256];

void DoIt(STRING name);

char encode(int by)
{
    if ((0 <= by) && (by < 26)) return(by + 'A');
    if ((0 < 26) && (by <= 31)) return(by - 26 + '0');
    return('?');
}


void Set_up_map(void)
{
    int i;

    if (decode)
        {
            for (i = 0; i < 256; i++)
                demap[i] = EOF;
            for (i = 0; i < 32; i++)
                demap[encode(i)] = i;
        }
    else
        {
            for (i = 0; i < 256; i++)
                map[i] = '?';
            for (i = 0; i < 32; i++)
                map[i] = encode(i);

        }
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

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

            case 'd': /* decode option */
                decode = TRUE;
                break;

            default:
                fprintf (stderr,"5bit: Bad option %c\n", *s);
                fprintf (stderr,"usage: 5bit [-d] file\n");
                exit(1);
            }
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int main(int argc, STRING *argv)
{
    Boolean namenotgiven = TRUE;

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
                    namenotgiven = FALSE;
                    input = fopen(*argv,"r");
                    if (input == NULL)
                        {
                            fprintf (stderr, "Can't open %s\n",*argv);
                        }
                    else
                        {
                            DoIt(*argv);
                            fclose(input);
                        }
                }
        }

    if (namenotgiven)
        {
            input = stdin;
            DoIt(NULL);
        }

    exit(0);
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int number_of_chars = 0;

void print_byte(int by)
{

    char c;

    if (decode)
        c = by;
    else
        c = encode(by);

    putchar(c);

    number_of_chars += 1;

    if (!decode)
        {
            if (number_of_chars >= 72)
                {
                    putchar('\n');
                    number_of_chars = 0;
                }
        }
}


#define END_OF_BITS 0xF

int get_bit()
{
    static int number_of_bits = 0;
    static int bits[8];

    if (number_of_bits <= 0)
        {
            int by;
            int num_input_bits;
            int i;

            int c;
            do
                {
                    c = getc(input);
                    if (c == EOF) return(END_OF_BITS);
                    
                    if (decode)
                        {
                            by = demap[c];
                            num_input_bits = 5;
                        }
                    else
                        {
                            by = c;
                            num_input_bits = 8;
                        }
                } while (by == EOF);
                    
            for (i = 0; i < num_input_bits; i++)
                {
                    bits[i] = by & 0x1;
                    by = by >> 1;
                    number_of_bits += 1;
                }
        }
    
    number_of_bits -= 1;
    return(bits[number_of_bits]);
}

int output_byte = 0;
int number_of_bits = 0;

void put_bit(int b)
{
    int num_output_bits;

    output_byte = (output_byte << 1) | b;
    number_of_bits += 1;

    if (decode)
        {
            num_output_bits = 8;
        }
    else
        {
            num_output_bits = 5;
        }

    if (number_of_bits >= num_output_bits)
        {
            print_byte(output_byte);
            output_byte = 0;
            number_of_bits = 0;
        }
}

void flush_bits()
{
    int i;
    for (i = 0; i < 4; i++)
        put_bit(0);
    if (number_of_chars != 0)
        putchar('\n');
}


void DoIt(STRING name)
{
    int b;

    Set_up_map();

    while ((b = get_bit()) != END_OF_BITS)
        put_bit(b);

    if (!decode)
        flush_bits();
}