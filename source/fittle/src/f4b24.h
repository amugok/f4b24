/* f4b24専用メッセージID */

#define WM_F4B24_IPC (WM_USER + 88)

/* WPARAM:機能番号 LPARAM:無視  */
#define WM_F4B24_IPC_GET_VERSION 100			/* test12 or later */
#define WM_F4B24_IPC_GET_IF_VERSION 101			/* test12 or later */
#define WM_F4B24_IPC_APPLY_CONFIG 102			/* test12 or later */

/* WPARAM:機能番号 LPARAM:文字列を受け取るHWND */
#define WM_F4B24_IPC_GET_VERSION_STRING 200		/* test12 or later */
#define WM_F4B24_IPC_GET_VERSION_STRING2 201	/* test12 or later */
#define WM_F4B24_IPC_GET_SUPPORT_LIST 202		/* test12 or later */

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
