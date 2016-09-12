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
;**     FileName: et9kbdef.h                                                  **
;**                                                                           **
;**  Description: This file contains the definitions specific to any          **
;**               particular Sloppy Type embedded keyboard.                   **
;**               Adheres to V1.0 of the API                                  **
;**                                                                           **
;*******************************************************************************
;******* 10 ****** 20 ****** 30 ****** 40 ****** 50 ****** 60 ****** 70 *******/

#ifndef ET9KBDDEF_H
#define ET9KBDDEF_H 1

typedef enum ET9KBDKEYDEF_e {
    ET9KEY_INVALID,         /* 0000 : Invalid key.         */
    ET9KEY_RELOAD,          /* 0001 : Reload.              */
    ET9KEY_OK,              /* 0002 : OK/Select.           */
    ET9KEY_CANCEL,          /* 0003 : Control break.       */

    ET9KEY_LEFT,            /* 0004 : Previous.            */
    ET9KEY_UP,              /* 0005 : Up.                  */
    ET9KEY_RIGHT,           /* 0006 : Right.               */
    ET9KEY_DOWN,            /* 0007 : Down.                */

    ET9KEY_BACK,            /* 0008 : Backspace.           */
    ET9KEY_TAB,             /* 0009 : Tab.                 */
    ET9KEY_0A,              /* 000A : Reserved.            */
    ET9KEY_PREVTAB,         /* 000B : Previous tab.        */
    ET9KEY_CLEAR,           /* 000C : Clear.               */
    ET9KEY_RETURN,          /* 000D : Return.              */
    ET9KEY_CLOSE_SEL_LIST,  /* 000E : Close selection list */
    ET9KEY_MENU,            /* 000F : Menu.                */
    ET9KEY_SHIFT,           /* 0010 : Shift.               */
    ET9KEY_CONTROL,         /* 0011 : CTRL.                */
    ET9KEY_ALT,             /* 0012 : ALT.                 */
    ET9KEY_PAUSE,           /* 0013 : Pause.               */
    ET9KEY_CAPS_LOCK,       /* 0014 : Caps Lock.           */

    ET9KEY_OPTION,          /* 0015 : Option.              */
    ET9KEY_EMOTICON,        /* 0016 : Emoticon.            */
    ET9KEY_ACCENTEDLAYOUT,  /* 0017 : Accented layout.     */
    ET9KEY_SYMBOLLAYOUT,    /* 0018 : Symbol layout.       */
    ET9KEY_MAINLAYOUT,      /* 0019 : First level layout (alpha).  */
    ET9KEY_MULTITAP,        /* 001A : Multi-tap mode.      */

    ET9KEY_ESCAPE,          /* 001B : Escape.              */
    ET9KEY_PRIOR,           /* 001C : Previous.            */
    ET9KEY_NEXT,            /* 001D : Next.                */
    ET9KEY_END,             /* 001E : End.                 */
    ET9KEY_HOME,            /* 001F : Begin.               */
    ET9KEY_SPACE,           /* 0020 : Space.               */

    ET9KEY_DIGITLAYOUT,     /* 0021 : Digit layout.  */
    ET9KEY_PUNCTLAYOUT,     /* 0022 : Punctuation layout.  */
    ET9KEY_LANGUAGE,        /* 0023 : Language.            */
    ET9KEY_UNDO,            /* 0024 : Undo.                */
    ET9KEY_REDO,            /* 0025 : Redo.                */
    ET9KEY_HIDE,            /* 0026 : Hide.                */

    /* Additional key will be defined in the range of 0xE000 to 0xF8FF */

    ET9KEY_WINDOWS  = 0xE001,   /* E001 : Windows Key.      */
    ET9KEY_SOFT1,               /* E002 : Soft Key 1        */
    ET9KEY_SOFT2,               /* E003 : Soft Key 2        */
    ET9KEY_SOFT3,               /* E004 : Soft Key 3        */
    ET9KEY_SOFT4,               /* E005 : Soft Key 4        */
    ET9KEY_OEMLAYOUT1,          /* E006 : OEM Layout 1      */
    ET9KEY_OEMLAYOUT2,          /* E007 : OEM Layout 2      */
    ET9KEY_OEMLAYOUT3,          /* E008 : OEM Layout 3      */
    ET9KEY_OEMLAYOUT4,          /* E009 : OEM Layout 4      */

    ET9KEY_OEM_01   = 0xE101,   /* E101 : OEM Key 1.        */
    ET9KEY_OEM_02,              /* E102 : OEM Key 2.        */
    ET9KEY_OEM_03,              /* E103 : OEM Key 3.        */
    ET9KEY_OEM_04,              /* E104 : OEM Key 4.        */
    ET9KEY_OEM_05,              /* E105 : OEM Key 5.        */
    ET9KEY_OEM_06,              /* E106 : OEM Key 6.        */
    ET9KEY_OEM_07,              /* E107 : OEM Key 7.        */
    ET9KEY_OEM_08,              /* E108 : OEM Key 8.        */
    ET9KEY_OEM_09,              /* E109 : OEM Key 9.        */
    ET9KEY_OEM_0A,              /* E10A : OEM Key A.        */
    ET9KEY_OEM_0B,              /* E10B : OEM Key B.        */
    ET9KEY_OEM_0C,              /* E10C : OEM Key C.        */
    ET9KEY_OEM_0D,              /* E10D : OEM Key D.        */
    ET9KEY_OEM_0E,              /* E10E : OEM Key E.        */
    ET9KEY_OEM_0F               /* E10F : OEM Key F.        */

} ET9KBDKEYDEF;

#endif /* ET9KBDDEF_H */


/* ----------------------------------< eof >--------------------------------- */
