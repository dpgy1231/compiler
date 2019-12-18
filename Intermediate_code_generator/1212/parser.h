#pragma once
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

//��ĳ�� ��ũ��
#define	NO_KEYWORDS 7
#define ID_LENGTH 12

//�ļ� ��ũ��
#define PS_SIZE 200

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
enum nodeNumber {
	ACTUAL_PARAM, ADD, ADD_ASSIGN, ARRAY_VAR, ASSIGN_OP,
	CALL, COMPOUND_ST, CONST_NODE, DCL, DCL_ITEM,
	DCL_LIST, DCL_SPEC, DIV, DIV_ASSIGN, EQ,
	ERROR_NODE, EXP_ST, FOMAL_PARA, FUNC_DEF, FUNC_HEAD,
	GE, GT, IDENT, IF_ELSE_ST, IF_ST,
	INDEX, INT_NODE, LE, LOGICAL_AND, LOGICAL_NOT,
	LOGICAL_OR, LT, MOD, MOD_ASSIGN, MUL,
	MUL_ASSIGN, NE, NUMBER, PARAM_DCL, POST_DEC,
	POST_INC, PRE_DEC, PRE_INC, PROGRAM, RETURN_ST,
	SIMPLE_VAR, STAT_LIST, SUB, SUB_ASSIGN, UNARY_MINUS,
	VOID_NODE, WHILE_ST
};

//��ĳ�� �ڵ�
struct tokenType {
	int number;
	union {
		char* id2;
		char id[ID_LENGTH];
		int num;
	}value;
};


//�ļ�
typedef struct nodeType {
	struct tokenType token;
	enum { terminal, nonterm } noderep;
	struct nodeType* son;
	struct nodeType* brother;
} Node;

//��ĳ���Լ�
void lexicalError(int n);
int superLetter(char ch);
int superLetterOrDigit(char ch);
int getIntNum(char firstCharacter);
struct tokenType scanner();

//�ļ��Լ�
int meaningfulToken(struct tokenType token);
Node* parser();
Node* buildNode(struct tokenType token);
Node* buildTree(int nodeNumber, int rhsLength);
void printTree(Node* pt, int indent);
void printNode(Node* pt, int indent);

//�ļ� ����

extern int sp;
extern int stateStack[PS_SIZE];
extern int symbolStack[PS_SIZE];
extern Node* valueStack[PS_SIZE];
extern char *nodeName[];
extern int ruleName[];
extern char *tokenName[];


//��ĳ�� ����
extern char* keyword[NO_KEYWORDS];
extern enum tsymbol tnum[NO_KEYWORDS];
