/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                           **
;**                NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/


#include "decumaQsort.h"

#if !defined(DECUMA_USE_STDLIB_QSORT)

/* ----------------------------------------------------------------------------
 * From:    Sorting and Searching Algorithms: A Cookbook
 * Author:  Thomas Niemann. 
 * Web:     http://www.cs.auckland.ac.nz/software/AlgAnim/niemann/s_qsort.txt
 *          http://www.cs.auckland.ac.nz/software/AlgAnim/niemann/s_title.htm
 * License: "Source code, when part of a software project, may be used freely 
 *          without reference to the author."
 * ---------------------------------------------------------------------------- */

#include <limits.h> /* Definition of CHAR_BIT */
#define MAXSTACK (sizeof(unsigned long) * CHAR_BIT)

static void Exchange(void *a, void *b, unsigned long size) {
    unsigned long i;
	
	int  * ia = (int *)a;
	int  * ib = (int *)b;
	
	char * ca;
	char * cb;

    /******************
     *  exchange a,b  *
     ******************/

	for (i = sizeof(int); i <= size; i += sizeof(int)) {
        int t = *ia;
        *ia++ = *ib;
        *ib++ = t;
    }

	ca = (char *)ia;
	cb = (char *)ib;

	for (i = i - sizeof(int) + 1; i <= size; i++) {
        char t = *ca;
        *ca++  = *cb;
        *cb++  = t;
    }

}

void decumaQsort(void *base, unsigned long nmemb, unsigned long size,
        int (*compar)(const void *, const void *)) {
    void *LbStack[MAXSTACK], *UbStack[MAXSTACK];
    int sp;
    unsigned int Offset;

    /********************
     *  ANSI-C qsort()  *
     ********************/

    LbStack[0] = (char *)base;
    UbStack[0] = (char *)base + (nmemb-1)*size;
    for (sp = 0; sp >= 0; sp--) {
        char *Lb, *Ub, *M;
        char *P, *I, *J;

        Lb = (char*)LbStack[sp];
        Ub = (char*)UbStack[sp];

        while (Lb < Ub) {

            /* select pivot and exchange with 1st element */
            Offset = (Ub - Lb) >> 1;
            P = Lb + Offset - Offset % size;
            Exchange (Lb, P, size);

            /* partition into two segments */
            I = Lb + size;
            J = Ub;
            for(;;)
			{
                while (I < J && compar(Lb, I) > 0) I += size;
                while (J >= I && compar(J, Lb) > 0) J -= size;
                if (I >= J) break;
                Exchange (I, J, size);
                J -= size;
                I += size;
            }

            /* pivot belongs in A[j] */
            Exchange (Lb, J, size);
            M = J;

            /* keep processing smallest segment, and stack largest */
            if (M - Lb <= Ub - M) {
                if (M + size < Ub) {
                    LbStack[sp] = M + size;
                    UbStack[sp++] = Ub;
                }
                Ub = M - size;
            } else {
                if (M - size > Lb) {
                    LbStack[sp] = Lb;
                    UbStack[sp++] = M - size;
                }
                Lb = M + size;
            }
        }
    }
}

#endif /* !DECUMA_USE_STDLIB_QSORT) */
