#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define	NO_KEYWORDS 7
#define ID_LENGTH 12

void lexicalError(int n);
int superLetter(char ch);
int superLetterOrDigit(char ch);
int getIntNum(char firstCharacter, FILE* source_file);

char id[ID_LENGTH];
char ch;

struct tokenType {
	int number;
	union {
		char id[ID_LENGTH];
		int num;
	}value;
};

char *keyword[NO_KEYWORDS] = { "const", "else", "if","int","return","void","while" };

enum tsymbol {
	tnull = -1,
	tnot, tnotequ, tmod, tmodAssign, tident, tnumber,
	tand, tlparen, trparen, tmul, tmulAssign, tplus,
	tinc, taddAssign, tcomma, tminus, tdec, tsubAssign,
	tdiv, tdivAssign, tsemicolon, tless, tlesse, tassign,
	tequal, tgreat, tgreate, tlbracket, trbracket, teof,
	tconst, telse, tif, tint, treturn, tvoid,
	twhile, tlbrace, tor, trbrace
};

enum tsymbol tnum[NO_KEYWORDS] = { tconst, telse, tif, tint, treturn, tvoid, twhile };


struct tokenType scanner(FILE* source_file)
{
	struct tokenType token;
	int i, index;
	

	token.number = tnull;

	do {
		while (isspace(ch = fgetc(source_file)));
		if (superLetter(ch)) {
			i = 0;
			do {
				if (i < ID_LENGTH)	id[i++] = ch;
				ch = fgetc(source_file);
			} while (superLetterOrDigit(ch));
			if (i >= ID_LENGTH)	lexicalError(1);
			id[i] = '\0';
			ungetc(ch, stdin);
			for (index = 0; index < NO_KEYWORDS; index++)
				if (!strcmp(id, keyword[index]))	break;
			if (index < NO_KEYWORDS)
				token.number = tnum[index];
			else {
				token.number = tident;
				strcpy_s(token.value.id, ID_LENGTH, id);
			}
		}
		else if (isdigit(ch)) {
			token.number = tnumber;
			token.value.num = getIntNum(ch, source_file);
		}
		else {
			switch (ch) {
			case '/':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '*')
					do {
						while (ch != '*')	ch = fgetc(source_file);
						ch = fgetc(source_file);
					} while (ch != '/');
				else if (ch == '/')
					while (fgetc(source_file) != '\n');
				else if (ch == '=')	token.number = tdivAssign;
				else {
					token.number = tdiv;
					ungetc(ch, stdin);
				}
				break;
			case '!':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') { token.number = tnotequ; id[1] = ch; }
				else {
					token.number = tnot;
					ungetc(ch, stdin);
				}
				break;
			case '%':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') {
					token.number = tmodAssign;
					id[1] = ch;
				}
				else {
					token.number = tmod;
					ungetc(ch, stdin);
				}
				break;
			case '&':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '&') { token.number = tand; id[1] = ch; }
				else {
					lexicalError(2);
					ungetc(ch, stdin);
				}
				break;
			case '*':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') { token.number = tmulAssign; id[1] = ch; }
				else {
					token.number = tmul;
					ungetc(ch, stdin);
				}
				break;
			case '+':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '+') { token.number = tinc;  id[1] = ch;}
				else if (ch == '=') { token.number = taddAssign; id[1] = ch;}
				else {
					token.number = tplus;
					ungetc(ch, stdin);
				}
				break;
			case '-':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '-') { token.number = tdec; id[1] = ch; }
				else if (ch == '-') { token.number = tsubAssign; id[1] = ch; }
				else {
					token.number = tminus;
					ungetc(ch, stdin);
				}
				break;
			case '<':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') { token.number = tlesse; id[1] = ch; }
				else {
					token.number = tless;
					ungetc(ch, stdin);
				}
				break;
			case '=':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') { token.number = tequal; id[1] = ch; }
				else {
					token.number = tassign;
					ungetc(ch, stdin);
				}
				break;
			case '>':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '=') { token.number = tgreate; id[1] = ch; }
				else {
					token.number = tgreat;
					ungetc(ch, stdin);
				}
				break;
			case '|':
				id[0] = ch;
				ch = fgetc(source_file);
				if (ch == '|') { token.number = tor; id[1] = ch; }
				else {
					lexicalError(3);
					ungetc(ch, stdin);
				}
				break;
			case '(':	id[0] = ch; token.number = tlparen;	break;
			case ')':	id[0] = ch; token.number = trparen;	break;
			case ',':	id[0] = ch; token.number = tcomma;	break;
			case ';':	id[0] = ch; token.number = tsemicolon;	break;
			case '[':	id[0] = ch; token.number = tlbracket;	break;
			case ']':	id[0] = ch; token.number = trbracket;	break;
			case '{':	id[0] = ch; token.number = tlbrace;	break;
			case '}':	id[0] = ch; token.number = trbrace;	break;
			case EOF:	token.number = teof;	break;
			default: {
				printf("Current character : %c", ch);
				lexicalError(4);
				break;
			}
			}

		}
	} while (token.number == tnull);
	
	return token;
}

void lexicalError(int n) {
	printf(" *** Lexical Error : ");
	switch (n) {
	case 1: printf("an identifier length must be less than 12.\n");
		break;
	case 2: printf("next character must be &.\n");
		break;
	case 3: printf("next character must be |.\n");
		break;
	case 4: printf("invalid character!!!\n");
		break;
	}
}

int superLetter(char ch) {
	if (isalpha(ch) || ch == '_')	return 1;
	else return 0;
}

int superLetterOrDigit(char ch) {
	if (isalnum(ch) || ch == '_')	return 1;
	else return 0;
}

int getIntNum(char firstCharacter, FILE* source_file) {
	int num = 0;
	char ch;

	ch = firstCharacter;
	do {
		num = 10 * num + (int)(ch - '0');
		ch = fgetc(source_file);
	} while (isdigit(ch));

	ungetc(ch, stdin);
	return num;
}
void main(int argc, char *argv[]) {
	FILE *source_file;
	int i;
	struct tokenType token;

	if (argc != 2) {
		fprintf(stderr, "Usage : scanner <source file name>\n");
		exit(1);
	}

	if ((source_file = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "%s file not found \n", argv[1]);
		exit(-1);
	}

	do {
		for (i = 0; i<ID_LENGTH; i++)
			id[i] = ' ';
		token = scanner(source_file);
		fprintf(stdout, "Token ---> ");

		if (token.number == 5) {	//상수
			//for (i = 0; i<ID_LENGTH; i++)
				fprintf(stdout, "%-12d", token.value.num);
			fprintf(stdout, ": (%d, %d)\n", token.number, token.value.num);
		}
		else if (token.number == 4) {	//식별자
			for (i = 0; i<ID_LENGTH; i++)
				fprintf(stdout, "%c", id[i]);
			fprintf(stdout, ": (%d, %s)\n", token.number, token.value.id);
		}
		else {	//지정어
			if (isalpha(id[0])){
				for (i = 0; i < ID_LENGTH; i++)
					fprintf(stdout, "%c", id[i]);
			fprintf(stdout, ": (%d, 0)\n", token.number);
			}
			else {	//구분자,연산자
				for (i = 0; i < ID_LENGTH; i++)
					fprintf(stdout, "%c", id[i]);
				fprintf(stdout, ": (%d, 0)\n", token.number);
			}

		}
	} while (!feof(source_file));
	fclose(source_file);
}
