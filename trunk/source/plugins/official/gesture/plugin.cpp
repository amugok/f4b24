/*
 * gesture
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#define WINVER		0x0400	// 98以降
#define _WIN32_IE	0x0400	// IE4以降

#pragma comment(lib, "shlwapi.lib")

#include "plugin.h"
#include <commctrl.h>
#include <shlwapi.h>

#define ID_STATUS	110
enum DIRECTION{ END=0, RIGHT, LEFT, UP, DOWN};

#define CHECK_DISTANCE 20*20
#define MAX_GES_COUNT 100

BOOL OnInit();
void OnQuit();
void OnTrackChange();
void OnStatusChange();
void OnConfig();

FITTLE_PLUGIN_INFO fpi = {
	PDK_VER,
	OnInit,
	OnQuit,
	OnTrackChange,
	OnStatusChange,
	OnConfig,
	NULL,
	NULL
};

/* ジェスチャーアイテム */
typedef struct{
	int nCmdId;
	int data[MAX_GES_COUNT];
} GESDATA;

/* 適当キュー */
typedef struct {
	int data[MAX_GES_COUNT];
	int qp;
} QUEUE;

HWND hStatus;
HHOOK hHook;
int nGesCount = 0;
GESDATA GesData[100] = {0};

int CalcOffsetSquare(int s, int t){
	return (s - t)*(s - t);
}

LRESULT CALLBACK MyHookProc(int nCode, WPARAM wp, LPARAM lp){
	#define msg ((MSG *)lp)

	static POINT ptBase;
	static char szStatus[100];
	static QUEUE queue = {0};
	static BOOL bDown = FALSE;
	static BOOL bHook = TRUE;

	int nXs, nYs, i, j, nDir;
	POINT pt;
	char szGesture[100];

	if(nCode<0)
		return CallNextHookEx(hHook, nCode, wp, lp);
	
	switch(msg->message){
		case WM_RBUTTONDOWN:
			if(bHook && !bDown){
				// ジェスチャー開始
				OutputDebugString("D");
				SetCapture(fpi.hParent);
				bDown = TRUE;

				// 基点を設定
				ptBase.x = msg->pt.x;
				ptBase.y = msg->pt.y;

				// キューを初期化
				queue.qp = 0;

				// ステータスバーの文字を覚える
				SendMessage(hStatus, SB_GETTEXT, (WPARAM)0, (LPARAM)szStatus);
				msg->message = WM_NULL;
			}
			break;

		case WM_MOUSEMOVE:
			if(bHook && bDown){
				// マウス移動中
				OutputDebugString("M");

				// 基点からの距離を求める
				pt.x = msg->pt.x;
				pt.y = msg->pt.y;
				nXs = CalcOffsetSquare(pt.x, ptBase.x);
				nYs = CalcOffsetSquare(pt.y, ptBase.y);

				// 距離がCHECK_DISTANCE以上
				if(nXs + nYs >= CHECK_DISTANCE){
					if(nXs>=nYs){
						// 右か左
						if(pt.x>ptBase.x){
							nDir = RIGHT;
						}else{
							nDir = LEFT;
						}
					}else{
						// 下か右
						if(pt.y>ptBase.y){
							nDir = DOWN;
						}else{
							nDir = UP;
						}
					}

			
					// 前回の方向と違ったらジェスチャーとして認識
					if(queue.qp==0 || nDir!=queue.data[queue.qp-1]){
						queue.data[queue.qp++] = nDir;
					}

					// 基点を変更
					ptBase.x = pt.x;
					ptBase.y = pt.y;

					// ジェスチャーを表示
					lstrcpy(szGesture, "Gesture: ");
					for(i=0;i<queue.qp;i++){
						switch(queue.data[i]){
							case RIGHT:
								lstrcat(szGesture, "→");
								break;
							case LEFT:
								lstrcat(szGesture, "←");
								break;
							case DOWN:
								lstrcat(szGesture, "↓");
								break;
							case UP:
								lstrcat(szGesture, "↑");
								break;
						}
					}
					PostMessage(hStatus, SB_SETTEXT, 0|0, (LPARAM)szGesture);
				}
			}
			break;

		case WM_RBUTTONUP:
			if(!bHook){
				bHook = TRUE;	// マウスアップを処理したらフック再開
			}else if(bDown){
				// ジェスチャ終了
				OutputDebugString("U\n");
				bDown = FALSE;
				ReleaseCapture();
				PostMessage(hStatus, SB_SETTEXT, 0|0, (LPARAM)szStatus);

				if(queue.qp<=0){
					// ジェスチャーじゃなくて右クリック
					pt.x = msg->pt.x;
					pt.y = msg->pt.y;
					ScreenToClient(WindowFromPoint(msg->pt), &pt);

					bHook = FALSE;
					PostMessage(WindowFromPoint(msg->pt), WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));

					/* 合った方がいいのかわからないので保留
					RECT rcDisp;
					GetClientRect(WindowFromPoint(msg->pt), &rcDisp);
					PostMessage(WindowFromPoint(msg->pt), WM_SIZE, 0, MAKELPARAM(rcDisp.right, rcDisp.bottom));
					*/

					PostMessage(WindowFromPoint(msg->pt), WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
				}else{
					// ジェスチャーアクション
					for(i=0;i<nGesCount;i++){
						// 前方から順番に一途を確認
						for(j=0;j<queue.qp;j++){
							if(queue.data[j]!=GesData[i].data[j]) break;
						}
						// 最後まで一致していたらコマンドを投げて終了
						if(j==queue.qp && GesData[i].data[queue.qp]==END){
							PostMessage(fpi.hParent, WM_COMMAND, MAKEWPARAM(GesData[i].nCmdId, 0), 0);
							break;
						}
					}
				}
				msg->message = WM_NULL;
			}
			break;
	}

	return CallNextHookEx(hHook, nCode, wp, lp);
}

/* エントリポント */
BOOL WINAPI _DllMainCRTStartup(HANDLE /*hModule*/, DWORD /*dwFunction*/, LPVOID /*lpNot*/){
    return TRUE;
}

/* 起動時に一度だけ呼ばれます */
BOOL OnInit(){
	char szCfgPath[MAX_PATH];
	int nLen, nID;
	int i=0,k;
	char c;
	char szLine[256] = {0};
	char *p;
	HANDLE hFile;
	DWORD dwAccBytes = 1;

	// 設定ファイルのパスを取得
	nLen = GetModuleFileName(fpi.hDllInst, szCfgPath, MAX_PATH);
	szCfgPath[nLen-3] = 'c';
	szCfgPath[nLen-2] = 'f';
	szCfgPath[nLen-1] = 'g';

	hFile = CreateFile(szCfgPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return FALSE;	// 設定ファイルが読めないなら終了

	while(dwAccBytes){
		ReadFile(hFile, &c, sizeof(char), &dwAccBytes, NULL);
		switch(c){
			//case '\r':
			case '\n':
			case '\0':
				szLine[i++] = '\0';
				OutputDebugString(szLine);
				OutputDebugString("\t");
				// コマンドIDを取る
				nID = StrToInt(szLine);
				if(nID>0){
					GesData[nGesCount].nCmdId = nID;
					p = szLine + lstrlen(szLine) + 1;
					OutputDebugString(p);
					OutputDebugString("\n");
					k = 0;
					for(;*p;p++){
						switch(*p){
							case 'R':
								GesData[nGesCount].data[k++] = RIGHT;
								break;
							case 'L':
								GesData[nGesCount].data[k++] = LEFT;
								break;
							case 'U':
								GesData[nGesCount].data[k++] = UP;
								break;
							case 'D':
								GesData[nGesCount].data[k++] = DOWN;
								break;
						}
					}
					GesData[nGesCount++].data[k] = END;
				}
				i = 0;
				break;

			case ' ':	// ホワイトスペースだったら終端文字で埋める
			case '\t':
				szLine[i++] = '\0';
				break;

			default :
				szLine[i++] = c;
				break;

		}
	};

	CloseHandle(hFile);

	// ステータスバーのハンドル取得
	hStatus = (HWND)SendMessage(fpi.hParent, WM_FITTLE, GET_CONTROL, ID_STATUS);
	// ローカルフック
	hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MyHookProc, (HINSTANCE)GetWindowLong(fpi.hParent, GWL_HINSTANCE), GetWindowThreadProcessId(fpi.hParent, NULL));

	return (BOOL)hHook;
}

/* 終了時に一度だけ呼ばれます */
void OnQuit(){
	return;
}

/* 曲が変わる時に呼ばれます */
void OnTrackChange(){
	return;
}

/* 再生状態が変わる時に呼ばれます */
void OnStatusChange(){
	return;
}

/* 設定画面を呼び出します（未実装）*/
void OnConfig(){
	return;
}

#ifdef __cplusplus
extern "C"
{
#endif
__declspec(dllexport) FITTLE_PLUGIN_INFO *GetPluginInfo(){
	return &fpi;
}
#ifdef __cplusplus
}
#endif