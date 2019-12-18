#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define SYMTAB_SIZE 200
#define LABEL_SIZE 12

int base = 1, offset = 1, width = 1;
int lvalue;
int symlevel = 0;

FILE* sourceFile;
FILE* ucodeFile;
FILE* astFile;

enum opcodeEnum {
	notop, neg, incop, decop, dup,
	add, sub, mult, divop, modop, swp,
	andop, orop, gt, lt, ge, le, eq, ne,
	lod, str, ldc, lda,
	ujp, tjp, fjp,
	chkh, chkl,
	ldi, sti,
	call, ret, retv, ldp, proc, endop,
	nop, bgn, sym
};
char *opcodeName[] = {
	"notop",    "neg",	"inc",	"dec",	"dup",
	"add",	"sub",	"mult",	"div",	"mod",	"swp",
	"and",	"or",	"gt",	"lt",	"ge",	"le",	"eq",	"ne",
	"lod",	"str",	"ldc",	"lda",
	"ujp",	"tjp",	"fjp",
	"chkh",	"chkl",
	"ldi",	"sti",
	"call",	"ret",	"retv",	"ldp",	"proc",	"end",
	"nop",	"bgn",	"sym"
};
typedef enum { NON_SPECIFIER, VOID_TYPE, INT_TYPE } TypeSpecifier;
typedef enum { NON_QUALIFIER, FUNC_TYPE, PARAM_TYPE, CONST_TYPE, VAR_TYPE} TypeQuailfier;
typedef enum { ZERO_DIMENSION, ONE_DIMENSION } Dimension;
char *typeName[] = { "none",   "void",   "int" };
char *qualifierName[] = { "NONE", "FUNC_TYPE",  "PARAM_TYPE",  "CONST_TYPE", "VAR_TYPE" };
typedef struct tableType {
	char symbolName[ID_LENGTH];
	int typeSpecifier;
	int typeQualifier;
	int base;
	int offset;
	int width;					// size
	int initialValue;			// initial value
	int nextIndex;				// link to next entry.
} SymbolEntry;
SymbolEntry symbolTable[SYMTAB_SIZE];		// symbol table
int symbolTableTop;

void rv_emit(Node* ptr);
void processOperator(Node* ptr);
void processArrayVariable(Node *ptr, int typeSpecifier, int typeQaulifier);
void processSimpleVariable(Node *ptr, int typeSpecifier, int typeQualifier);
void processSimpleParamVariable(Node *ptr, int typeSpecifier, int typeQualifier);
void processArrayParamVariable(Node *ptr, int typeSpecifier, int typeQualifier);
void processDeclaration(Node *ptr);
void processParamDeclaration(Node *ptr);
void processStatement(Node *ptr);
void codeGen(Node *ptr);
void icg_error(int err);
void emitLabel(char* label);
void genLabel(char* label);
void emitFunc(char *value, int p, int q, int r);
void emit0(int opcode);
void emit1(int opcode, int operand);
void emit2(int opcode, int operand1, int operand2);
void emitSym(int operand1, int operand2, int operand3);
void emitJump(int value, char* label);
void processCondition(Node* ptr);
void processFuncHeader(Node *ptr);
void processFunction(Node* ptr);
int typeSize(int typeSpecifier);
int checkPredefined(Node *ptr);
void initSymbolTable();
int lookup(char *symbol);
int insert(char *symbol, int specifier, int qualifier, int base, int offset, int width, int initialValue);

void initSymbolTable() {
	symbolTableTop = 0;
}
int lookup(char *symbol) {
	int stIndex;
	for (stIndex = 0; stIndex < symbolTableTop; stIndex++)
		if ((strcmp(symbol, symbolTable[stIndex].symbolName) == 0) && (symbolTable[stIndex].nextIndex == symlevel))
			return stIndex;
	return -1;
}
int insert(char *symbol, int specifier, int qualifier, int base, int offset,
	int width, int initialValue) {
	strcpy(symbolTable[symbolTableTop].symbolName, symbol);
	symbolTable[symbolTableTop].typeSpecifier = specifier;
	symbolTable[symbolTableTop].typeQualifier = qualifier;
	symbolTable[symbolTableTop].base = base;
	symbolTable[symbolTableTop].offset = offset;
	symbolTable[symbolTableTop].width = width;
	symbolTable[symbolTableTop].initialValue = initialValue;
	symbolTable[symbolTableTop].nextIndex = symlevel;
	return ++symbolTableTop;
}

void genSym(int base) {
	int stIndex;
	//	fprintf(ucodeFile, "// Information for Symbol Table\n");
	for (stIndex = 0; stIndex <= symbolTableTop; stIndex++) {
		if ((symbolTable[stIndex].typeQualifier == FUNC_TYPE) || (symbolTable[stIndex].typeQualifier == CONST_TYPE)) 
			continue;
		if (base == symbolTable[stIndex].base)
			emitSym(symbolTable[stIndex].base, symbolTable[stIndex].offset, symbolTable[stIndex].width);
	}
}
void codeGen(Node *ptr) {
	Node *p;
	int globalSize;
	initSymbolTable();
	//1.process the declaration part
	for (p = ptr->son; p; p = p->brother) {
		if (p->token.number == DCL) processDeclaration(p->son);
		else if (p->token.number == FUNC_DEF) processFuncHeader(p->son);
		else icg_error(3);
	}
	//dumpSymbolTable();//
	globalSize = offset - 1;
	//printf("size of global variables = %d\n", globalSize);
	genSym(base);
	//2.process the function part
	for (p = ptr->son; p; p = p->brother)
		if (p->token.number == FUNC_DEF) processFunction(p);
	//if (!mainExist) warningmsg("main does not exist");
	//3.generate codes for starting routine
	// bgn	globalSize
	// ldp
	// call main
	// end
	emit1(bgn, globalSize);
	emit0(ldp);
	emitJump(call, "main");
	emit0(endop);
}
void processDeclaration(Node *ptr) {
	int typeSpecifier, typeQualifier;
	Node *p, *q;
	if (ptr->token.number != DCL_SPEC) icg_error(4);
	//printf("processDeclaration\n");
	//1. process DCL_SPEC
	typeSpecifier = INT_TYPE;
	typeQualifier = VAR_TYPE;
	p = ptr->son;
	while (p) {
		if (p->token.number == INT_NODE) typeSpecifier = INT_TYPE;
		else if (p->token.number == CONST_NODE)
			typeQualifier = CONST_TYPE;
		else { //AUTO, EXTERN, REGISTER, FLOAT, DOUBLE, SIGNED, UNSIGNED
			printf("not yet implemented\n");
			return;
		}
		p = p->brother;
	}
	//2. process DCL_ITEM
	p = ptr->brother;
	if (p->token.number != DCL_ITEM) icg_error(5);
	while (p) {
		q = p->son;	//SIMPLE_VAR OR ARRAY_VAR
		switch (q->token.number) {
		case SIMPLE_VAR:
			processSimpleVariable(q, typeSpecifier, typeQualifier);
			break;
		case ARRAY_VAR:
			processArrayVariable(q, typeSpecifier, typeQualifier);
			break;
		default:
			printf("error in SIMPLE_VAR or ARRAY_VAR\n");
			break;
		}
		p = p->brother;
	}
}
void processSimpleVariable(Node *ptr, int typeSpecifier, int typeQualifier) {
	Node *p = ptr->son;			//variable name(=> indentifier)
	Node *q = ptr->brother;		//initial value part
	int stIndex, size;
	int initialValue;
	int sign = 1;

	if (ptr->token.number != SIMPLE_VAR) printf("error in SIMPLE_VAR\n");
	if (typeQualifier == CONST_TYPE) {
		if (q == NULL) {
			printf("%s must have a constant value\n", ptr->son->token.value.id);
			return;
		}
		if (q->token.number == UNARY_MINUS) {
			sign = -1;
			q = q->son;
		}
		initialValue = sign * q->token.value.num;
		stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
			0/*base*/, 0/*offset*/, 0/*width*/, initialValue);
	}
	else {
		size = typeSize(typeSpecifier);
		stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
			base, offset, width, 0);
		offset += size;
	}
}
void processArrayVariable(Node *ptr, int typeSpecifier, int typeQaulifier) {
	Node *p = ptr->son;
	int stIndex, size;

	if (ptr->token.number != ARRAY_VAR) {
		printf("error in ARRAY_VAR\n");
		return;
	}
	if (p->brother == NULL)
		printf("array size must be specified\n");
	else size = p->brother->token.value.num;
	size *= typeSize(typeSpecifier);
	stIndex = insert(p->token.value.id, typeSpecifier, typeQaulifier,
		base, offset, size, 0);
	offset += size;
}
void processSimpleParamVariable(Node *ptr, int typeSpecifier, int typeQualifier) {
	Node *p = ptr->son;
	int stIndex, size;
	
	if (ptr->token.number != SIMPLE_VAR) printf("error in SIMPLE_VAR\n");
	size = typeSize(typeSpecifier);
	stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
		base, offset, 0, 0);
	offset += size;
}
void processArrayParamVariable(Node *ptr, int typeSpecifier, int typeQualifier) {
	Node *p = ptr->son; // variable name(=> identifier)
	int stIndex, size;
	
	if (ptr->token.number != ARRAY_VAR) { printf("error in ARRAY_VAR\n"); return; }
	size = typeSize(typeSpecifier);
	stIndex = insert(p->token.value.id, typeSpecifier, typeQualifier,
		base, offset, width, 0);
	offset += size;
}
void processParamDeclaration(Node *ptr) {
	int typeSpecifier, typeQualifier;
	Node *p, *q;

	if (ptr->token.number != DCL_SPEC) icg_error(4);
	typeSpecifier = INT_TYPE;
	typeQualifier = VAR_TYPE;
	p = ptr->son;
	while (p) {
		if (p->token.number == INT_NODE) typeSpecifier = INT_TYPE;
		else if (p->token.number == CONST_NODE) typeQualifier = CONST_TYPE;
		else {	printf("not yet implemented\n"); return;  }
		p = p->brother;
	}
	p = ptr->brother;
	switch (p->token.number) {
	case SIMPLE_VAR:
		processSimpleParamVariable(p, typeSpecifier, typeQualifier);
		break;
	case ARRAY_VAR:
		processArrayParamVariable(p, typeSpecifier, typeQualifier);
		break;
	default:
		printf("error in SIMPLE_VAR or ARRAY_VAR\n");
		break;
	}
}
void processOperator(Node* ptr) {
	switch (ptr->token.number) { // assignment operator
	case ASSIGN_OP: {
		Node* lhs = ptr->son, *rhs = ptr->son->brother;
		int stIndex;
		//1. generate instructions for left-hand side if INDEX node
		if (lhs->noderep == nonterm) {
			lvalue = 1;
			processOperator(lhs);
			lvalue = 0;
		}
		//2.generate instructions for right-hand side
		if (rhs->noderep == nonterm) processOperator(rhs);
		else rv_emit(rhs);
		//3.generate a store instruction
		if (lhs->noderep == terminal) {
			stIndex = lookup(lhs->token.value.id);
			if (stIndex == -1) {
				printf("undefined varriable : %s\n", lhs->token.value.id);
				return;
			}
			emit2(str, symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}
		else emit0(sti);
		break;
	}
	case ADD_ASSIGN: case SUB_ASSIGN: case MUL_ASSIGN: case DIV_ASSIGN: case MOD_ASSIGN:
	{	//complex assignment operators
		Node* lhs = ptr->son;
		Node* rhs = ptr->son->brother;
		int nodeNumber = ptr->token.number;
		int stIndex;
		ptr->token.number = ASSIGN_OP;
		//1. code generation for left hand side
		if (lhs->noderep == nonterm) {
			lvalue = 1;
			processOperator(lhs);
			lvalue = 0;
		}
		ptr->token.number = nodeNumber;
		//2. code generation for repeating part
		if (lhs->noderep == nonterm)
			processOperator(lhs);
		else rv_emit(lhs);
		//3. code generation for right hand side
		if (rhs->noderep == nonterm) processOperator(rhs);
		else rv_emit(rhs);
		//4. emit the corresponding operation code
		switch (ptr->token.number) {
		case ADD_ASSIGN: emit0(add); break;
		case SUB_ASSIGN: emit0(sub); break;
		case MUL_ASSIGN: emit0(mult); break;
		case DIV_ASSIGN: emit0(divop); break;
		case MOD_ASSIGN: emit0(modop); break;
		}
		//5. code generation for stor code
		if (lhs->noderep == terminal) {
			stIndex = lookup(lhs->token.value.id);
			if (stIndex == -1) {
				printf("undefined variable : %s\n", lhs->son->token.value.id);
				return;
			}
			emit2(str, symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}
		else emit0(sti);
		break;
	}
	case ADD: case SUB: case MUL: case DIV: case MOD:
	case EQ:  case NE:  case GT:  case LT:  case GE:  case LE:
	case LOGICAL_AND:   case LOGICAL_OR:
	{	//binary(arithmetic/relational/logical) operators
		Node *lhs = ptr->son, *rhs = ptr->son->brother;
		//1. visit left operand
		if (lhs->noderep == nonterm) processOperator(lhs);
		else rv_emit(lhs);
		//2.visit right operand
		if (rhs->noderep == nonterm) processOperator(rhs);
		else rv_emit(rhs);
		//3. visit root
		switch (ptr->token.number) {
		case ADD: emit0(add); break;
		case SUB: emit0(sub); break;
		case MUL: emit0(mult); break;
		case DIV: emit0(divop); break;
		case MOD: emit0(modop); break;
		case EQ: emit0(eq); break;
		case NE: emit0(ne); break;
		case GT: emit0(gt); break;
		case LT: emit0(lt); break;
		case GE: emit0(ge); break;
		case LE: emit0(le); break;
		case LOGICAL_AND:   emit0(andop); break;
		case LOGICAL_OR: emit0(orop); break;
		}
		break;
	}
	case UNARY_MINUS: case LOGICAL_NOT:
	{   //unary operators
		Node* p = ptr->son;
		if (p->noderep == nonterm) processOperator(p);
		else rv_emit(p);
		switch (ptr->token.number) {
		case UNARY_MINUS: emit0(neg); break;
		case LOGICAL_NOT: emit0(notop); break;
		}
		break;
	}
	case PRE_INC: case PRE_DEC: case POST_INC: case POST_DEC:
	{   //increment/decrement operators
		Node *p = ptr->son; Node *q;
		int stIndex; //int amount = 1;
		if (p->noderep == nonterm) processOperator(p);
		else rv_emit(p);
		q = p;
		while (q->noderep != terminal) q = q->son;
		if (!q || (q->token.number != tident)) {
			printf("increment/decrement operators can not be applied in expression\n");
			return;
		}
		stIndex = lookup(q->token.value.id);
		if (stIndex == -1) return;
		switch (ptr->token.number) {
		case PRE_INC: emit0(incop);
			// if(isOperation(ptr)) emit0(dup); 
			break;
		case PRE_DEC: emit0(decop);
			// if(isOperation(ptr)) emit0(dup); 
			break;
		case POST_INC: emit0(incop);
			// if(isOperation(ptr)) emit0(dup); 
			break;
		case POST_DEC:emit0(decop);
			// if(isOperation(ptr)) emit0(dup); 
			break;
		}
		if (p->noderep == terminal) {
			stIndex = lookup(p->token.value.id);
			if (stIndex == -1) return;
			emit2(str, symbolTable[stIndex].base, symbolTable[stIndex].offset);
		}
		else if (p->token.number == INDEX) {
			lvalue = 1;
			processOperator(p);
			lvalue = 0;
			emit0(swp);
			emit0(sti);
		}
		else printf("error in increment/decrement operators\n");
		break;
	}
	case INDEX:
	{
		Node* indexExp = ptr->son->brother;
		int stIndex;
		if (indexExp->noderep == nonterm) processOperator(indexExp);
		else rv_emit(indexExp);
		stIndex = lookup(ptr->son->token.value.id);
		if (stIndex == -1) {
			printf("undefined variable : %s\n", ptr->son->token.value.id);
			return;
		}
		emit2(lda, symbolTable[stIndex].base, symbolTable[stIndex].offset);
		emit0(add);
		if (!lvalue) emit0(ldi);
		break;
	}
	case CALL:
	{
		Node* p = ptr->son;
		char *functionName;
		int stIndex; int noArguments;
		if (checkPredefined(p)) break;
		//handle for user func
		functionName = p->token.value.id;
		stIndex = lookup(functionName);
		if (stIndex == -1) break;
		noArguments = symbolTable[stIndex].width;
		emit0(ldp);
		p = p->brother;
		while (p) {
			if (p->noderep == nonterm) processOperator(p);
			else rv_emit(p);
			noArguments--;
			p = p->brother;
		}
		if (noArguments > 0) printf("%s: too few actual arguments", functionName);
		if (noArguments < 0) printf("%s: too many actual arguments", functionName);
		emitJump(call, ptr->son->token.value.id);
		break;
	}
	}
}
void processStatement(Node *ptr) {
	Node *p = NULL;
	int returnWithValue;
	switch (ptr->token.number) {
	case COMPOUND_ST:
		p = ptr->son->brother;
		p = p->son;
		while (p) {
			processStatement(p);
			p = p->brother;
		}
		break;
	case EXP_ST:
		if (ptr->son != NULL) processOperator(ptr->son);
		break;
	case RETURN_ST:
		if (ptr->son != NULL) {
			returnWithValue = 1;
			p = ptr->son;
			if (p->noderep == nonterm) processOperator(p);
			else rv_emit(p);
			emit0(retv);
		}
		else  emit0(retv);
		break;
	case IF_ST: {
		char label[LABEL_SIZE];
		genLabel(label);
		processCondition(ptr->son);
		emitJump(fjp, label);
		processStatement(ptr->son->brother);
		emitLabel(label);
	}break;
	case IF_ELSE_ST:
	{
		char label1[LABEL_SIZE], label2[LABEL_SIZE];
		genLabel(label1); genLabel(label2);
		processCondition(ptr->son);
		emitJump(fjp, label1);
		processStatement(ptr->son->brother);
		emitJump(ujp, label2);
		emitLabel(label1);
		processStatement(ptr->son->brother->brother);
		emitLabel(label2);
	}break;
	case WHILE_ST:
	{
		char label1[LABEL_SIZE], label2[LABEL_SIZE];
		genLabel(label1); genLabel(label2);
		emitLabel(label1);
		processCondition(ptr->son);
		emitJump(fjp, label2);
		processStatement(ptr->son->brother);
		emitJump(ujp, label1);
		emitLabel(label2);
	}break;
	default:
		printf("not yet implemented.\n");
		break;
	}
}
void processCondition(Node* ptr) {
	if (ptr->noderep == nonterm) processOperator(ptr);
	else rv_emit(ptr);
}
void processFunction(Node *ptr)
{
	Node *p, *q;
	int sizeOfVar = 0;
	int numOfVar = 0;
	int stIndex;
	base++;
	offset = 1;
	if (ptr->token.number != FUNC_DEF) icg_error(4);
	// step 1: process formal parameters
	p = ptr->son->son->brother->brother;
	p = p->son;
	while (p) {
		if (p->token.number == PARAM_DCL) {
			processParamDeclaration(p->son);
			sizeOfVar++;
			numOfVar++;
		}
		p = p->brother;
	}
	// step 2: process the declaration part in function body
	p = ptr->son->brother->son->son;
	while (p) {
		if (p->token.number == DCL) {
			processDeclaration(p->son);
			q = p->son->brother;
			while (q) {
				if (q->token.number == DCL_ITEM) {
					if (q->son->token.number == ARRAY_VAR) {
						sizeOfVar += q->son->son->brother->token.value.num;
					}
					else sizeOfVar += 1;
					numOfVar++;
				}
				q = q->brother;
			}
		}
		p = p->brother;
	}
	// step 3: emit the function start code
	p = ptr->son->son->brother;	// IDENT
	emitFunc(p->token.value.id, sizeOfVar, base, 2);
	for (stIndex = symbolTableTop - numOfVar; stIndex < symbolTableTop; stIndex++)
		emitSym(symbolTable[stIndex].base, symbolTable[stIndex].offset, symbolTable[stIndex].width);
	// step 4: process the statement part in function body
	p = ptr->son->brother;	// COMPOUND_ST
	processStatement(p);
	// step 5: check if return type and return value
	p = ptr->son->son;	// DCL_SPEC
	if (p->token.number == DCL_SPEC) {
		p = p->son;
		if (p->token.number == VOID_NODE)	emit0(ret);
		else if (p->token.number == CONST_NODE) {
			if (p->brother->token.number == VOID_NODE)	emit0(ret);
		}
	}
	// step 6: generate the ending codes
	emit0(endop);
	base--;
	symbolTable->nextIndex++;
}
void processFuncHeader(Node *ptr) {
	int noArguments, returnType;
	int stIndex;
	Node* p;

	//printf("processFuncHeader\n");
	if (ptr->token.number != FUNC_HEAD)
		printf("error in processFuncHeader\n");
	//1.process the function return type
	p = ptr->son->son;
	while (p) {
		if (p->token.number == INT_NODE) returnType = INT_TYPE;
		else if (p->token.number == VOID_NODE) returnType = VOID_TYPE;
		else printf("invalid function return type\n");
		p = p->brother;
	}
	//2. count the number of formal parameters
	p = ptr->son->brother->brother;
	p = p->son;
	noArguments = 0;
	while (p) {
		noArguments++;
		p = p->brother;
	}
	//3.insert the function name
	stIndex = insert(ptr->son->brother->token.value.id, returnType, FUNC_TYPE,
		1/*base*/, 0/*width*/, noArguments/*width*/, 0/*initialValue*/);
	//if(!strcmp("main",functionName)) mainExist=1;
}
void rv_emit(Node* ptr) {
	int stIndex;

	if (ptr->token.number == tnumber)
		emit1(ldc, ptr->token.value.num);
	else {
		stIndex = lookup(ptr->token.value.id);
		if (stIndex == -1) return;
		if (symbolTable[stIndex].typeQualifier == CONST_TYPE)
			emit1(ldc, symbolTable[stIndex].initialValue);
		else if (symbolTable[stIndex].width > 1)
			emit2(lda, symbolTable[stIndex].base, symbolTable[stIndex].offset);
		else
			emit2(lod, symbolTable[stIndex].base, symbolTable[stIndex].offset);
	}
}
void emitSym(int operand1, int operand2, int operand3) {
	fprintf(ucodeFile, "           sym  %d  %d  %d\n", operand1, operand2, operand3);
	printf("           sym  %d  %d  %d\n", operand1, operand2, operand3);
}
void emitJump(int opcode, char *label) {
	fprintf(ucodeFile, "           %s  %s\n", opcodeName[opcode], label);
	printf("           %s  %s\n", opcodeName[opcode], label);
}
void emit0(int opcode) {
	fprintf(ucodeFile, "           %s\n", opcodeName[opcode]);
	printf("           %s\n", opcodeName[opcode]);
}
void emit1(int opcode, int operand) {
	fprintf(ucodeFile, "           %s  %d\n", opcodeName[opcode], operand);
	printf("           %s  %d\n", opcodeName[opcode], operand);
}
void emit2(int opcode, int operand1, int operand2) {
	fprintf(ucodeFile, "           %s  %d  %d\n", opcodeName[opcode], operand1, operand2);
	printf("           %s  %d  %d\n", opcodeName[opcode], operand1, operand2);
}
void emitFunc(char *value, int p, int q, int r) {
	int label;
	label = strlen(value);
	fprintf(ucodeFile, "%s", value);
	printf("%s", value);
	for (; label < LABEL_SIZE - 1; label++) {
		fprintf(ucodeFile, " ");
		printf(" ");
	}
	fprintf(ucodeFile, "fun  %d  %d  %d\n", p, q, r);
	printf("fun  %d  %d  %d\n", p, q, r);
}
void genLabel(char* label) {
	static int labelNum = 0;
	sprintf(label, "$$%d", labelNum++);
}
void emitLabel(char* label) {
	int length;
	length = strlen(label);
	fprintf(ucodeFile, "%s", label);
	printf("%s", label);
	for (; length < LABEL_SIZE - 1; length++) {
		fprintf(ucodeFile, " ");
		printf(" ");
	}
	fprintf(ucodeFile, "nop\n");
	printf("nop\n");
}
void icg_error(int err) {
	printf("error %d\n", err);
}
int checkPredefined(Node *ptr)
{
	Node *p = NULL;
	if (strcmp(ptr->token.value.id, "read") == 0) {
		emit0(ldp);
		p = ptr->brother;
		while (p) {
			if (p->noderep == nonterm) processOperator(p);
			else rv_emit(p);
			p = p->brother;
		}
		emitJump(call, "read");
		return 1;
	}
	else if (strcmp(ptr->token.value.id, "write") == 0) {
		emit0(ldp);
		p = ptr->brother;
		while (p) {
			if (p->noderep == nonterm) processOperator(p);
			else rv_emit(p);
			p = p->brother;
		}
		emitJump(call, "write");
		return 1;
	}
	else if (strcmp(ptr->token.value.id, "lf") == 0) {
		emitJump(call, "lf");
		return 1;
	}
	return 0;
}
int typeSize(int typeSpecifier)
{
	if (typeSpecifier == INT_TYPE)
		return 1;
	else {
		printf("not yet implemented\n");
		return 1;
	}
}
int main(int argc, char* argv[]) {
	char fileName[30];
	Node *root;

	printf(" *** start of Mini C Compiler\n");
	if (argc != 2) {
		icg_error(1);
		exit(1);
	}
	strcpy(fileName, argv[1]);
	printf(" *source file name: %s\n", fileName);
	freopen(fileName, "r", stdin);
	if ((sourceFile = fopen(fileName, "r")) == NULL) {
		icg_error(2);
		exit(1);
	}
	astFile = fopen(strcat(strtok(fileName, "."), ".ast"), "w");
	ucodeFile = fopen(strcat(strtok(fileName, "."), ".uco"), "w");

	printf(" === start of Parser\n");
	root = parser();
	printTree(root, 0);
	printf(" === start of ICG\n");
	codeGen(root);
	printf(" *** end   of Mini C Compiler\n");

	fclose(sourceFile);
	fclose(astFile);
	fclose(ucodeFile);
	return 0;
}
