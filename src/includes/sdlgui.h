/*
  Hatari - sdlgui.h

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Header for the tiny graphical user interface for Hatari.
*/

#ifndef HATARI_SDLGUI_H
#define HATARI_SDLGUI_H

#include <SDL.h>

enum
{
  SGBOX,
  SGTEXT,
  SGEDITFIELD,
  SGBUTTON,
  SGRADIOBUT,
  SGCHECKBOX,
  SGPOPUP
};


/* Object flags: */
#define SG_TOUCHEXIT  1
#define SG_EXIT       2  /* Not yet tested */

/* Object states: */
#define SG_SELECTED   1

/* Special characters: */
#define SGRADIOBUTTON_NORMAL    12
#define SGRADIOBUTTON_SELECTED  13
#define SGCHECKBOX_NORMAL    14
#define SGCHECKBOX_SELECTED  15
#define SGARROWUP    1
#define SGARROWDOWN  2
#define SGFOLDER     5


typedef struct
{
  int type;             /* What type of object */
  int flags;            /* Object flags */
  int state;		/* Object state */
  int x, y;             /* The offset to the upper left corner */
  int w, h;             /* Width and height */
  char *txt;            /* Text string */
}  SGOBJ;


extern int SDLGui_Init(void);
extern int SDLGui_UnInit(void);
extern int SDLGui_PrepareFont(void);
extern int SDLGui_DoDialog(SGOBJ *dlg);
extern void SDLGui_CenterDlg(SGOBJ *dlg);

/* prototypes for gui-sdl/dlg*.c functions */
extern int Dialog_MainDlg(BOOL *bReset);
extern int SDLGui_FileSelect(char *path_and_name, char *zip_path, BOOL bAllowNew);
extern void Dialog_AboutDlg(void);
extern void Dialog_DeviceDlg(void);
extern void Dialog_DiscDlg(void);
extern void Dialog_JoyDlg(void);
extern void Dialog_KeyboardDlg(void);
extern void Dialog_MemDlg(void);
extern void DlgNewDisc_Main(void);
extern void Dialog_ScreenDlg(void);
extern void Dialog_SoundDlg(void);
extern void Dialog_SystemDlg(void);
extern void Dialog_TosGemDlg(void);

#endif
