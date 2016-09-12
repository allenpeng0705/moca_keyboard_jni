
#ifndef __CONFIG_VERSION_H__
#define __CONFIG_VERSION_H__

namespace mocainput {

/* Speech salt */
void getRCVersionString(jbyte** ppString, jsize* pSize);

/* XT9 Salt */
void getMinorVersionString(jbyte** ppString, jsize* pSize);

/* XT9 Key */
void getMajorVersionString(jbyte** ppString, jsize* pSize);
}

#endif // __CONFIG_NATIVE_H__
