/* bfi - a brainfuck interpreter
 *
 * This software is licensed under the MIT License, see LICENSE for more info
 * Find the most recent version at http://justesen.zxq.net/src/bfi
 *
 * Usage: bfi [options] file
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PRG_NAME "bfi" /* Program name */
#define VERSION  "0.2" /* Version number */


/* Holds user specified options */
struct conf {
	FILE *in; /* input file (brainfuck source) */
	int dump; /* dump memory cells if met while interpreting */
	int eof;  /* if change on eof, then value to replace EOF */
	int warn; /* print warnings */
};


/* Doubly linked list */
struct node {
	int data;          /* memory cell content */
	int cell;          /* cell number */
	struct node *prev; /* pointer to previous memory cell */
	struct node *next; /* pointer to next memory cell */
};


int interpret(const int, struct node **, struct conf *);
void loop(struct node **, struct conf *);
void next_cell(struct node **);
void prev_cell(struct node **, struct conf *);
void get_cell_data(struct node **, struct conf *);
void skip_loop(FILE *);
void mem_dump(struct node *);
void parse_args(const int, char **, struct conf *);
int parens_match(FILE *);
void exit_err(const char *, ...);
void warning(const char *, ...);
void help(void);
void version(void);
int insert_last(struct node **);
int insert_first(struct node **);
void free_mem(struct node *);


/* Brainfuck interpreter
 *
 * Parameter(s): argc - number of arguments
 *               argv - arguments
 *
 * Return: 0 on succes
 *         1 otherwise
 */
int main(int argc, char *argv[])
{
	struct conf opts;
	struct node *ptr;
	int c;

	opts.in = stdin;
	opts.dump = 9999;
	opts.eof = 9999;
	opts.warn = 0;

	parse_args(argc, argv, &opts);

	if ((ptr = malloc(sizeof *ptr)) == NULL) {
		exit_err("memory allocation failure");
	}
	ptr->data = 0;
	ptr->cell = 0;
	ptr->prev = NULL;
	ptr->next = NULL;

	if (!parens_match(opts.in)) {
		exit_err("parens don't match");
	}
	while ((c = fgetc(opts.in)) != EOF) {
		interpret(c, &ptr, &opts);
	}
	free_mem(ptr);
	fclose(opts.in);

	return 0;
}


/* Interpret character from brainfuck source
 *
 * Parameter(s): c    - character to be interpreted
 *               ptr  - pointer to current memory cell
 *               opts - options
 *
 * Return: 0 if a loop ends (a ] is met)
 *         1 otherwise
 */
int interpret(const int c, struct node **ptr, struct conf *opts)
{
	if (c == opts->dump) {
		mem_dump(*ptr);
	}
	switch (c) {
		case '>':
			next_cell(ptr);
			break;
		case '<':
			prev_cell(ptr, opts);
			break;
		case '+':
			++(*ptr)->data;
			break;
		case '-':
			if (--(*ptr)->data == -1 && opts->warn) {
				warning("value of cell #%d is negative",
				        (*ptr)->cell);
			}
			break;
		case '.':
			putchar((*ptr)->data);
			break;
		case ',':
			get_cell_data(ptr, opts);
			break;
		case '[':
			loop(ptr, opts);
			break;
		case ']':
			return 0;
	}
	return 1;
}


/* Goto next cell '>'
 *
 * Parameter(s): ptr  - pointer to current memory cell
 *               opts - options
 */
void next_cell(struct node **ptr)
{
	if ((*ptr)->next == NULL) {
		if (!insert_last(ptr)) {
			exit_err("memory allocation failure");
		}
	} else {
		*ptr = (*ptr)->next;
	}
}


/* Goto previous cell '<'
 *
 * Parameter(s): ptr  - pointer to current memory cell
 *               opts - options
 */
void prev_cell(struct node **ptr, struct conf *opts)
{
	if ((*ptr)->prev == NULL) {
		if (!insert_first(ptr)) {
			exit_err("memory allocation failure");
		}
		if ((*ptr)->cell == -1 && opts->warn) {
			warning("you reached a 'negative' memory cell");
		}
	} else {
		*ptr = (*ptr)->prev;
	}
}


/* Get data from input to cell ','
 *
 * Parameter(s): ptr  - pointer to current memory cell
 *               opts - options *
 */
void get_cell_data(struct node **ptr, struct conf *opts)
{
	int tmp = getchar();

	if (tmp == EOF) {
		if (opts->warn) {
			warning("encountered EOF while reading input");
		}
		if (opts->eof != 9999) {
			(*ptr)->data = opts->eof;
		}
	} else {
		(*ptr)->data = tmp;
	}
}

/* Handle loop
 *
 * Parameter(s): ptr  - pointer to current memory cell
 *               opts - options
 */
void loop(struct node **ptr, struct conf *opts)
{
	int c;
	fpos_t loop_start;

	if ((*ptr)->data <= 0) {
		skip_loop(opts->in);

		return;
	}
	fgetpos(opts->in, &loop_start);

	while ((*ptr)->data > 0) {
		fsetpos(opts->in, &loop_start);

		while ((c = fgetc(opts->in)) != EOF) {
			if (!interpret(c, ptr, opts)) {
				break;
			}
		}
	}
}


/* Skip a loop if the value of the current memory cell is initially 0
 *
 * Parameter(s): in - input file (brainfuck source)
 */
void skip_loop(FILE *in)
{
	int depth = 0;
	int c;

	while ((c = fgetc(in)) != EOF) {
		if (c == '[') {
			++depth;
		} else if (c == ']') {
			if (depth == 0) {
				break;
			} else {
				--depth;
			}
		}
	}
}


/* Print the cell number and content of the memory cells
 *
 * Parameter(s): ptr - pointer to current memory cell
 */
void mem_dump(struct node *ptr)
{
	while (ptr->prev != NULL) {
		ptr = ptr->prev;
	}
	printf("cell no | data (int) | data (char)\n");

	while (ptr != NULL) {
		printf("%7d | %10d | %11c\n",
		       ptr->cell,
		       ptr->data,
		       isgraph(ptr->data) ? ptr->data : ' ') ;

		ptr = ptr->next;
	}
}


/* Parse command line arguments
 *
 * Parameter(s): argc - number of arguments
 *               argv - arguments
 *               opts - options
 */
void parse_args(const int argc, char *argv[], struct conf *opts)
{
	int i;

	for (i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "-d", 2) == 0
		 || strncmp(argv[i], "--dump", 6) == 0) {
			if (i + 1 < argc && strlen(argv[i + 1]) == 1) {
				opts->dump = argv[++i][0];
			} else {
				exit_err("missing <char> after -d (--dump)");
			}
		} else if (strncmp(argv[i], "-e", 2) == 0
		        || strncmp(argv[i], "--eof", 5) == 0) {
			if (i + 1 < argc && strlen(argv[i + 1]) <= 2) {
				opts->eof = atoi(argv[++i]);
			} else {
				exit_err("missing <num> after -e (--eof)");
			}
		} else if (strncmp(argv[i], "-h", 2) == 0
		        || strncmp(argv[i], "--help", 6) == 0) {
			help();		
		} else if (strncmp(argv[i], "-v", 2) == 0
		        || strncmp(argv[i], "--version", 9) == 0) {
			version();
		} else if (strncmp(argv[i], "-w", 2) == 0
		        || strncmp(argv[i], "--warnings", 10) == 0) {
			opts->warn = 1;
		} else if (opts->in == stdin){
			if ((opts->in = fopen(argv[i], "r")) == NULL) {
				exit_err("can't read file %s", argv[i]);
			}
		} else {
			warning("unknown argument %s", argv[i]);
		}
	}
	if (opts->in == stdin) {
		exit_err("no input file");
	}
}


/* Check if there are as many opening parens '[' as closing parens ']'
 *
 * Parameter(s): in - input file (brainfuck source)
 *
 * Return: 0 if parens match
 *         1 if they do match
 */
int parens_match(FILE *in)
{
	int open_parens = 0;
	int close_parens = 0;
	int c;
	fpos_t file_start;

	fgetpos(in, &file_start);

	while ((c = fgetc(in)) != EOF) {
		if (c == '[') {
			++open_parens;
		} else if (c == ']') {
			++close_parens;
		}
	}
	fsetpos(in, &file_start);

	return open_parens == close_parens;
}


/* Display help information and exit succesfully
 */
void help(void)
{
	printf("\
%s - a brainfuck interpreter\n\
\n\
Usage: %s [options] file\n\
\n\
Options:\n\
  -d --dump <char>  Dump memory when <char> is met\n\
  -e --eof <num>    Replace EOF with <num> if such is encountered (default\n\
                    is no change)\n\
  -h --help         Display this information\n\
  -v --version      Display program name and version number\n\
  -w --warnings     Print warnings\n\
\n\
A man page should have come with %s, see \n\
  man %s\n\
for more info and examples\n",
	       PRG_NAME, PRG_NAME, PRG_NAME, PRG_NAME);

	exit(EXIT_SUCCESS);
}


/* Display program name and version number - exit succesfully
 */
void version(void)
{
	printf("\
%s %s\n\
\n\
For license and copyright information see the LICENSE file, which should\n\
have been distributed with the software.\n",
	       PRG_NAME, VERSION);

	exit(EXIT_SUCCESS);
}


/* Print error message and usage information before exiting indicating an error
 *
 * Parameter(s): ferrs - formatted error string
 *               ...   - arguments to ferrs
 */
void exit_err(const char *ferrs, ...)
{
	va_list ap;

	va_start(ap, ferrs);

	fprintf(stderr, "%s: error: ", PRG_NAME);
	vfprintf(stderr, ferrs, ap);
	fputc('\n', stderr);

	va_end(ap);

	exit(EXIT_FAILURE);
}


/* Print error message and usage information before exiting indicating an error
 *
 * Parameter(s): fwarns - formatted warning string
 *               ...    - arguments to ferrs
 */
void warning(const char *fwarns, ...)
{
	va_list ap;

	va_start(ap, fwarns);

	fprintf(stderr, "%s: warning: ", PRG_NAME);
	vfprintf(stderr, fwarns, ap);
	fputc('\n', stderr);

	va_end(ap);
}


/* Insert new lasst memory cell
 *
 * Parameter(s): last - pointer to (current) last memory cell
 *
 * Return: 0 on memory allocation failure
 *         1 otherwise
 */
int insert_last(struct node **last)
{
	struct node *new = malloc(sizeof *new);

	if (new == NULL) {
		return 0;
	}
	new->data = 0;
	new->cell = (*last)->cell + 1;
	new->next = NULL;
	new->prev = *last;
	(*last)->next = new;
	*last = new;

	return 1;
}


/* Insert new first memory cell
 *
 * Parameter(s): first - pointer to (current) first memory cell
 *
 * Return: 0 on memory allocation failure
 *         1 otherwise
 */
int insert_first(struct node **first)
{
	struct node *new = malloc(sizeof *new);

	if (new == NULL) {
		return 0;
	}
	new->data = 0;
	new->cell = (*first)->cell - 1;
	new->next = *first;
	new->prev = NULL;
	(*first)->prev = new;
	*first = new;

	return 1;
}


/* Free memory cells
 */
void free_mem(struct node *ptr)
{
	struct node *tmp;

	while (ptr->next != NULL) {
		ptr = ptr->next;
	}
	while (ptr != NULL) {
		tmp = ptr;
		ptr = ptr->prev;

		free(tmp);
	}
}

