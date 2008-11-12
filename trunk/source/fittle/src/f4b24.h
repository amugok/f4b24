/* f4b24専用メッセージID */

#define WM_F4B24_IPC (WM_USER + 88)

/* WPARAM:function code LPARAM:ignore  */
#define WM_F4B24_IPC_GET_VERSION 100				/* test12 or later */
#define WM_F4B24_IPC_GET_IF_VERSION 101				/* test12 or later */
#define WM_F4B24_IPC_APPLY_CONFIG 102				/* test12 or later */
#define WM_F4B24_IPC_UPDATE_DRIVELIST 103			/* test18 or later */
#define WM_F4B24_IPC_GET_REPLAYGAIN_MODE 104		/* test25 or later */
#define WM_F4B24_IPC_GET_PREAMP 105					/* test28 or later */
#define WM_F4B24_IPC_INVOKE_OUTPUT_PLUGIN_SETUP 106	/* test28 or later */
#define WM_F4B24_IPC_TRAYICONMENU 107				/* test36 or later */
#define WM_F4B24_IPC_GET_LX_IF 108					/* test36 or later */

/* WPARAM:function code LPARAM:HWND */
#define WM_F4B24_IPC_GET_VERSION_STRING 200		/* test12 or later */
#define WM_F4B24_IPC_GET_VERSION_STRING2 201	/* test12 or later */
#define WM_F4B24_IPC_GET_SUPPORT_LIST 202		/* test12 or later */
#define WM_F4B24_IPC_GET_PLAYING_PATH 204		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_TITLE 205		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_ALBUM 206		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_ARTIST 207		/* test17 or later */
#define WM_F4B24_IPC_GET_CURPATH 208			/* test18 or later */
#define WM_F4B24_IPC_SET_CURPATH 209			/* test18 or later */
#define WM_F4B24_IPC_GET_PLAYING_TRACK 210		/* test25 or later */
#define WM_F4B24_IPC_GET_WADSP_LIST 211			/* wadsp */

/* WPARAM:function code LPARAM:number */
#define WM_F4B24_IPC_SETTING 300				/* test13 or later */
#define  WM_F4B24_IPC_SETTING_LP_GENERAL 0
#define  WM_F4B24_IPC_SETTING_LP_PATH 1
#define  WM_F4B24_IPC_SETTING_LP_CONTROL 2
#define  WM_F4B24_IPC_SETTING_LP_TASKTRAY 3
#define  WM_F4B24_IPC_SETTING_LP_HOTKEY 4
#define  WM_F4B24_IPC_SETTING_LP_ABOUT 5
#define  WM_F4B24_IPC_SETTING_LP_MAX_V13 5
#define  WM_F4B24_IPC_SETTING_LP_MAX 5
#define WM_F4B24_IPC_GET_CAPABLE 301			/* test13 or later */
#define  WM_F4B24_IPC_GET_CAPABLE_LP_FLOATOUTPUT 0
#define  WM_F4B24_IPC_GET_CAPABLE_RET_SUPPORTED 0x202
#define  WM_F4B24_IPC_GET_CAPABLE_RET_NOT_SUPPORTED 0x101
#define WM_F4B24_IPC_INVOKE_WADSP_SETUP 302		/* wadsp */
#define WM_F4B24_IPC_GET_COLUMN_TYPE 303		/* test39 or later */

/* WPARAM:function code LPARAM:HWND */
#define WM_F4B24_HOOK_UPDATE_DRIVELISTE 400		/* test18 or later */
#define WM_F4B24_HOOK_GET_TREE_ROOT 401			/* test18 or later */
#define WM_F4B24_HOOK_LIST_CREATED 402			/* test39 or later */

/* WPARAM:function code LPARAM:BASS stream handle */
#define WM_F4B24_HOOK_CREATE_STREAM 500			/* test25 or later */
#define WM_F4B24_HOOK_FREE_STREAM 501			/* test25 or later */
#define WM_F4B24_HOOK_REPLAY_GAIN 502			/* test25 or later */
#define WM_F4B24_HOOK_CREATE_DECODE_STREAM 503	/* test25 or later */
#define WM_F4B24_HOOK_FREE_DECODE_STREAM 504	/* test28 or later */
#define WM_F4B24_HOOK_START_DECODE_STREAM 505	/* test32 or later */
#define WM_F4B24_HOOK_CREATE_ASIO_STREAM 506	/* test32 or later */
#define WM_F4B24_HOOK_FREE_ASIO_STREAM 507		/* test32 or later */
