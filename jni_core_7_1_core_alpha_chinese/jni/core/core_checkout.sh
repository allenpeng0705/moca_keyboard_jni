#!/bin/bash

XT9_IME_CORE_FOLDER=xt9
T9WRITE_IME_ALPHA_CORE_FOLDER=t9write_alpha
T9WRITE_IME_CHINESE_CORE_FOLDER=t9write_chinese

XT9_CORE_SOURCE_CVS_TAG='-r FinalRelease_XT9_7-0-2'
#CJK: Chinese_Japanese_and_Korean
T9WRITE_CJK_CORE_SOURCE_CVS_TAG='-r T9Write_CJK_4-3-1'
#AAH: Alphabetic_Arabic_and_Hebrew
T9WRITE_AAH_CORE_SOURCE_CVS_TAG='-r T9Write_AAH_5-1-2'

cd ${XT9_IME_CORE_FOLDER}
if [ -d alphabetic ]
then
  rm -rf alphabetic
fi
mkdir alphabetic
cd alphabetic
cvs checkout ${XT9_CORE_SOURCE_CVS_TAG} -d core et9/alphabetic/core
cd ..

if [ -d chinese ]
then
  rm -rf chinese
fi
mkdir chinese
cd chinese
cvs checkout ${XT9_CORE_SOURCE_CVS_TAG} -d core et9/chinese/core
cd ..

if [ -d generic ]
then
  rm -rf generic
fi
mkdir generic
cd generic
cvs checkout ${XT9_CORE_SOURCE_CVS_TAG} -d core et9/generic/core

cd ..
cd ..
cd ${T9WRITE_IME_ALPHA_CORE_FOLDER}
if [ -d api ]
then
  rm -rf api
fi
if [ -d src ]
then
  rm -rf src
fi
cvs checkout ${T9WRITE_AAH_CORE_SOURCE_CVS_TAG} -P -d api t9write/alphabetic/core/api
cvs checkout ${T9WRITE_AAH_CORE_SOURCE_CVS_TAG} -P -d src t9write/alphabetic/core/src
              
cd ..
cd ${T9WRITE_IME_CHINESE_CORE_FOLDER}
if [ -d api ]
then
  rm -rf api
fi
if [ -d include ]
then
  rm -rf include
fi
if [ -d src ]
then
  rm -rf src
fi
cvs checkout ${T9WRITE_CJK_CORE_SOURCE_CVS_TAG} -P -d api t9write/cjk/core/api
cvs checkout ${T9WRITE_CJK_CORE_SOURCE_CVS_TAG} -P -d include t9write/cjk/core/include
cvs checkout ${T9WRITE_CJK_CORE_SOURCE_CVS_TAG} -P -d src t9write/cjk/core/src
