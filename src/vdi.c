/*
  Hatari

  VDI (Virtual Device Interface) (Trap #2)

  To get higher resolutions on the Desktop, we intercept the VDI/Line-A calls and set elements
  in their structures to the higher width/height/cel/planes. We need to intercept the initial Line-A
  call(which we force into the TOS on boot-up) and also the init calls to the VDI.
  As we intercept the VDI calls, this is a good point to pass them on to the accelerated native
  PC functions if we need to - this improves drawing speed far greater than any ST software
  accelerator.
*/

#include "main.h"
#include "decode.h"
#include "file.h"
#include "gemdos.h"
#include "m68000.h"
#include "memAlloc.h"
#include "screen.h"
#include "stMemory.h"
#include "vdi.h"
#include "video.h"
#include "uae-cpu/newcpu.h"


BOOL bUseVDIRes=FALSE;             /* Set to TRUE (if want VDI), or FALSE (ie for games) */
int LineABase;                     /* Line-A structure */
int FontBase;                      /* Font base, used for 16-pixel high font */
unsigned int VDI_OldPC;            /* When call Trap#2, store off PC */

int VDIWidth=640,VDIHeight=480;    /* 640x480,800x600 or 1024x768 */
int VDIRes=0;                      /* 0,1 or 2(low, medium, high) */
int VDIPlanes=4,VDIColours=16,VDICharHeight=8;  /* To match VDIRes */

unsigned long Control;
unsigned long Intin;
unsigned long Ptsin;
unsigned long Intout;
unsigned long Ptsout;


/*-----------------------------------------------------------------------*/
/* Desktop TOS 1.04 and TOS 2.06 desktop configuration files */
unsigned char DesktopScript[504] =
{
0x23,0x61,0x30,0x30,0x30,0x30,0x30,0x30,0x0D,0x0A,0x23,0x62,0x30,0x30,0x30,0x30,0x30,0x30,0x0D,0x0A,0x23,0x63,0x37,0x37,0x37,0x30,0x30,0x30,0x37,0x30,0x30,0x30,
0x36,0x30,0x30,0x30,0x37,0x30,0x30,0x35,0x35,0x32,0x30,0x30,0x35,0x30,0x35,0x35,0x35,0x32,0x32,0x32,0x30,0x37,0x37,0x30,0x35,0x35,0x37,0x30,0x37,0x35,0x30,0x35,
0x35,0x35,0x30,0x37,0x37,0x30,0x33,0x31,0x31,0x31,0x31,0x30,0x33,0x0D,0x0A,0x23,0x64,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x0D,0x0A,
0x23,0x45,0x20,0x31,0x38,0x20,0x31,0x31,0x20,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x37,0x20,0x32,0x36,0x20,0x30,0x43,0x20,
0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x32,0x20,0x30,0x42,0x20,0x32,0x36,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,
0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x41,0x20,0x30,0x46,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,
0x30,0x20,0x30,0x30,0x20,0x30,0x45,0x20,0x30,0x31,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x31,0x20,0x30,0x30,0x20,
0x30,0x30,0x20,0x46,0x46,0x20,0x43,0x20,0x48,0x41,0x52,0x44,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x30,0x20,0x30,0x30,0x20,
0x30,0x30,0x20,0x46,0x46,0x20,0x41,0x20,0x46,0x4C,0x4F,0x50,0x50,0x59,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x30,0x20,0x30,
0x31,0x20,0x30,0x30,0x20,0x46,0x46,0x20,0x42,0x20,0x46,0x4C,0x4F,0x50,0x50,0x59,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x54,0x20,0x30,0x30,
0x20,0x30,0x33,0x20,0x30,0x32,0x20,0x46,0x46,0x20,0x20,0x20,0x54,0x52,0x41,0x53,0x48,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x46,0x20,0x46,0x46,0x20,0x30,0x34,0x20,
0x20,0x20,0x40,0x20,0x2A,0x2E,0x2A,0x40,0x20,0x0D,0x0A,0x23,0x44,0x20,0x46,0x46,0x20,0x30,0x31,0x20,0x20,0x20,0x40,0x20,0x2A,0x2E,0x2A,0x40,0x20,0x0D,0x0A,0x23,
0x47,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x20,0x20,0x2A,0x2E,0x41,0x50,0x50,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x47,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x20,0x20,
0x2A,0x2E,0x50,0x52,0x47,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x50,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x20,0x20,0x2A,0x2E,0x54,0x54,0x50,0x40,0x20,0x40,0x20,0x0D,
0x0A,0x23,0x46,0x20,0x30,0x33,0x20,0x30,0x34,0x20,0x20,0x20,0x2A,0x2E,0x54,0x4F,0x53,0x40,0x20,0x40,0x20,0x0D,0x0A,0x1A,
};

unsigned char NewDeskScript[786] =
{
0x23,0x61,0x30,0x30,0x30,0x30,0x30,0x30,0x0D,0x0A,0x23,0x62,0x30,0x30,0x30,0x30,0x30,0x30,0x0D,0x0A,0x23,0x63,0x37,0x37,0x37,0x30,0x30,0x30,0x37,0x30,0x30,0x30,
0x36,0x30,0x30,0x30,0x37,0x30,0x30,0x35,0x35,0x32,0x30,0x30,0x35,0x30,0x35,0x35,0x35,0x32,0x32,0x32,0x30,0x37,0x37,0x30,0x35,0x35,0x37,0x30,0x37,0x35,0x30,0x35,
0x35,0x35,0x30,0x37,0x37,0x30,0x33,0x31,0x31,0x31,0x31,0x30,0x33,0x0D,0x0A,0x23,0x64,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x0D,0x0A,
0x23,0x4B,0x20,0x34,0x46,0x20,0x35,0x33,0x20,0x34,0x43,0x20,0x30,0x30,0x20,0x34,0x36,0x20,0x34,0x32,0x20,0x34,0x33,0x20,0x35,0x37,0x20,0x34,0x35,0x20,0x35,0x38,
0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,
0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x35,0x32,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x34,0x44,0x20,0x35,0x36,0x20,0x35,0x30,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,
0x23,0x45,0x20,0x31,0x38,0x20,0x30,0x31,0x20,0x30,0x30,0x20,0x30,0x36,0x20,0x0D,0x0A,0x23,0x51,0x20,0x34,0x31,0x20,0x34,0x30,0x20,0x34,0x33,0x20,0x34,0x30,0x20,
0x34,0x33,0x20,0x34,0x30,0x20,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x37,0x20,0x32,0x36,0x20,0x30,0x43,0x20,0x30,0x30,0x20,
0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x32,0x20,0x30,0x42,0x20,0x32,0x36,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,
0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x41,0x20,0x30,0x46,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,
0x30,0x20,0x30,0x45,0x20,0x30,0x31,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x34,0x20,
0x30,0x37,0x20,0x32,0x36,0x20,0x30,0x43,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x43,0x20,0x30,0x42,0x20,0x32,0x36,
0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x38,0x20,0x30,0x46,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,
0x30,0x20,0x40,0x0D,0x0A,0x23,0x57,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x36,0x20,0x30,0x31,0x20,0x31,0x41,0x20,0x30,0x39,0x20,0x30,0x30,0x20,0x40,0x0D,0x0A,
0x23,0x4E,0x20,0x46,0x46,0x20,0x30,0x34,0x20,0x30,0x30,0x30,0x20,0x40,0x20,0x2A,0x2E,0x2A,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x44,0x20,0x46,0x46,0x20,0x30,0x31,
0x20,0x30,0x30,0x30,0x20,0x40,0x20,0x2A,0x2E,0x2A,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x47,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x30,0x30,0x30,0x20,0x2A,0x2E,0x41,
0x50,0x50,0x40,0x20,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x47,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x30,0x30,0x30,0x20,0x2A,0x2E,0x50,0x52,0x47,0x40,0x20,0x40,0x20,
0x40,0x20,0x0D,0x0A,0x23,0x59,0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x30,0x30,0x30,0x20,0x2A,0x2E,0x47,0x54,0x50,0x40,0x20,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x50,
0x20,0x30,0x33,0x20,0x46,0x46,0x20,0x30,0x30,0x30,0x20,0x2A,0x2E,0x54,0x54,0x50,0x40,0x20,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x46,0x20,0x30,0x33,0x20,0x30,0x34,
0x20,0x30,0x30,0x30,0x20,0x2A,0x2E,0x54,0x4F,0x53,0x40,0x20,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x30,0x20,0x30,0x31,0x20,0x30,0x30,0x20,0x46,0x46,
0x20,0x43,0x20,0x48,0x41,0x52,0x44,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x30,0x30,0x20,0x46,0x46,
0x20,0x41,0x20,0x46,0x4C,0x4F,0x50,0x50,0x59,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x4D,0x20,0x30,0x31,0x20,0x30,0x30,0x20,0x30,0x30,0x20,
0x46,0x46,0x20,0x42,0x20,0x46,0x4C,0x4F,0x50,0x50,0x59,0x20,0x44,0x49,0x53,0x4B,0x40,0x20,0x40,0x20,0x0D,0x0A,0x23,0x54,0x20,0x30,0x30,0x20,0x30,0x33,0x20,0x30,
0x32,0x20,0x46,0x46,0x20,0x20,0x20,0x54,0x52,0x41,0x53,0x48,0x40,0x20,0x40,0x20,0x0D,0x0A,
};


/*-----------------------------------------------------------------------*/
/*
  Set Width/Height/BitDepth according to passed GEMRES_640x480, GEMRES_800x600 or GEMRES_1024x768
*/
void VDI_SetResolution(int GEMRes,int GEMColour)
{
  /* Resolution */
  switch(GEMRes)
  {
    case GEMRES_640x480:
      VDIWidth = 640;
      VDIHeight = 480;
      break;
    case GEMRES_800x600:
      VDIWidth = 800;
      VDIHeight = 600;
      break;
    case GEMRES_1024x768:
      VDIWidth = 1024;
      VDIHeight = 768;
      break;
  }

  /* Colour depth */
  switch(GEMColour)
  {
    case GEMCOLOUR_2:
      VDIRes = 2;
      VDIPlanes = 1;
      VDIColours = 2;
      VDICharHeight = 16;
      break;
    case GEMCOLOUR_4:
      VDIRes = 1;
      VDIPlanes = 2;
      VDIColours = 4;
      VDICharHeight = 8;
      break;
    case GEMCOLOUR_16:
      VDIRes = 0;
      VDIPlanes = 4;
      VDIColours = 16;
      VDICharHeight = 8;
      break;
  }

  /* Force screen code to re-set bitmap/full-screen */
  Screen_SetDrawModes();
  /*Screen_SetupRGBTable();*/
  Screen_SetFullUpdate();
  PrevSTRes = -1;

  /* Write resolution to re-boot takes effect with correct bit-depth */
  VDI_FixDesktopInf();
}


/*-----------------------------------------------------------------------*/
/*
  Check VDI call and see if we need to re-direct to our own routines
  Return TRUE if we've handled the exception, else return FALSE

  We enter here with Trap #2, so D1 is pointer to VDI vectors, ie Control, Intin, Ptsin etc...
*/
BOOL VDI(void)
{
  unsigned long TablePtr = Regs[REG_D1];
  /*unsigned short int OpCode;*/

  /* Read off table pointers */
  Control = STMemory_ReadLong(TablePtr);
  Intin = STMemory_ReadLong(TablePtr+4);
  Ptsin = STMemory_ReadLong(TablePtr+8);
  Intout = STMemory_ReadLong(TablePtr+12);
  Ptsout = STMemory_ReadLong(TablePtr+16);

/*
  OpCode = STMemory_ReadWord(Control);
  // Check OpCode
  // 8 - Text Font
  if (OpCode==9)
  {
    return(TRUE);
  }
*/

  /* Call as normal! */
  return(FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Modify Line-A structure for our VDI resolutions
*/
void VDI_LineA(void)
{
  if (bUseVDIRes)
  {
    STMemory_WriteWord(LineABase-6*2,VDIWidth);                      /* v_rez_hz */
    STMemory_WriteWord(LineABase-2*2,VDIHeight);                     /* v_rez_vt */
    STMemory_WriteWord(LineABase-1*2,(VDIWidth*VDIPlanes)/8);        /* bytes_lin */
    STMemory_WriteWord(LineABase+1*2,(VDIWidth*VDIPlanes)/8);        /* width */

    STMemory_WriteWord(LineABase-23*2,VDICharHeight);                /* char height */
    STMemory_WriteWord(LineABase-22*2,(VDIWidth/8)-1);               /* v_cel_mx */
    STMemory_WriteWord(LineABase-21*2,(VDIHeight/VDICharHeight)-1);  /* v_cel_my */
    STMemory_WriteWord(LineABase-20*2,VDICharHeight*((VDIWidth*VDIPlanes)/8));  /* v_cel_wr */

    STMemory_WriteWord(LineABase-0*2,VDIPlanes);                     /* planes */
  }
}


/*-----------------------------------------------------------------------*/
/*
  This is called on completion of a VDI Trap, used to modify return structure for 
*/
void VDI_Complete(void)
{
  unsigned short int OpCode;

  OpCode = STMemory_ReadWord(Control);
  /* Is 'Open Workstation', or 'Open Virtual Screen Workstation'? */
  if ( (OpCode==1) || (OpCode==100) )
  {
    STMemory_WriteWord(Intout,VDIWidth-1);                         /* IntOut[0] Width-1 */
    STMemory_WriteWord(Intout+1*2,VDIHeight-1);                    /* IntOut[1] Height-1 */
    STMemory_WriteWord(Intout+13*2,VDIColours);                    /* IntOut[13] #colours */
    STMemory_WriteWord(Intout+39*2,512);                           /* IntOut[39] #available colours */

    STMemory_WriteWord(LineABase-0x15a*2,VDIWidth-1);              /* WKXRez */
    STMemory_WriteWord(LineABase-0x159*2,VDIHeight-1);             /* WKYRez */

    VDI_LineA();                  /* And modify Line-A structure accordingly */
  }
}


/*-----------------------------------------------------------------------*/
/*
  Save desktop configuration file for VDI, eg desktop.inf(TOS 1.04) or newdesk.inf(TOS 2.06)
*/
void VDI_SaveDesktopInf(char *pszFileName,unsigned char *Script,long ScriptSize)
{
  /* Just save file */
  File_Save(pszFileName, Script, ScriptSize, FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Modify exisiting ST desktop configuration files to set resolution(keep user settings)
*/
void VDI_ModifyDesktopInf(char *pszFileName)
{
  long InfSize;
  unsigned char *pInfData;
  int i;

  /* Load our '.inf' file */
  pInfData = (unsigned char *)File_Read(pszFileName,NULL,&InfSize,NULL);
  if (pInfData)
  {
    /* Scan file for '#E' */
    i = 0;
    while(i<(InfSize-8))
    {
      if ( (pInfData[i]=='#') && (pInfData[i+1]=='E') )
      {
        /* Modify resolution */
        pInfData[i+7] = '1'+VDIRes;
        goto done_modify;
      }

      i++;
    }

done_modify:;
    /* And save */
    File_Save(pszFileName, pInfData, InfSize, FALSE);
    /* Free */
    Memory_Free(pInfData);
  }
}


/*-----------------------------------------------------------------------*/
/*
  Modify (or create) ST desktop configuration files so VDI boots up in
  correct color depth
*/
void VDI_FixDesktopInf(void)
{
  char szDesktopFileName[MAX_FILENAME_LENGTH],szNewDeskFileName[MAX_FILENAME_LENGTH];

  if(!GEMDOS_EMU_ON)
  {
    /* Can't modify DESKTOP.INF when not using GEMDOS hard disk emulation */
    return;
  }

  /* Create filenames for hard-drive */
  GemDOS_CreateHardDriveFileName(2, "\\DESKTOP.INF", szDesktopFileName);
  GemDOS_CreateHardDriveFileName(2, "\\NEWDESK.INF", szNewDeskFileName);

  /* First, check if files exist(ie modify or replace) */
  if (!File_Exists(szDesktopFileName))
    VDI_SaveDesktopInf(szDesktopFileName,DesktopScript,sizeof(DesktopScript));
  VDI_ModifyDesktopInf(szDesktopFileName);

  if (!File_Exists(szNewDeskFileName))
    VDI_SaveDesktopInf(szNewDeskFileName,NewDeskScript,sizeof(NewDeskScript));
  VDI_ModifyDesktopInf(szNewDeskFileName);
}
