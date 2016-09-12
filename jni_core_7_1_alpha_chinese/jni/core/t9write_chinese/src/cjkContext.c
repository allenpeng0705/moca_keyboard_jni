#include "cjkContext.h"

/*  */
/* This context struct should be initialized. */
/*  */
void cjkContextInit(CJK_CONTEXT * const con) 
{
	con->previous_unichar = 0;
	con->double_quotation_counter = 0;
	con->single_quotation_counter = 0;
}

CJK_UNICHAR cjkContextGetPrevious(CJK_CONTEXT const * const con) 
{
	return con->previous_unichar;
}

void cjkContextSetPrevious(CJK_CONTEXT * const con, CJK_UNICHAR prev_uc) 
{
	con->previous_unichar = prev_uc;
}

DECUMA_INT32 cjkContextGetSingleQuoteCount(CJK_CONTEXT const * const con) 
{
	return con->single_quotation_counter;
}

void cjkContextSetSingleQuoteCount(CJK_CONTEXT * const con) 
{
	con->single_quotation_counter = 1;
}

void cjkContextIncSingleQuoteCount(CJK_CONTEXT * const con) 
{
	con->single_quotation_counter++;
}

DECUMA_INT32 cjkContextGetDoubleQuoteCount(CJK_CONTEXT const * const con) 
{
	return con->double_quotation_counter;
}

void cjkContextSetDoubleQuoteCount(CJK_CONTEXT * const con) 
{
	con->double_quotation_counter = 1;
}

void cjkContextIncDoubleQuoteCount(CJK_CONTEXT * const con) 
{
	con->double_quotation_counter++;
}


/*  */
/* There are some functions that resets the members in the struct. */
/*  */
void cjkContextResetPrevious(CJK_CONTEXT * const con) 
{
	con->previous_unichar = 0;
}

void cjkContextResetSingleQuoteCount(CJK_CONTEXT * const con) 
{
	con->single_quotation_counter = 0;
}

void cjkContextResetDoubleQuoteCount(CJK_CONTEXT * const con) 
{
	con->double_quotation_counter = 0;
}

CJK_BOOLEAN cjkContextHasPrevious(CJK_CONTEXT * const con) 
{
	return !(con->previous_unichar == 0x0000);
}
