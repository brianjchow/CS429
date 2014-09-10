
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

void DoIt(STRING name);

char encode(int by)
{
    if ((0 <= by) && (by < 26)) return(by + 'A');
    if ((0 < 26) && (by <= 31)) return(by - 26 + '0');
    return('?');
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


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void DoIt(STRING name)
{
    int bits[16];
    int m = 0;
    int i;
    int c;
    
    while ((c = getc(input)) != EOF)
        {
            /* put the individual bits into our bit array */
            for (i = 0; i < 8; i++)
                {
                    bits[m+7-i] = c & 0x1;
                    c = c >> 1;
                }
            m = m + 8;
            
            /* now output every 5 bit chunk we have */
            while(m >= 5)
                {
                    int b = 0;
                    
                    //Merge 5 bits into b.
                    for(i = 0; i < 5; i++)
                        {
                            b = b << 1;
                            b = b | bits[i];
                        }
                    
                    //Move bits down the array.
                    for(i = 5; i < m; i++)
                        {
                            bits[i - 5] = bits[i];
                        }
                    m = m - 5;
                    
                    print_byte(b);
                }
        }

    if (m > 0)
        {
            //Merge m bits into b.
            int b = 0;
            for(i = 0; i < m; i++)
                {
                    b = b << 1;
                    b = b | bits[i];
                }
            b = b << (5-m);
            print_byte(b);
        }
    if (number_of_chars > 0) putchar('\n');
}