#include "parser.h"
#include "MiniC.tbl"

char *nodeName[] = {
	"ACTUAL_PARAM",	"ADD",		   "ADD_ASSIGN",	"ARRAY_VAR",   "ASSIGN_OP",
	"CALL",			"COMPOUND_ST", "CONST_NODE",	"DCL",		   "DCL_ITEM",
	"DCL_LIST",		"DCL_SPEC",	   "DIV",			"DIV_ASSIGN",  "EQ",
	"ERROR_NODE",	"EXP_ST",	   "FOMAL_PARA",	"FUNC_DEF",	   "FUNC_HEAD",
	"GE",			"GT",		   "IDENT",			"IF_ELSE_ST",  "IF_ST",
	"INDEX",		"INT_NODE",	   "LE",			"LOGICAL_AND", "LOGICAL_NOT",
	"LOGICAL_OR",	"LT",		   "MOD",			"MOD_ASSIGN",  "MUL",
	"MUL_ASSIGN",	"NE",		   "NUMBER",		"PARAM_DCL",   "POST_DEC",
	"POST_INC",		"PRE_DEC",	   "PRE_INC",		"PROGRAM",	   "RETURN_ST",
	"SIMPLE_VAR",	"STAT_LIST",   "SUB",			"SUB_ASSIGN",  "UNARY_MINUS",
	"VOID_NODE",	"WHILE_ST"
};


int ruleName[] = {
	0,			PROGRAM,	 0,			  0,			 0,
	0,			FUNC_DEF,	 FUNC_HEAD,	   DCL_SPEC,	 0,
	0,			0,			 0,			   CONST_NODE,	 INT_NODE,
	VOID_NODE,	0,			 FOMAL_PARA,   0,			 0,
	0,			0,			 PARAM_DCL,	   COMPOUND_ST, DCL_LIST,
	DCL_LIST,	0,			 0,			   DCL,		 0,
	0,			DCL_ITEM,	 DCL_ITEM,	   SIMPLE_VAR,	 ARRAY_VAR,
	0,			0,			 STAT_LIST,	   0,			 0,
	0,			0,			 0,			   0,			 0,
	0,			EXP_ST,		 0,			   0,			 IF_ST,
	IF_ELSE_ST, WHILE_ST,	 RETURN_ST,	   0,			 0,
	ASSIGN_OP,	ADD_ASSIGN,  SUB_ASSIGN,   MUL_ASSIGN,	 DIV_ASSIGN,
	MOD_ASSIGN,	0,			 LOGICAL_OR,   0,			 LOGICAL_AND,
	0,			EQ,			 NE,		   0,			 GT,
	LT,			GE,			 LE,		   0,			 ADD,
	SUB,		0,			 MUL,		   DIV,			 MOD,
	0,			UNARY_MINUS, LOGICAL_NOT,  PRE_INC,		 PRE_DEC,
	0,			INDEX,		 CALL,		   POST_INC,	 POST_DEC,
	0,			0,			 ACTUAL_PARAM, 0,			 0,
	0,			0,			 0
};

char *tokenName[] = {
	/* 0          1      2       3        4           5 */
	"!",        "!=",   "%",    "%=",   "ident",    "%number",
	/* 6          7      8       9       10          11 */
	"&&",       "(",    ")",    "*",    "*=",       "+",
	/*12         13     14      15       16          17 */
	"++",       "+=",   ",",    "-",    "--",       "-=",
	/*18         19     20      21       22          23 */
	"/",        "/=",   ";",    "<",    "<=",       "=",
	/*24         25     26      27       28          29 */
	"==",       ">",    ">=",   "[",    "]",        "eof",
	/* ............ word symbols ...................... */
	/*30         31     32      33       34          35 */
	"const",    "else", "if",   "int",  "return",   "void",
	/*36         37     38      39                      */
	"while",    "{",    "||",   "}"
};

char id[ID_LENGTH];
char* keyword[NO_KEYWORDS] = { "const", "else", "if","int","return","void","while" };
enum tsymbol tnum[NO_KEYWORDS] = { tconst, telse, tif, tint, treturn, tvoid, twhile };

char ch;
int sp;
int stateStack[PS_SIZE];
int symbolStack[PS_SIZE];
Node* valueStack[PS_SIZE];

struct tokenType scanner()
{
	struct tokenType token;
	int i, index;


	token.number = tnull;

	do {
		while (isspace(ch = getchar()));
		if (superLetter(ch)) {
			i = 0;
			do {
				if (i < ID_LENGTH)	id[i++] = ch;
				ch = getchar();
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
			token.value.num = getIntNum(ch);
		}
		else {
			switch (ch) {
			case '/':
				id[0] = ch;
				ch = getchar();
				if (ch == '*')
					do {
						while (ch != '*')	ch = getchar();
						ch = getchar();
					} while (ch != '/');
				else if (ch == '/')
					while (getchar() != '\n');
				else if (ch == '=')	token.number = tdivAssign;
				else {
					token.number = tdiv;
					ungetc(ch, stdin);
				}
				break;
			case '!':
				id[0] = ch;
				ch = getchar();
				if (ch == '=') { token.number = tnotequ; id[1] = ch; }
				else {
					token.number = tnot;
					ungetc(ch, stdin);
				}
				break;
			case '%':
				id[0] = ch;
				ch = getchar();
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
				ch = getchar();
				if (ch == '&') { token.number = tand; id[1] = ch; }
				else {
					lexicalError(2);
					ungetc(ch, stdin);
				}
				break;
			case '*':
				id[0] = ch;
				ch = getchar();
				if (ch == '=') { token.number = tmulAssign; id[1] = ch; }
				else {
					token.number = tmul;
					ungetc(ch, stdin);
				}
				break;
			case '+':
				id[0] = ch;
				ch = getchar();
				if (ch == '+') { token.number = tinc;  id[1] = ch; }
				else if (ch == '=') { token.number = taddAssign; id[1] = ch; }
				else {
					token.number = tplus;
					ungetc(ch, stdin);
				}
				break;
			case '-':
				id[0] = ch;
				ch = getchar();
				if (ch == '-') { token.number = tdec; id[1] = ch; }
				else if (ch == '-') { token.number = tsubAssign; id[1] = ch; }
				else {
					token.number = tminus;
					ungetc(ch, stdin);
				}
				break;
			case '<':
				id[0] = ch;
				ch = getchar();
				if (ch == '=') { token.number = tlesse; id[1] = ch; }
				else {
					token.number = tless;
					ungetc(ch, stdin);
				}
				break;
			case '=':
				id[0] = ch;
				ch = getchar();
				if (ch == '=') { token.number = tequal; id[1] = ch; }
				else {
					token.number = tassign;
					ungetc(ch, stdin);
				}
				break;
			case '>':
				id[0] = ch;
				ch = getchar();
				if (ch == '=') { token.number = tgreate; id[1] = ch; }
				else {
					token.number = tgreat;
					ungetc(ch, stdin);
				}
				break;
			case '|':
				id[0] = ch;
				ch = getchar();
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

int getIntNum(char firstCharacter) {
	int num = 0;
	char ch;
	ch = firstCharacter;
	do {
		num = 10 * num + (int)(ch - '0');
		ch = getchar();
	} while (isdigit(ch));

	ungetc(ch, stdin);
	return num;
}

//파서코드
//AST코드
Node* parser() {
	extern int parsingTable[NO_STATES][NO_SYMBOLS + 1];
	extern int leftSymbol[NO_RULES + 1], rightLength[NO_RULES + 1];
	int errcnt = 0;
	int entry, ruleNumber, lhs;
	int currentState;
	struct tokenType token;
	Node* ptr;

	sp = 0; stateStack[sp] = 0;
	token = scanner();
	while (1) {
		currentState = stateStack[sp];
		entry = parsingTable[currentState][token.number];
		if (entry > 0) {
			sp++;
			symbolStack[sp] = token.number;
			stateStack[sp] = entry;
			valueStack[sp] = meaningfulToken(token) ? buildNode(token) : NULL;
			token = scanner();
		}
		else if (entry < 0) {
			ruleNumber = -entry;
			if (ruleNumber == GOAL_RULE) {
				return ptr;

			}
			//printf("reduced rule number = %d\n", ruleNumber);
			ptr = buildTree(ruleName[ruleNumber], rightLength[ruleNumber]);
			sp -= rightLength[ruleNumber];
			lhs = leftSymbol[ruleNumber];
			currentState = parsingTable[stateStack[sp]][lhs];
			sp++;
			symbolStack[sp] = lhs;
			stateStack[sp] = currentState;
			valueStack[sp] = ptr;
		}
		else {
			printf(" === error in source === \n");
			exit(1);
		}
	}
}

int meaningfulToken(struct tokenType token) {
	if ((token.number == tident) || (token.number == tnumber)) return 1;
	else return 0;
}

Node* buildNode(struct tokenType token) {

	Node* ptr;
	ptr = (Node *)malloc(sizeof(Node));
	if (!ptr) {
		printf("malloc error in buildNode()\n");
		exit(1);
	}
	ptr->token = token;
	ptr->noderep = terminal;
	ptr->son = ptr->brother = NULL;
	return ptr;
}

Node* buildTree(int nodeNumber, int rhsLength) {
	int i, j, start;
	Node* first;
	Node* ptr;

	i = sp - rhsLength + 1;
	while (i <= sp && valueStack[i] == NULL) i++;
	if (!nodeNumber && i > sp) return NULL;
	start = i;
	while (i <= sp - 1) {
		j = i + 1;
		while (j <= sp && valueStack[j] == NULL) j++;
		if (j <= sp) {
			ptr = valueStack[i];
			while (ptr->brother) ptr = ptr->brother;
			ptr->brother = valueStack[j];
		}
		i = j;
	}
	first = (start > sp) ? NULL : valueStack[start];
	if (nodeNumber) {
		ptr = (Node*)malloc(sizeof(Node));
		if (!ptr) {
			printf("malloc error in buildTree()\n");
			exit(1);
		}
		ptr->token.number = nodeNumber;
		ptr->token.value.id2 = NULL;

		ptr->noderep = nonterm;
		ptr->son = first;
		ptr->brother = NULL;
		return ptr;
	}
	else return first;
}

void printNode(Node* pt, int indent) {
	extern FILE* astFile;
	int i;

	for (i = 1; i <= indent; i++) fprintf(astFile, " ");
	if (pt->noderep == terminal) {
		if (pt->token.number == tident)
			fprintf(astFile, " Terminal: %s", pt->token.value.id);
		else if (pt->token.number == tnumber)
			fprintf(astFile, " Terminal: %d", pt->token.value.num);
	}
	else {
		int i;
		i = (int)(pt->token.number);
		fprintf(astFile, " Nonterminal: %s", nodeName[i]);
	}
	fprintf(astFile, "\n");

}

void printTree(Node *pt, int indent) {
	Node* p = pt;
	while (p != NULL) {
		printNode(p, indent);
		if (p->noderep == nonterm) printTree(p->son, indent + 3);
		p = p->brother;
	}
}