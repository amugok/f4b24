/* f4b24専用メッセージID */

#define WM_F4B24_IPC (WM_USER + 88)

/* WPARAM:機能番号 LPARAM:無視  */
#define WM_F4B24_IPC_GET_VERSION 100			/* test12 or later */
#define WM_F4B24_IPC_GET_IF_VERSION 101			/* test12 or later */
#define WM_F4B24_IPC_APPLY_CONFIG 102			/* test12 or later */
#define WM_F4B24_IPC_UPDATE_DRIVELIST 103		/* test18 or later */

/* WPARAM:機能番号 LPARAM:文字列を受け渡しするHWND */
#define WM_F4B24_IPC_GET_VERSION_STRING 200		/* test12 or later */
#define WM_F4B24_IPC_GET_VERSION_STRING2 201	/* test12 or later */
#define WM_F4B24_IPC_GET_SUPPORT_LIST 202		/* test12 or later */
#define WM_F4B24_IPC_GET_PLAYING_PATH 204		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_TITLE 205		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_ALBUM 206		/* test17 or later */
#define WM_F4B24_IPC_GET_PLAYING_ARTIST 207		/* test17 or later */
#define WM_F4B24_IPC_GET_CURPATH 208			/* test18 or later */
#define WM_F4B24_IPC_SET_CURPATH 209			/* test18 or later */

/* WPARAM:機能番号 LPARAM:数値 */
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

/* WPARAM:機能番号 LPARAM:HWND */
#define WM_F4B24_HOOK_UPDATE_DRIVELISTE 400		/* test18 or later */
#define WM_F4B24_HOOK_GET_TREE_ROOT 401			/* test18 or later */
