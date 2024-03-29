
/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/* A program to simulate cache behaviour on a valgrind memory trace. */

/* The input is a description of a set of caches, and a memory trace.
   We run the memory trace on each of the caches, and report statistics. */


#include "global.h"
#include "caches.h"

Boolean debug = FALSE;
Boolean manualDebug = TRUE;


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void usage(void)
{
    fprintf (stderr,"usage: caches [-d] cache-descriptions-file  memory-trace-file\n");
}

void scanargs(String s)
{
    /* check each character of the option list for
       its meaning. */

    while (*++s != '\0')
        switch (*s)
            {

            case 'D': /* debug option */
                debug = TRUE;
                if (debug)
                    {
                        debug_file = fopen("DEBUG_LOG", "w");
                        if (debug_file == NULL)
                            {
                                fprintf(stderr, "Cannot open DEBUG_LOG\n");
                                debug = FALSE;
                            }
                    }
                break;

            case 'd': /* debug option */
                debug = TRUE;
                if (debug)
                    {
                        debug_file = fopen("DEBUG_LOG", "w");
                        if (debug_file == NULL)
                            {
                                fprintf(stderr, "Cannot open DEBUG_LOG\n");
                                debug = FALSE;
                            }
                    }
                break;

            default:
                fprintf (stderr,"caches: Bad option %c\n", *s);
                usage();
                exit(1);
            }
}


/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int main(int argc, String *argv)
{
    /* main driver program.  There are two input 
       files.
       1. defines the caches
       2. defines the memory trace
    */
       
	if (manualDebug) {
		Read_Cache_Descriptions("test_definition");
		Init_Caches();
		Simulate_Caches("test_trace4.txt");
		Print_Cache_Statistics();
		Delete_Caches();
	}
	else {
	    /* Process all arguments. */
	    /* skip program name */
	    argc--, argv++;
	    while ((argc > 1) && (**argv == '-'))
	        {
	            scanargs(*argv);
	            argc--, argv++;
	        }

	    if (argc != 2)
	        {
	            usage();
	            exit(-1);
	        }

	    Read_Cache_Descriptions(argv[0]);		// in read_cds.c
	    Init_Caches();							// in cds.c

	    Simulate_Caches(argv[1]);				// in simulate.c

	    Print_Cache_Statistics();				// in cds.c
	    Delete_Caches();						// in cds.c
	}

    exit(0);
}
