
#ifndef __UCS2_H__
#define __UCS2_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short ucs2_Char;
int utf8ToUcs2(ucs2_Char* dest, int destMaxLen, const char* src);
int ucs2ToUtf8(char* dest, int destMaxLen, const ucs2_Char* src);
int ucs2_strncmp(const ucs2_Char* src1, const ucs2_Char* src2, int len);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* __UCS2_H__ */
