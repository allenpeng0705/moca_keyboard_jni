/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9misc.c                                                   **
;**                                                                           **
;**  Description: miscellaneous tools for ET9                                 **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9misc Miscellaneous Tools for XT9
* Miscellaneous tools for generic XT9.
* @{
*/

#include "et9api.h"
#include "et9sym.h"
#include "et9misc.h"


#ifndef ET9_DEACTIVATE_MATHLIB_USE
#include <math.h>
#endif

#ifdef _DEBUG
#ifndef ET9_DEBUG
#ifdef _WIN32
#pragma message ("*** WARNING - _DEBUG defined but not ET9_DEBUG ***")
#endif
#endif
#endif

#ifdef ET9_DEBUG
#ifdef _WIN32
#pragma message ("*** ET9_DEBUG (et9 asserts etc) activated ***")
#endif
#endif

#ifdef ET9ACTIVATEMISCSTDCLIBUSE
#ifdef _WIN32
#pragma message ("*** USING STD-C LIB SUPPORT FOR memcpy etc ***")
#endif
#else /* ET9ACTIVATEMISCSTDCLIBUSE */
#ifdef _WIN32
#pragma message ("*** USING INTERNAL memcpy etc SUPPORT ***")
#endif
#endif /* ET9ACTIVATEMISCSTDCLIBUSE */

#ifdef ET9_DEACTIVATE_MATHLIB_USE
#ifdef _WIN32
#pragma message ("*** No math lib use ***")
#endif
#endif

#ifndef ET9PTRDIFF
#ifdef _WIN32
#include <stddef.h>
#endif
#ifdef _PTRDIFF_T_DEFINED
#define ET9PTRDIFF ptrdiff_t
#else
#define ET9PTRDIFF ET9U32
#endif
#endif


#ifndef ET9ACTIVATEMISCSTDCLIBUSE

/*---------------------------------------------------------------------------*/
/** \internal
 * Byte copy.
 * This function will NOT assert on size zero.
 *
 * @param des            To copy to.
 * @param src            To copy from.
 * @param size           Size to copy.
 *
 * @return None
 */

void ET9FARCALL _ET9ByteCopy(ET9U8          *des,
                             ET9U8  const * src,
                             ET9U32         size)
{
    ET9Assert(des);
    ET9Assert(src);

    while (size--) {
        *des++ = *src++;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Sym copy.
 * This function will NOT assert on size zero.
 *
 * @param des            To copy to.
 * @param src            To copy from.
 * @param size           Size to copy.
 *
 * @return None
 */

void ET9FARCALL _ET9SymCopy(ET9SYMB *des,
                            ET9SYMB const *src,
                            ET9U32 size)
{
    ET9Assert(des);
    ET9Assert(src);

    while (size--) {
        *des++ = *src++;
    }
}

#if 0
/*---------------------------------------------------------------------------*/
/** \internal
 * Byte move.
 * Move src to des (can be overlapping).
 * This function will NOT assert on size zero.
 *
 * @param dst            To move to.
 * @param src            To move from.
 * @param size           Size to move.
 *
 * @return None
 */

void ET9FARCALL _ET9ByteMove(ET9U8 *dst,
                             ET9U8 const *src,
                             ET9U32 size)
{
    ET9U8 *svdst;

    ET9Assert(dst);
    ET9Assert(src);

    if (!size) {
        return;
    }

    if ((dst > src) && (dst < src + size)) {
        src += size;
        for (svdst = dst + size; size-- > 0; ) {
            *--svdst = *--src;
        }
    }
    else {
        for (svdst = dst; size-- > 0; ) {
            *svdst++ = *src++;
        }
    }
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Sym move.
 * Move src to des (can be overlapping).
 * This function will NOT assert on size zero.
 *
 * @param dst            To move to.
 * @param src            To move from.
 * @param size           Size to move.
 *
 * @return None
 */

void ET9FARCALL _ET9SymMove(ET9SYMB *dst,
                            ET9SYMB const *src,
                            ET9U32 size)
{
    ET9SYMB *svdst;

    ET9Assert(dst);
    ET9Assert(src);

    if ((dst > src) && (dst < src + size)) {
        src += size;
        for (svdst = dst + size; size-- > 0; ) {
            *--svdst = *--src;
        }
    }
    else {
        for (svdst = dst; size-- > 0; ) {
            *svdst++ = *src++;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Clear memory.
 * Clear given mem space.
 * This function will NOT assert on size zero.
 *
 * @param des            Where to clear.
 * @param size           Size to clear.
 *
 * @return None
 */

void ET9FARCALL _ET9ClearMem(ET9U8 *des,
                             ET9U32 size)
{
    ET9U32 count4;
    ET9U32 count1;
    ET9U32 *des4;

    ET9Assert(des);

    count1 = (ET9U32)((ET9PTRDIFF)des & 0x03);
    if (count1) {
        count1 = 4 - count1;
        while (count1-- && size) {
            *des++ = (ET9U8)0;
            --size;
        }
    }
    if (size) {
        count4 = (ET9U32)(size >> 2);
        count1 = (ET9U32)(size & (ET9U32)0x03);

        des4 = (ET9U32*)des;
        while (count4--) {
            *des4++ = 0;
        }
        des = (ET9U8*)des4;
        while (count1--) {
            *des++ = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Byte set.
 * Set des buffer with a byte value.
 * This function will NOT assert on size zero.
 *
 * @param des            Where to set.
 * @param size           Size to set.
 * @param s              Value to set.
 *
 * @return None
 */

void ET9FARCALL _ET9ByteSet(ET9U8 *des,
                            ET9U32 size,
                            ET9U8 s)
{
    ET9Assert(des);

    while (size--) {
        *des++ = s;
    }
}

#endif /* ET9ACTIVATEMISCSTDCLIBUSE */

#ifdef ET9_NAV_ALPHABETIC_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * Word set - obsolete - avoid using it.
 * Set des buffer with a word value.
 * This function will NOT assert on size zero.
 *
 * @param des            Where to set.
 * @param size           Size to set.
 * @param s              Value to set.
 *
 * @return None
 */

void ET9FARCALL _ET9WordSet(ET9U16 *des,
                            ET9U32 size,
                            ET9U16 s)
{
    ET9Assert(des);

    while (size--) {
        *des++ = s;
    }
}

#endif /*ET9_NAV_ALPHABETIC_MODULE*/

#ifdef ET9_DEACTIVATE_MATHLIB_USE

/*---------------------------------------------------------------------------*/

#define _ET9_MATH_ACOS_TABLESIZE 101

static const ET9FLOAT __pfAcos[] =
{
    3.14159265358979310f, 2.94125781126667360f, 2.85779854438146510f, 2.79342663231683240f, 2.73887681200913180f, 2.69056584179353080f,
    2.64665852724889780f, 2.60606599927540560f, 2.56807954916669660f, 2.53220734555899840f, 2.49809154479650890f, 2.46546214402913180f, 2.43410944181045030f, 2.40386668513654420f,
    2.37459864572792640f, 2.34619382340564940f, 2.31855896145481700f, 2.29161508766498610f, 2.26529459242145270f, 2.23953902999726840f, 2.21429743558818080f, 2.18952501746714790f,
    2.16518212679595880f, 2.14123343619481870f, 2.11764727749084080f, 2.09439510239319570f, 2.07145103919948470f, 2.04879152531384890f, 2.02639500019071980f, 2.00424164686478260f,
    1.98231317286238460f, 1.96059262326915730f, 1.93906422023153670f, 1.91771322432205830f, 1.89652581408952670f, 1.87548898081029410f, 1.85459043600322460f, 1.83381852970336560f,
    1.81316217783385980f, 1.79261079729169090f, 1.77215424758522740f, 1.75178277804144430f, 1.73148697974680730f, 1.71125774150475250f, 1.69108620918968480f, 1.67096374795645630f,
    1.65088190682855560f, 1.63083238524017520f, 1.61080700114888550f, 1.59079766036828720f, 1.57079632679489660f, 1.55079499322150620f, 1.53078565244090760f, 1.51076026834961820f,
    1.49071074676123750f, 1.47062890563333680f, 1.45050644440010830f, 1.43033491208504080f, 1.41010567384298600f, 1.38980987554834900f, 1.36943840600456590f, 1.34898185629810220f,
    1.32843047575593330f, 1.30777412388642780f, 1.28700221758656870f, 1.26610367277949900f, 1.24506683950026640f, 1.22387942926773490f, 1.20252843335825640f, 1.18100003032063630f,
    1.15927948072740870f, 1.13735100672501080f, 1.11519765339907330f, 1.09280112827594420f, 1.07014161439030840f, 1.04719755119659790f, 1.02394537609895250f, 1.00035921739497470f,
    0.97641052679383433f, 0.95206763612264544f, 0.92729521800161208f, 0.90205362359252472f, 0.87629806116834075f, 0.84997756592480711f, 0.82303369213497612f, 0.79539883018414359f,
    0.76699400786186667f, 0.73772596845324878f, 0.70748321177934292f, 0.67613050956066134f, 0.64350110879328426f, 0.60938530803079494f, 0.57351310442309655f, 0.53552665431438762f,
    0.49493412634089573f, 0.45102681179626264f, 0.40271584158066176f, 0.34816602127296103f, 0.28379410920832798f, 0.20033484232311968f, 0.00000000000000000f
};

/*---------------------------------------------------------------------------*/
/** \internal
 * Approximate the arccosine of a float value. The error is <2% in the middle, but gets very bad approaching -1 or 1.
 *
 * @param fX             Value between –1 and 1 whose arccosine is to be calculated.
 *
 * @return Returns the arccosine of fX in the range 0 to pi radians.
 */

ET9FLOAT ET9FARCALL _ET9acos_f_approximate(const ET9FLOAT fX)
{
    ET9Assert(-1.0f <= fX && fX <= 1.0f);

    return __pfAcos[ (ET9UINT)((1 + fX) / 2 * (_ET9_MATH_ACOS_TABLESIZE - 1)) ];
}


#define _ET9_MATH_Atan_TABLE_SIZE   50
#define _ET9_MATH_Atan_TABLE_BOUND  10.000000f
#define _ET9_MATH_Atan_STEP_SIZE    0.200000f
#define _ET9_MATH_Pi                3.14159265358979323846f

const static ET9FLOAT afAtan[] =
{
    0.00000000000000000f, 0.98697779924940388f, 0.19739555984988078f, 0.91555408631242063f, 0.38050637711236490f, 0.79956561579109686f, 0.54041950027058427f, 0.67160720976484234f,
    0.67474094222355274f, 0.55328610586947768f, 0.78539816339744828f, 0.45329943600372569f, 0.87605805059819353f, 0.37244395106940831f, 0.95054684081207519f, 0.30825085319629464f,
    1.01219701145133410f, 0.25750405475612803f, 1.06369782240255970f, 0.21725447695765343f, 1.10714871779409040f, 0.18510057936965030f, 1.14416883366802050f, 0.15918186713557381f,
    1.17600520709513520f, 0.13808642940771110f, 1.20362249297667740f, 0.12074946698757860f, 1.22777238637419320f, 0.10636693012030629f, 1.24904577239825440f, 0.09432843010835357f,
    1.26791145841992510f, 0.08416713328826653f, 1.28474488507757840f, 0.07552295689448818f, 1.29984947645647610f, 0.06811567683665976f, 1.31347261182380800f, 0.06172525922112260f,
    1.32581766366803260f, 0.05617741128981280f, 1.33705314592599510f, 0.05133289864134305f, 1.34731972565426370f, 0.04707958788405686f, 1.35673564323107510f, 0.04332647187027083f,
    1.36540093760512930f, 0.03999914669943316f, 1.37340076694501590f, 0.03703635965582541f, 1.38080803887618100f, 0.03438735328115783f, 1.38768550953241250f, 0.03200980596223757f,
    1.39408747072486010f, 0.02986822297376901f, 1.40006111531961390f, 0.02793267030328006f, 1.40564764938026990f, 0.02617777128203769f, 1.41088320363667740f, 0.02458190617139144f,
    1.41579958487095570f, 0.02312656958403214f, 1.42042489878776210f, 0.02179585147929597f, 1.42478406908362130f, 0.02057601553555721f, 1.42889927219073280f, 0.01945515473322224f,
    1.43279030313737720f, 0.01842290852275497f, 1.43647488484192820f, 0.01747022939455722f, 1.43996893072083960f, 0.01658918929409392f, 1.44328676857965840f, 0.01577281834238331f,
    1.44644133224813510f, 0.01501496987998952f, 1.44944432622413300f, 0.01431020706312958f, 1.45230636763675890f, 0.01365370718663495f, 1.45503710907408590f, 0.01304118065163062f,
    1.45764534520441200f, 0.01246880208294443f, 1.46013910562100090f, 0.01193315156719832f, 1.46252573593444060f, 0.01143116435428082f, 1.46481196880529670f, 0.01096008766278467f,
    1.46700398633785390f, 0.01051744346982853f, 1.46910747503181960f, 0.01010099635957551f
};

/*---------------------------------------------------------------------------*/
/** \internal
 * Approximate the arctangent of a float value.
 *
 * @param fX             Value whose arctangent is to be calculated.
 *
 * @return Returns the arctangent of fX in the range 0 to pi radians.
 */

ET9FLOAT ET9FARCALL _ET9atan_f_approximate(const ET9FLOAT fX)
{
    ET9INT      i;
    ET9FLOAT    fY0;
    ET9FLOAT    fDYDX0;
    ET9FLOAT    fDX;

    if (fX < 0.0f) {
        return -_ET9atan_f_approximate(-fX);
    }

    i = __ET9Min((ET9INT)(fX / _ET9_MATH_Atan_STEP_SIZE), _ET9_MATH_Atan_TABLE_SIZE - 1);

    ET9Assert(0 <= i);

    fDX = fX - i * _ET9_MATH_Atan_STEP_SIZE;
    fY0 = afAtan[2*i];
    fDYDX0 = afAtan[2*i + 1];

    return __ET9Min(fY0 + fDYDX0 * fDX, _ET9_MATH_Pi / 2);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Approximate the arctan2 of float values.
 *
 * @param fY             Any value.
 * @param fX             Any value.
 *
 * @return Returns the arctangent of fY/fX in the range -pi to pi radians.
 */

ET9FLOAT ET9FARCALL _ET9atan2_f_approximate(const ET9FLOAT fY, const ET9FLOAT fX)
{
    ET9FLOAT fTmp;

    if (0.0f == fX) {

        if (0.0f < fY) {
            return _ET9_MATH_Pi / 2;
        }
        else if (0.0f > fY) {
            return - _ET9_MATH_Pi / 2;
        }
        else {
            return 0.0f;
        }
    }
    else {

        fTmp = _ET9atan_f_approximate(fY / fX);

        if (0.0f < fX) {
            return fTmp;
        }
        else if (0.0f <= fY) {
            return fTmp + _ET9_MATH_Pi;
        }
        else {
            return fTmp - _ET9_MATH_Pi;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Approximate the square root of a float value. This implements fast inverse square root, then inverts the result.
 *
 * @param fX             Nonnegative floating-point value.
 *
 * @return Returns the square-root of fX.
 */

ET9FLOAT ET9FARCALL _ET9sqrt_f_approximate(const ET9FLOAT fX)
{
    union {
        ET9INT i;
        ET9FLOAT f;
    } u;

    if (fX <= 0.0f) {
        return fX;
    }

    u.f = fX;

    /* magically approximate inv square root */

    u.i = 0x5f375a86 - (u.i >> 1);

    /* do one iteration of Newton's method and invert */

    return 1.0f / (u.f * (1.5f - 0.5f * fX * u.f * u.f));
}

/*---------------------------------------------------------------------------*/

#ifndef ET9FLOAT_SIG_BITS
#define ET9FLOAT_SIG_BITS 23
#endif

#ifndef ET9FLOAT_EXP_BIAS
#define ET9FLOAT_EXP_BIAS 127
#endif

#define _ET9_MATH_POW_MAGICNUM 100
#define _ET9_MATH_POW_MAGIC_TABLESIZE 20

static const ET9FLOAT __pfMagicPower[] =
{
    1.0f,
    100.0f,
    10000.0f,
    1000000.0f,
    100000000.0f,
    10000000000.0f,
    1000000000000.0f,
    100000000000000.0f,
    10000000000000000.0f,
    1000000000000000000.0f,
    100000000000000000000.0f,
    10000000000000000000000.0f,
    999999999999999980000000.0f,
    100000000000000000000000000.0f,
    9999999999999999600000000000.0f,
    1000000000000000000000000000000.0f,
    100000000000000010000000000000000.0f,
    9999999999999999500000000000000000.0f,
    1000000000000000000000000000000000000.0f,
    99999999999999998000000000000000000000.0f
};

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates x raised to the power of (int)y. Y must be >= 0.
 * For fX == 2.0f : returns 2^(int)fY quickly
 * For fX == _ET9_MATH_POW_MAGICNUM, fY < _ET9_MATH_POW_MAGIC_TABLESIZE: returns correct value quickly
 * For fX == _ET9_MATH_POW_MAGICNUM, fY >= _ET9_MATH_POW_MAGIC_TABLESIZE: returns incorrectly _ET9_MATH_POW_MAGICNUM ^ (_ET9_MATH_POW_MAGIC_TABLESIZE - 1)
 * For other values, will multiply loop, ignoring fY's fractional part.
 *
 * @param fX             Base
 * @param fY             Exponent
 *
 * @return Returns the value of x^y.
 */

ET9FLOAT ET9FARCALL _ET9pow_f_approximate(const ET9FLOAT fX, const ET9FLOAT fY)
{
    ET9Assert((ET9INT)fY >= 0);

    if (fY < 0.0f) {
        /* return 1.0f/_ET9pow_f_approximate(fX, -fY); */
        return 0.0f;
    }

    if (2.0f == fX) {

        union {
            ET9UINT n;
            ET9FLOAT f;
        } u;

        u.n = ((ET9UINT)fY + ET9FLOAT_EXP_BIAS) << ET9FLOAT_SIG_BITS;

        return u.f;
    }
    else if ((ET9FLOAT)_ET9_MATH_POW_MAGICNUM == fX) {

        return __pfMagicPower[__ET9Min((ET9UINT)fY, _ET9_MATH_POW_MAGIC_TABLESIZE-1)];
    }
    else {

        ET9UINT n = (ET9UINT)fY;
        ET9FLOAT fRet = 1.0f;

        while (n--) {
            fRet *= fX;
        }

        return fRet;
    }
}

/*---------------------------------------------------------------------------*/

#define _ET9_MATH_Log_TABLE_INDEX_BITS      5
#ifndef ET9_CUSTOM_FLOAT
#define _ET9_MATH_Log_TABLE_INDEX_MASK      0x007C0000
#define _ET9_MATH_Log_TABLE_NON_INDEX_MASK  0x0003FFFF
#endif
#define _ET9_MATH_Log_2_TO_E_LOG_RATIO      0.69314718055994529f

/* lg(x) for x in (1,2) */

static const ET9FLOAT __afLog[] =
{
    0.00000000000000000f, 0.04439411935845344f, 0.08746284125033940f, 0.12928301694496647f, 0.16992500144231237f, 0.20945336562894978f, 0.24792751344358552f, 0.28540221886224831f,
    0.32192809488736235f, 0.35755200461808373f, 0.39231742277876031f, 0.42626475470209796f, 0.45943161863729726f, 0.49185309632967472f, 0.52356195605701294f, 0.55458885167763738f,
    0.58496250072115619f, 0.61470984411520824f, 0.64385618977472470f, 0.67242534197149562f, 0.70043971814109218f, 0.72792045456319920f, 0.75488750216346856f, 0.78135971352465972f,
    0.80735492205760406f, 0.83289001416474173f, 0.85798099512757209f, 0.88264304936184135f, 0.90689059560851848f, 0.93073733756288635f, 0.95419631038687525f, 0.97727992349991655f
};

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates natural log of x. Error is < .01
 *
 * @param fX             X
 *
 * @return Returns the value of ln(x).
 */

ET9FLOAT ET9FARCALL _ET9log_f_approximate(const ET9FLOAT fX)
{
    ET9U32 nX = *((ET9U32 *)&fX); /* x cast to int to work with bits */
    ET9INT snExponent;            /* the exponent from the float */
    ET9U32 nIndex;                /* index into table from high bits of mantissa */
    ET9FLOAT fChange;             /* fraction representing remaining bits of mantissa */

    ET9Assert(fX > 0.0f);

    snExponent = (ET9INT)((nX >> ET9FLOAT_SIG_BITS) - ET9FLOAT_EXP_BIAS);
    nIndex = ((nX & _ET9_MATH_Log_TABLE_INDEX_MASK) >> (ET9FLOAT_SIG_BITS - _ET9_MATH_Log_TABLE_INDEX_BITS));

    ET9Assert(nIndex < sizeof(__afLog)/sizeof(*__afLog));

    /* assume lg(x) ~= x - 1 for x in (1,2) */

    fChange = (ET9FLOAT)(_ET9_MATH_Log_TABLE_NON_INDEX_MASK & nX) / (1 << (ET9FLOAT_SIG_BITS));

    return _ET9_MATH_Log_2_TO_E_LOG_RATIO * (snExponent + __afLog[nIndex] + fChange);
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate the arccosine of a float value.
 *
 * @param fX             Value between –1 and 1 whose arccosine is to be calculated.
 *
 * @return Returns the arccosine of fX in the range 0 to pi radians.
 */

ET9FLOAT ET9FARCALL _ET9acos_f(const ET9FLOAT fX)
{
#ifdef ET9_DEACTIVATE_MATHLIB_USE
    return (ET9FLOAT)_ET9acos_f_approximate(fX);
#else
    return (ET9FLOAT)acos(fX);
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate the arccosine of a float value.
 *
 * @param fY             Any value.
 * @param fX             Any value.
 *
 * @return Returns the arctangent of fY/fX in the range -pi to pi radians.
 */

ET9FLOAT ET9FARCALL _ET9atan2_f(const ET9FLOAT fY, const ET9FLOAT fX)
{
#ifdef ET9_DEACTIVATE_MATHLIB_USE
    return (ET9FLOAT)_ET9atan2_f_approximate(fY, fX);
#else
    return (ET9FLOAT)atan2(fY, fX);
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates the square root of a float value.
 *
 * @param fX             Nonnegative floating-point value.
 *
 * @return Returns the square-root of fX.
 */

ET9FLOAT ET9FARCALL _ET9sqrt_f(const ET9FLOAT fX)
{
#ifdef ET9_DEACTIVATE_MATHLIB_USE
    return (ET9FLOAT)_ET9sqrt_f_approximate(fX);
#else
    return (ET9FLOAT)sqrt(fX);
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates the natural log of a float value.
 *
 * @param fX             Nonnegative floating-point value x.
 *
 * @return Returns ln(x).
 */

ET9FLOAT ET9FARCALL _ET9log_f(const ET9FLOAT fX)
{
#ifdef ET9_DEACTIVATE_MATHLIB_USE
    return (ET9FLOAT)_ET9log_f_approximate(fX);
#else
    return (ET9FLOAT)log(fX);
#endif
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates x raised to the power of y.
 *
 * @param fX             Base
 * @param fY             Exponent
 *
 * @return Returns the value of x^y.
 */

ET9FLOAT ET9FARCALL _ET9pow_f(const ET9FLOAT fX, const ET9FLOAT fY)
{
#ifdef ET9_DEACTIVATE_MATHLIB_USE
    return (ET9FLOAT)_ET9pow_f_approximate(fX, fY);
#else
    return (ET9FLOAT)pow(fX, fY);
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Binary to hex.
 * Performs binary to hex conversion and places results into destination.
 *
 * @param byNumber       Number to be converted.
 * @param psDest         Destination for 2 bytes of results.
 *
 * @return None
 */

void ET9FARCALL _ET9BinaryToHex(ET9U8       byNumber,
                                ET9SYMB    *psDest)
{
    static const ET9U8 byDigits[] = "0123456789ABCDEF";

    ET9Assert(psDest);

    *psDest++ = byDigits[byNumber >> 4];
    *psDest++ = byDigits[byNumber & 0x0f];
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check fundamental data types.
 *
 * @return ET9STATUS_TYPE_ERROR on failure, otherwise ET9STATUS_NONE.
 */

ET9STATUS ET9FARCALL _ET9CheckFundamentalTypes(void)
{
    ET9U8   bValue;
    ET9U16  wValue;
    ET9U32  dwValue;
    ET9UINT nValue;
    ET9S8   sbValue;
    ET9S16  swValue;
    ET9S32  sdwValue;
    ET9INT  snValue;

    /* ET9U8 */

    bValue = 255U;
    if (bValue != 255U) {
        return ET9STATUS_TYPE_ERROR;
    }

    bValue = (ET9U8)-1;
    if (bValue < 0) {                   /* compiler warning expected */
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9U16 */

    wValue = 65535U;
    if (wValue != 65535U) {
        return ET9STATUS_TYPE_ERROR;
    }

    wValue = (ET9U16)-1;
    if (wValue < 0) {                   /* compiler warning expected */
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9U32 */

    dwValue = 4294967295U;
    if (dwValue != 4294967295U) {
        return ET9STATUS_TYPE_ERROR;
    }

    dwValue = (ET9U32)-1;
    if (dwValue < 0) {                  /* compiler warning expected */
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9UINT (min 16 bits) */

    nValue = 65535U;
    if (nValue != 65535U) {
        return ET9STATUS_TYPE_ERROR;
    }

    nValue = (ET9UINT)-1;
    if (nValue < 0) {                   /* compiler warning expected */
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9S8 */

    sbValue = 127;
    if (sbValue != 127) {
        return ET9STATUS_TYPE_ERROR;
    }

    sbValue = -127;
    if (sbValue != -127 || sbValue >= 0) {
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9S16 */

    swValue = 32767;
    if (swValue != 32767) {
        return ET9STATUS_TYPE_ERROR;
    }

    swValue = -32767;
    if (swValue != -32767 || swValue >= 0) {
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9S32 */

    sdwValue = 2147483647;
    if (sdwValue != 2147483647) {
        return ET9STATUS_TYPE_ERROR;
    }

    sdwValue = -2147483647;
    if (sdwValue != -2147483647 || sdwValue >= 0) {
        return ET9STATUS_TYPE_ERROR;
    }

    /* ET9INT (min 16 bits) */

    snValue = 32767;
    if (snValue != 32767) {
        return ET9STATUS_TYPE_ERROR;
    }

    snValue = -32767;
    if (snValue != -32767 || snValue >= 0) {
        return ET9STATUS_TYPE_ERROR;
    }

    /* FLOAT and math lib */

#ifdef ET9_DEACTIVATE_MATHLIB_USE
    {
        /* validate assumptions of float composition */
        ET9FLOAT f = 33.0f;
        ET9U32 n = *((ET9U32 *)&f);

        if (0x42040000 != n) {
            return ET9STATUS_TYPE_ERROR;
        }
    }
#else
    {
        ET9FLOAT fValue;

        fValue = 1.5;

        if (fValue != 1.5) {
            return ET9STATUS_TYPE_ERROR;
        }

        fValue = -1.5;

        if (fValue != -1.5) {
            return ET9STATUS_TYPE_ERROR;
        }

        fValue = _ET9acos_f(0.5);

        if (fValue < 1.047 || fValue > 1.048) {
            return ET9STATUS_MATH_ERROR;
        }

        fValue = _ET9acos_f(-0.5);

        if (fValue < 2.094 || fValue > 2.095) {
            return ET9STATUS_MATH_ERROR;
        }

        fValue = _ET9sqrt_f(2);

        if (fValue < 1.414 || fValue > 1.415) {
            return ET9STATUS_MATH_ERROR;
        }

        fValue = _ET9pow_f(2, 16);

        if (fValue != 65536.0) {
            return ET9STATUS_MATH_ERROR;
        }
    }
#endif

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/

static const ET9U8  __FirstByteMark[]    = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static const ET9U32 __OffsetsFromUTF8[]  = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

static const ET9U16 __MaximumUCS2        = 0xFFFF;

/*---------------------------------------------------------------------------*/
/** \internal
 * Converting Unicode to UTF8.
 *
 * @param sIn              Unicode symbol convert.
 * @param pbOut            UTF8 receving buffer (must be at least 4 bytes long).
 *
 * @return Length of converted UTF8 symbol.
 */

ET9U8 ET9FARCALL _ET9SymbToUtf8(const ET9SYMB           sIn,
                                ET9U8           * const pbOut)
{
    ET9U8           bBytesToWrite;
    ET9U8           *pbCurr;
    ET9SYMB         sChar;
    const ET9SYMB   sByteMask = 0xBF;
    const ET9SYMB   sByteMark = 0x80;

    sChar = sIn;
    pbCurr = pbOut;

    /* It would be nice to avoid this conditional */

    if (sChar < 0x80) {
        bBytesToWrite = 1;
    }
    else if (sChar < 0x800) {
        bBytesToWrite = 2;
    }
    else {
        bBytesToWrite = 3;
    }

    pbCurr += bBytesToWrite - 1;

    /* Write the converted bytes to the target buffer */

    switch (bBytesToWrite) /* note: code falls through cases! */
    {
        case 3:
            *pbCurr-- = (ET9U8)((sChar | sByteMark) & sByteMask);
            sChar >>= 6;
        case 2:
            *pbCurr-- = (ET9U8)((sChar | sByteMark) & sByteMask);
            sChar >>= 6;
        case 1:
            *pbCurr   = (ET9U8)(sChar | __FirstByteMark[bBytesToWrite]);
    }

    return bBytesToWrite;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Converting UTF8 to Unicode.
 *
 * @param pbIn             UTF8 buffer to convert.
 * @param pbEnd            Pointer to the byte after the end of the buffer (or NULL to skip validation).
 * @param psOut            Unicode receiving buffer.
 *
 * @return Length of converted UTF8 symbol.
 */

ET9U8 ET9FARCALL _ET9Utf8ToSymb(ET9U8      const * const pbIn,
                                ET9U8      const * const pbEnd,
                                ET9SYMB          * const psOut)
{
    ET9U8  bBytesToRead;
    ET9U32 dwResult = 0;
    ET9U8           const *pbCurr;

    pbCurr = pbIn;

    /* for multi-byte symbols */

    dwResult = 0;

    if (*pbCurr < 0xC0) {
        bBytesToRead = 1;
    }
    else if (*pbCurr < 0xE0) {
        bBytesToRead = 2;
    }
    else if (*pbCurr < 0xF0) {
        bBytesToRead = 3;
    }
    /* else for 4 bytes it should be less than 0xf8, but we should never see this */
    else {
        bBytesToRead = 4;
    }

    if (pbEnd && pbIn + bBytesToRead > pbEnd) {
        *psOut = 0;
        return (ET9U8)(pbEnd - pbIn);
    }

    switch (bBytesToRead) /* note: code falls through cases! */
    {
        case 4:
            dwResult = *pbCurr++;
            dwResult <<= 6;
        case 3:
            dwResult = dwResult + *pbCurr++;
            dwResult <<= 6;
        case 2:
            dwResult = dwResult + *pbCurr++;
            dwResult <<= 6;
        case 1:
            dwResult = dwResult + *pbCurr++;
    }

    dwResult = dwResult - __OffsetsFromUTF8[bBytesToRead - 1];

    if (dwResult <= __MaximumUCS2) {
        *psOut = (ET9U16)dwResult;
    }
    else {
        bBytesToRead = 0;
    }

    return bBytesToRead;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decode a special char - will not support the exhaustive set, just some common ones.
 *
 * Described more here: http://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
 *
 * @param pbIn             UTF8 buffer to convert.
 * @param pbEnd            Pointer to the byte after the end of the buffer.
 * @param psOut            Unicode receiving buffer.
 *
 * @return Number of bytes consumed.
 */

ET9U8 ET9FARCALL _ET9DecodeSpecialChar(ET9U8      const * const pbIn,
                                       ET9U8      const * const pbEnd,
                                       ET9SYMB          * const psOut)
{
    ET9U8   const * pbEndChar;

    *psOut = 0;

    if (pbIn >= pbEnd || *pbIn != '&') {
        return 0;
    }

    for (pbEndChar = pbIn + 1; pbEndChar < pbEnd; ++pbEndChar) {
        if (*pbEndChar == ';') {
            break;
        }
    }

    if (pbEndChar >= pbEnd) {
        return 0;
    }

    /* number */

    if (*(pbIn + 1) == '#') {

        ET9U8   const * pbCurr;
        ET9UINT         nSymbValue = 0;

        for (pbCurr = pbIn + 2; pbCurr < pbEndChar; ++pbCurr) {
            if (*pbCurr >= '0' && *pbCurr <= '9') {
                nSymbValue = nSymbValue * 10 + (*pbCurr - '0');
            }
        }

        *psOut = (ET9SYMB)nSymbValue;

        return (ET9U8)(pbEndChar - pbIn + 1);
    }

    /* string */

    {
        ET9U8     const *pbCurr;
        ET9U32          dwHashValue;

        dwHashValue = 0;
        for (pbCurr = pbIn + 1; pbCurr < pbEndChar; ++pbCurr) {
            dwHashValue = *pbCurr + (65599 * dwHashValue);
        }

        switch (dwHashValue)
        {
            case 0xc38e5609: *psOut = 0x0022; break;     /* quot     */
            case 0x3030fb24: *psOut = 0x0026; break;     /* amp      */
            case 0xd8aafc93: *psOut = 0x0027; break;     /* apos     */
            case 0x006c1b08: *psOut = 0x003c; break;     /* lt       */
            case 0x006719cd: *psOut = 0x003e; break;     /* gt       */
            case 0x2ea2be31: *psOut = 0x00a0; break;     /* nbsp     */
            case 0x729db005: *psOut = 0x00a1; break;     /* iexcl    */
            case 0x304cf348: *psOut = 0x00a2; break;     /* cent     */
            case 0x1de944ac: *psOut = 0x00a3; break;     /* pound    */
            case 0x8ec1fc9b: *psOut = 0x00a4; break;     /* curren   */
            case 0x3bfa6d42: *psOut = 0x00a5; break;     /* yen      */
            case 0x5f06c84d: *psOut = 0x00a6; break;     /* brvbar   */
            case 0x18aefc83: *psOut = 0x00a7; break;     /* sect     */
            case 0x3a0a3134: *psOut = 0x00a8; break;     /* uml      */
            case 0x353b8ed5: *psOut = 0x00a9; break;     /* copy     */
            case 0x64fb8345: *psOut = 0x00aa; break;     /* ordf     */
            case 0x80e721f6: *psOut = 0x00ab; break;     /* laquo    */
            case 0x3699c533: *psOut = 0x00ac; break;     /* not      */
            case 0x39091104: *psOut = 0x00ad; break;     /* shy      */
            case 0x388800b4: *psOut = 0x00ae; break;     /* reg      */
            case 0xff8dda03: *psOut = 0x00af; break;     /* macr     */
            case 0x31a327a6: *psOut = 0x00b0; break;     /* deg      */
            case 0xd7ce197b: *psOut = 0x00b1; break;     /* plusmn   */
            case 0x209cf784: *psOut = 0x00b2; break;     /* sup2     */
            case 0x209cf785: *psOut = 0x00b3; break;     /* sup3     */
            case 0xf4c3ec24: *psOut = 0x00b4; break;     /* acute    */
            case 0x32222d24: *psOut = 0x00b5; break;     /* micro    */
            case 0x8b314fe0: *psOut = 0x00b6; break;     /* para     */
            case 0xab2f1401: *psOut = 0x00b7; break;     /* middot   */
            case 0xd1404165: *psOut = 0x00b8; break;     /* cedil    */
            case 0x209cf783: *psOut = 0x00b9; break;     /* sup1     */
            case 0x64fb834c: *psOut = 0x00ba; break;     /* ordm     */
            case 0x18715bfc: *psOut = 0x00bb; break;     /* raquo    */
            case 0xe6f73111: *psOut = 0x00bc; break;     /* frac14   */
            case 0xe6f7310f: *psOut = 0x00bd; break;     /* frac12   */
            case 0xe6f9318f: *psOut = 0x00be; break;     /* frac34   */
            case 0x89567439: *psOut = 0x00bf; break;     /* iquest   */
            case 0xdda1dcc4: *psOut = 0x00c0; break;     /* Agrave   */
            case 0x96089d23: *psOut = 0x00c1; break;     /* Aacute   */
            case 0x714d5178: *psOut = 0x00c2; break;     /* Acirc    */
            case 0xb33dff37: *psOut = 0x00c3; break;     /* Atilde   */
            case 0x0a4531b3: *psOut = 0x00c4; break;     /* Auml     */
            case 0x2b2f8bb1: *psOut = 0x00c5; break;     /* Aring    */
            case 0xfef20766: *psOut = 0x00c6; break;     /* AElig    */
            case 0x9eddb4e2: *psOut = 0x00c7; break;     /* Ccedil   */
            case 0x365361c0: *psOut = 0x00c8; break;     /* Egrave   */
            case 0xeeba221f: *psOut = 0x00c9; break;     /* Eacute   */
            case 0x80fecd7c: *psOut = 0x00ca; break;     /* Ecirc    */
            case 0xc46074af: *psOut = 0x00cb; break;     /* Euml     */
            case 0x8f04e6bc: *psOut = 0x00cc; break;     /* Igrave   */
            case 0x476ba71b: *psOut = 0x00cd; break;     /* Iacute   */
            case 0x90b04980: *psOut = 0x00ce; break;     /* Icirc    */
            case 0x7e7bb7ab: *psOut = 0x00cf; break;     /* Iuml     */
            case 0x224e42b9: *psOut = 0x00d0; break;     /* ETH      */
            case 0xd37eef6a: *psOut = 0x00d1; break;     /* Ntilde   */
            case 0x140f2e36: *psOut = 0x00d2; break;     /* Ograve   */
            case 0xcc75ee95: *psOut = 0x00d3; break;     /* Oacute   */
            case 0x283a8386: *psOut = 0x00d4; break;     /* Ocirc    */
            case 0xe9ab50a9: *psOut = 0x00d5; break;     /* Otilde   */
            case 0x95a49c25: *psOut = 0x00d6; break;     /* Ouml     */
            case 0x12785e06: *psOut = 0x00d7; break;     /* times    */
            case 0x2bf7bd0e: *psOut = 0x00d8; break;     /* Oslash   */
            case 0x991975b0: *psOut = 0x00d9; break;     /* Ugrave   */
            case 0x5180360f: *psOut = 0x00da; break;     /* Uacute   */
            case 0xbfc4bd8c: *psOut = 0x00db; break;     /* Ucirc    */
            case 0xaccd809f: *psOut = 0x00dc; break;     /* Uuml     */
            case 0xaa31bb0b: *psOut = 0x00dd; break;     /* Yacute   */
            case 0x86b2bf57: *psOut = 0x00de; break;     /* THORN    */
            case 0xe505cd23: *psOut = 0x00df; break;     /* szlig    */
            case 0xa32e04a4: *psOut = 0x00e0; break;     /* agrave   */
            case 0x5b94c503: *psOut = 0x00e1; break;     /* aacute   */
            case 0xeed93198: *psOut = 0x00e2; break;     /* acirc    */
            case 0x78ca2717: *psOut = 0x00e3; break;     /* atilde   */
            case 0xdb1f4993: *psOut = 0x00e4; break;     /* auml     */
            case 0xa8bb6bd1: *psOut = 0x00e5; break;     /* aring    */
            case 0x4d57ff66: *psOut = 0x00e6; break;     /* aelig    */
            case 0x6469dcc2: *psOut = 0x00e7; break;     /* ccedil   */
            case 0xfbdf89a0: *psOut = 0x00e8; break;     /* egrave   */
            case 0xb44649ff: *psOut = 0x00e9; break;     /* eacute   */
            case 0xfe8aad9c: *psOut = 0x00ea; break;     /* ecirc    */
            case 0x953a8c8f: *psOut = 0x00eb; break;     /* euml     */
            case 0x54910e9c: *psOut = 0x00ec; break;     /* igrave   */
            case 0x0cf7cefb: *psOut = 0x00ed; break;     /* iacute   */
            case 0x0e3c29a0: *psOut = 0x00ee; break;     /* icirc    */
            case 0x4f55cf8b: *psOut = 0x00ef; break;     /* iuml     */
            case 0x32303ad9: *psOut = 0x00f0; break;     /* eth      */
            case 0x990b174a: *psOut = 0x00f1; break;     /* ntilde   */
            case 0xd99b5616: *psOut = 0x00f2; break;     /* ograve   */
            case 0x92021675: *psOut = 0x00f3; break;     /* oacute   */
            case 0xa5c663a6: *psOut = 0x00f4; break;     /* ocirc    */
            case 0xaf377889: *psOut = 0x00f5; break;     /* otilde   */
            case 0x667eb405: *psOut = 0x00f6; break;     /* ouml     */
            case 0x2b85a0f9: *psOut = 0x00f7; break;     /* divide   */
            case 0xf183e4ee: *psOut = 0x00f8; break;     /* oslash   */
            case 0x5ea59d90: *psOut = 0x00f9; break;     /* ugrave   */
            case 0x170c5def: *psOut = 0x00fa; break;     /* uacute   */
            case 0x3d509dac: *psOut = 0x00fb; break;     /* ucirc    */
            case 0x7da7987f: *psOut = 0x00fc; break;     /* uuml     */
            case 0x6fbde2eb: *psOut = 0x00fd; break;     /* yacute   */
            case 0xe4faaf77: *psOut = 0x00fe; break;     /* thorn    */
            case 0x37c2db7b: *psOut = 0x00ff; break;     /* yuml     */
            case 0x94fef25e: *psOut = 0x03b1; break;     /* alpha    */
            case 0x01cc23f0: *psOut = 0x03b2; break;     /* beta     */
            case 0x19281f18: *psOut = 0x03b4; break;     /* delta    */
            case 0x953f8dcd: *psOut = 0x20ac; break;     /* euro     */
            case 0xd1160cf5: *psOut = 0x2190; break;     /* larr     */
            case 0x73d363ac: *psOut = 0x2190; break;     /* uarr     */
            case 0xe83ef16f: *psOut = 0x2192; break;     /* rarr     */
            case 0x5cdf86fd: *psOut = 0x2193; break;     /* darr     */
            case 0x16fac9f9: *psOut = 0x2194; break;     /* harr     */
            case 0x2ca7aed2: *psOut = 0x21b5; break;     /* crarr    */
            case 0xc1541cd5: *psOut = 0x21d0; break;     /* lArr     */
            case 0x6411738c: *psOut = 0x21d1; break;     /* uArr     */
            case 0xd87d014f: *psOut = 0x21d2; break;     /* rArr     */
            case 0x4d1d96dd: *psOut = 0x21d3; break;     /* dArr     */
            case 0x0738d9d9: *psOut = 0x21d4; break;     /* hArr     */
            case 0x3b14c64a: *psOut = 0x0395; break;     /* Epsilon  */
            case 0x4e13c3c5: *psOut = 0x03a3; break;     /* Sigma    */
            case 0x0072966a: *psOut = 0x03b5; break;     /* epsilon  */
            case 0x4c687433: *psOut = 0x03b9; break;     /* iota     */
            case 0x006d1b48: *psOut = 0x03bc; break;     /* mu       */
            case 0x602a8639: *psOut = 0x03bf; break;     /* omicron  */
            case 0x628434be: *psOut = 0x0398; break;     /* Theta    */
            case 0x004d1368: *psOut = 0x039c; break;     /* Mu       */
            case 0x00701bf9: *psOut = 0x03c0; break;     /* pi       */
            case 0x5dec096a: *psOut = 0x2019; break;     /* rsquo    */
            case 0x09a51a13: *psOut = 0x2022; break;     /* bull     */
            case 0xcf9acd97: *psOut = 0x2203; break;     /* exist    */
            case 0x006e1b77: *psOut = 0x2260; break;     /* ne       */
            case 0xd1120bee: *psOut = 0x27e8; break;     /* lang     */
            case 0xe83af068: *psOut = 0x27e9; break;     /* rang     */
            default:         *psOut = 0;      break;
        }
    }

    return (ET9U8)(pbEndChar - pbIn + 1);
}

#ifdef ET9_DEBUG

/*---------------------------------------------------------------------------*/
/** \internal
 * Compare strings with a max compare length.
 *
 * @param psString1        String to compare (1).
 * @param psString2        String to compare (2).
 * @param wLen             Max length to compare.
 *
 * @return -1 if string-1 is less than string-2, +1 if string-1 is greather than string-2 else zero (equal).
 */

ET9INT ET9FARCALL _ET9symbncmp(ET9SYMB  const * const psString1, ET9SYMB  const * const psString2, const ET9U16 wLen)
{
    ET9U16 wCount;
    ET9U16 const * psSymb1 = psString1;
    ET9U16 const * psSymb2 = psString2;

    for (wCount = wLen; wCount && *psSymb1 && *psSymb1 == *psSymb2; --wCount, ++psSymb1, ++psSymb2) {
    }

    if (!wCount) {
        return 0;
    }
    else if (*psSymb1 < *psSymb2) {
        return -1;
    }
    else if (*psSymb1 > *psSymb2) {
        return +1;
    }
    else {
        return 0;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Create checksum from zero terminated byte string.
 *
 * @param pbString         String to checksum.
 *
 * @return Checksum
 */

ET9U32 ET9FARCALL _ET9ByteStringCheckSum(ET9U8 const * const pbString)
{
    ET9U32 dwCheckSum;
    ET9U8 const * pbByte;

    dwCheckSum = 0;

    for (pbByte = pbString; *pbByte; ++pbByte) {
        dwCheckSum = *pbByte + (65599 * dwCheckSum);
    }

    return dwCheckSum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check fundamental char props.
 *
 * @return ET9STATUS_CHARPROP_ERROR on failure, otherwise ET9STATUS_NONE.
 */

ET9STATUS ET9FARCALL _ET9CheckCharProps(void)
{
#ifdef ET9CHECKCHARPROPS

    static ET9BOOL bTested = 0;

    if (bTested) {
        return ET9STATUS_NONE;
    }

    /* check symbol class */

    {
        ET9SYMB sSymb;
        ET9UINT pnClassCount[ET9_LastSymbClass] = { 0 };

        for (sSymb = 0; ; ++sSymb) {

            if (sSymb == 0xcccc) {
                continue;
            }

            {
                const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

                if (eClass >= ET9_LastSymbClass) {
                    return ET9STATUS_CHARPROP_ERROR;
                }

                ++pnClassCount[eClass];
            }

            if (sSymb == 0xFFFF) {
                break;
            }
        }

#ifdef ET9SYMBOLENCODING_UNICODE

        if (pnClassCount[ET9_WhiteSymbClass] != 90) {
            return ET9STATUS_CHARPROP_ERROR;
        }
        if (pnClassCount[ET9_PunctSymbClass] != 4505) {
            return ET9STATUS_CHARPROP_ERROR;
        }
        if (pnClassCount[ET9_NumbrSymbClass] != 808) {
            return ET9STATUS_CHARPROP_ERROR;
        }
        if (pnClassCount[ET9_AlphaSymbClass] != 58078) {
            return ET9STATUS_CHARPROP_ERROR;
        }
        if (pnClassCount[ET9_UnassSymbClass] != 2054) {
            return ET9STATUS_CHARPROP_ERROR;
        }

#endif /* ET9SYMBOLENCODING_UNICODE */
    }

    /* check upper/lower */

    {
        ET9SYMB sSymb;
        ET9UINT nLower = 0;
        ET9UINT nUpper = 0;

        for (sSymb = 0; ; ++sSymb) {

            if (_ET9SymIsLower(sSymb, 0)) {
                ++nLower;
            }

            if (_ET9SymIsUpper(sSymb, 0)) {
                ++nUpper;
            }

            if (sSymb == 0xFFFF) {
                break;
            }
        }

#ifdef ET9SYMBOLENCODING_UNICODE

        if (nLower != 999) {
            return ET9STATUS_CHARPROP_ERROR;
        }
        if (nUpper != 989) {
            return ET9STATUS_CHARPROP_ERROR;
        }

#endif /* ET9SYMBOLENCODING_UNICODE */
    }

#ifdef ET9SYMBOLENCODING_UNICODE

    {
        static const ET9U16 psTestSymbs[] = { 0x27, 0x3A, 0x300, 0x301, 0x303, 0x309, 0x323, 0x302, 0x30c };

        ET9UINT nNonFree = 0;

        ET9UINT nIndex;

        for (nIndex = 0; nIndex < sizeof(psTestSymbs) / sizeof(ET9U16); ++nIndex) {

            const ET9BOOL bFree = _ET9_IsFree(psTestSymbs[nIndex]);

            if (!bFree) {
                ++nNonFree;
            }
        }

        if (nNonFree) {
            return ET9STATUS_CHARPROP_ERROR;
        }
    }

#endif /* ET9SYMBOLENCODING_UNICODE */

    bTested = 1;

#endif /* ET9CHECKCHARPROPS */

    /* done */

    return ET9STATUS_NONE;
}

#ifdef ET9CHECKCOMPILE
/*---------------------------------------------------------------------------*/
/**
 * ET9_CheckCompileParameters
 * Check the consistency between core data type and integration.
 *
 * @param pbET9U8             Xx
 *
 * @return Error
 */

ET9U32 ET9FARCALL ET9_CheckCompileParameters(ET9U8   *pbET9U8,
                                             ET9U8   *pbET9U16,
                                             ET9U8   *pbET9U32,
                                             ET9U8   *pbET9UINT,
                                             ET9U8   *pbET9S8,
                                             ET9U8   *pbET9S16,
                                             ET9U8   *pbET9S32,
                                             ET9U8   *pbET9INT,
                                             ET9U8   *pbET9SYMB,
                                             ET9U8   *pbET9BOOL,
                                             ET9U8   *pbET9FARDATA,
                                             ET9U8   *pbET9FARCALL,
                                             ET9U8   *pbET9LOCALCALL,
                                             ET9U8   *pbVoidPtr,
                                             ET9U16  *pwET9SymbInfo,
                                             ET9UINT *pwET9WordSymbInfo)
{
    ET9U32 dwError = 0;

    if (!(pbET9U8 && pbET9U16 && pbET9U32 && pbET9S8 && pbET9S16 && pbET9S32 &&
        pbET9SYMB && pbET9UINT && pbET9INT && pbET9BOOL &&
        pbET9FARDATA && pbET9FARCALL && pbET9LOCALCALL &&
        pbVoidPtr && pwET9SymbInfo && pwET9WordSymbInfo)) {
        return (ET9U32)ET9NULL_POINTERS;
    }

    if (sizeof(ET9U8) != *pbET9U8) {
        *pbET9U8 = sizeof(ET9U8);
        dwError |= (1L << ET9WRONGSIZE_ET9U8);
    }

    if (sizeof(ET9U16) != *pbET9U16) {
        *pbET9U16 = sizeof(ET9U16);
        dwError |= (1L << ET9WRONGSIZE_ET9U16);
    }

    if (sizeof(ET9U32) != *pbET9U32) {
        *pbET9U32 = sizeof(ET9U32);
        dwError |= (1L << ET9WRONGSIZE_ET9U32);
    }

    if (sizeof(ET9UINT) != *pbET9UINT) {
        *pbET9UINT = sizeof(ET9UINT);
        dwError |= (1L << ET9WRONGSIZE_ET9UINT);
    }

    if (sizeof(ET9S8) != *pbET9S8) {
        *pbET9S8 = sizeof(ET9S8);
        dwError |= (1L << ET9WRONGSIZE_ET9S8);
    }

    if (sizeof(ET9S16) != *pbET9S16) {
        *pbET9S16 = sizeof(ET9S16);
        dwError |= (1L << ET9WRONGSIZE_ET9S16);
    }

    if (sizeof(ET9S32) != *pbET9S32) {
        *pbET9S32 = sizeof(ET9S32);
        dwError |= (1L << ET9WRONGSIZE_ET9S32);
    }

    if (sizeof(ET9INT) != *pbET9INT) {
        *pbET9INT = sizeof(ET9INT);
        dwError |= (1L << ET9WRONGSIZE_ET9INT);
    }

    if (sizeof(ET9SYMB) != *pbET9SYMB) {
        *pbET9SYMB = sizeof(ET9SYMB);
        dwError |= (1L << ET9WRONGSIZE_ET9SYMB);
    }

    if (sizeof(ET9BOOL) != *pbET9BOOL) {
        *pbET9BOOL = sizeof(ET9BOOL);
        dwError |= (1L << ET9WRONGSIZE_ET9BOOL);
    }

    if (sizeof(void ET9FARDATA *) != *pbET9FARDATA) {
        *pbET9FARDATA = sizeof(void ET9FARDATA *);
        dwError |= (1L << ET9WRONGSIZE_ET9FARDATA);
    }

    if (sizeof(void ET9FARCALL *) != *pbET9FARCALL) {
        *pbET9FARCALL = sizeof(void ET9FARCALL *);
        dwError |= (1L << ET9WRONGSIZE_ET9FARCALL);
    }

    if (sizeof(void ET9LOCALCALL *) != *pbET9LOCALCALL) {
        *pbET9LOCALCALL = sizeof(void ET9LOCALCALL *);
        dwError |= (1L << ET9WRONGSIZE_ET9LOCALCALL);
    }

    if (sizeof(void *) != *pbVoidPtr) {
        *pbVoidPtr = sizeof(void *);
        dwError |= (1L << ET9WRONGSIZE_VOIDPOINTER);
    }

    if (sizeof(ET9SymbInfo) != *pwET9SymbInfo) {
        *pwET9SymbInfo = sizeof(ET9SymbInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9SYMBINFO);
    }

    if (sizeof(ET9WordSymbInfo) != *pwET9WordSymbInfo) {
        *pwET9WordSymbInfo = sizeof(ET9WordSymbInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9WORDSYMBINFO);
    }

    return dwError;
}

#endif

#ifdef ET9_DEBUGLOG
#include "stdlib.h"
#include <io.h>
#include <fcntl.h>

FILE * pLogFile = NULL;

/*---------------------------------------------------------------------------*/
/** \internal
 * Create a log file.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _OpenLogFile()
{
    int nErr;

    pLogFile = _wfopen(L"stlog.txt", L"at");
    if (!pLogFile) {
        nErr = errno;
        return ET9STATUS_ERROR;
    }
    fseek(pLogFile, 0, SEEK_END);

    return ET9STATUS_NONE;

} /* _OpenLogFile */

/*---------------------------------------------------------------------------*/
/** \internal
 * Close a log file.
 *
 * @return None
 */

void ET9FARCALL _CloseLogFile()
{
    if (pLogFile) {
        fclose(pLogFile);
        pLogFile = NULL;
    }

} /* _CloseLogFile */


/*---------------------------------------------------------------------------*/
/** \internal
 * Write data to log file.
 *
 * @param pbyData        Pointer to data.
 * @param wDataLen       Data size.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _WriteLogFile(ET9U8 *pbyData, ET9U16 wDataLen)
{
    ET9STATUS      wStatus = ET9STATUS_NONE;
    TCHAR         swzCharReturn[] = L"\n";

    ET9Assert(pbyData);

    if (!pLogFile) {
        wStatus = _OpenLogFile();
        if (wStatus != ET9STATUS_NONE) {
            return wStatus;
        }
    }

    if (fwrite(pbyData, 1, wDataLen, pLogFile) != wDataLen) {
        return ET9STATUS_ERROR;
    }

    fwrite(swzCharReturn, 2, wcslen(swzCharReturn), pLogFile);

    return ET9STATUS_NONE;

} /* _WriteLogFile */


#endif /* ET9_DEBUGLOG */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
