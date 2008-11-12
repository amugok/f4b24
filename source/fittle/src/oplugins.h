#ifndef OUTPUT_H_INC_
#define OUTPUT_H_INC_

/*
	output.h
	音声出力関係
	処理の実体も含まれる
*/

/* 出力プラグインリスト */
static struct OUTPUT_PLUGIN_NODE {
	struct OUTPUT_PLUGIN_NODE *pNext;
	OUTPUT_PLUGIN_INFO *pInfo;
	HMODULE hDll;
} *pOutputPluginList = NULL;

/* 初期化された出力プラグインIF */
static OUTPUT_PLUGIN_INFO *m_pOutputPlugin = NULL;
static volatile BOOL m_fOnSetupNext = FALSE;
static const float fBypassVol = -0.00001f;

/* bass.dll単体だと音量増幅ができないため処理をユーザー選択する */
static float CalcBassVolume(DWORD dwVol){
	float fVol = g_cfg.nReplayGainPostAmp * g_cInfo[g_bNow].sGain * dwVol / (float)(SLIDER_DIVIDED * 100);
	switch (g_cfg.nReplayGainMixer) {
	case 0:
		/*  内蔵 */
		g_cInfo[g_bNow].sAmp = (fVol == (float)1) ? fBypassVol : fVol;
		fVol = 1;
		break;
	case 1:
		/*  増幅のみ内蔵 */
		if (fVol > 1){
			g_cInfo[g_bNow].sAmp = fVol;
			fVol = 1;
		}else{
			g_cInfo[g_bNow].sAmp = fBypassVol;
		}
		break;
	case 2:
		/*  BASS */
		g_cInfo[g_bNow].sAmp = fBypassVol;
		if (fVol > 1) fVol = 1;
		break;
	}
	return fVol;
}

/* 出力プラグインを開放 */
static void FreeOutputPlugins(){
	struct OUTPUT_PLUGIN_NODE *pList = pOutputPluginList;
	while (pList) {
		struct OUTPUT_PLUGIN_NODE *pNext = pList->pNext;
		FreeLibrary(pList->hDll);
		HFree(pList);
		pList = pNext;
	}
	pOutputPluginList = NULL;
}

/* 出力プラグインを登録 */
static BOOL CALLBACK RegisterOutputPlugin(HMODULE hPlugin, LPVOID user){
	FARPROC lpfnProc = GetProcAddress(hPlugin, "GetOPluginInfo");
	if (lpfnProc){
		struct OUTPUT_PLUGIN_NODE *pNewNode = (struct OUTPUT_PLUGIN_NODE *)HAlloc(sizeof(struct OUTPUT_PLUGIN_NODE));
		if (pNewNode) {
			pNewNode->pInfo = ((GetOPluginInfoFunc)lpfnProc)();
			pNewNode->pNext = NULL;
			pNewNode->hDll = hPlugin;
			if (pNewNode->pInfo){
				if (pOutputPluginList) {
					struct OUTPUT_PLUGIN_NODE *pList;
					for (pList = pOutputPluginList; pList->pNext; pList = pList->pNext);
					pList->pNext = pNewNode;
				} else {
					pOutputPluginList = pNewNode;
				}
				return TRUE;
			}
			HFree(pNewNode);
		}
	}
	return FALSE;
}

static BOOL CALLBACK OPCBIsEndCue(void){
	BOOL fRet = m_bCueEnd;
	return fRet;
}

static void OPResetOnPlayNextBySystem(void){
	m_fOnSetupNext = FALSE;
}

static void CALLBACK OPCBPlayNext(HWND hWnd){
	if (!m_fOnSetupNext) {
		m_fOnSetupNext = TRUE;
		m_bCueEnd = FALSE;
		PostMessage(hWnd, WM_F4B24_IPC, WM_F4B24_INTERNAL_PLAY_NEXT, 0);
	}
}

static DWORD CALLBACK OPCBGetDecodeChannel(float *pGain) {
	CHANNELINFO *pCh = &g_cInfo[g_bNow];
	*pGain = pCh->sAmp;
	return pCh->hChan;
}

#if DEFAULT_DEVICE_ENABLE
static DWORD m_hOutDefault = NULL;	// ストリームハンドル
static HWND m_hwndDefault = NULL;

static int Default_Init(HWND h){
	m_hwndDefault = h;
	return -1;
}

static int Default_GetStatus(){
	if (m_hOutDefault) {
		int nActive = BASS_ChannelIsActive(m_hOutDefault);
		switch (nActive) {
		case BASS_ACTIVE_PAUSED:
			return OUTPUT_PLUGIN_STATUS_PAUSE;
		case BASS_ACTIVE_PLAYING:
		case BASS_ACTIVE_STALLED:
			return OUTPUT_PLUGIN_STATUS_PLAY;
		}
	}
	return OUTPUT_PLUGIN_STATUS_STOP;
}
// メインストリームプロシージャ
static DWORD CALLBACK Default_StreamProc(DWORD handle, void *buf, DWORD len, void *user){
	DWORD r;
	BASS_CHANNELINFO bci;
	DWORD hChan = g_cInfo[g_bNow].hChan;
	if(BASS_ChannelGetInfo(handle ,&bci) && BASS_ChannelIsActive(hChan)){
		BOOL fFloat = bci.flags & BASS_SAMPLE_FLOAT;
		r = BASS_ChannelGetData(hChan, buf, fFloat ? len | BASS_DATA_FLOAT : len);
		if (r == (DWORD)-1) r = 0;
		if(OPCBIsEndCue() || !BASS_ChannelIsActive(hChan)){
			OPCBPlayNext(m_hwndDefault);
		}
	}else{
		OPCBPlayNext(m_hwndDefault);
		r = BASS_STREAMPROC_END;
	}
	return r;
}

static void Default_End(void){
	if (m_hOutDefault) {
		SendMessage(m_hwndDefault, WM_F4B24_IPC, WM_F4B24_HOOK_FREE_STREAM, (LPARAM)m_hOutDefault);
		BASS_StreamFree(m_hOutDefault);
		m_hOutDefault = 0;
	}
}

static void Default_Play(void){
	if (m_hOutDefault) {
		BASS_ChannelPlay(m_hOutDefault, FALSE);
	}
}

static void Default_SetVolume(float sVolume){
	if (m_hOutDefault) {
		BASS_ChannelSetAttribute(m_hOutDefault, BASS_ATTRIB_VOL, sVolume);
	}
}

static void Default_Start(BASS_CHANNELINFO *info, float sVolume, BOOL fFloat){
	DWORD dwFlag = info->flags & ~(BASS_STREAM_DECODE | BASS_MUSIC_DECODE);
	if (fFloat) dwFlag = (dwFlag & ~BASS_SAMPLE_8BITS) | BASS_SAMPLE_FLOAT;

	Default_End();

	m_hOutDefault = BASS_StreamCreate(info->freq, info->chans, dwFlag, &Default_StreamProc, 0);

	if (m_hOutDefault){
		SendMessage(m_hwndDefault, WM_F4B24_IPC, WM_F4B24_HOOK_CREATE_STREAM, (LPARAM)m_hOutDefault);
	}

	Default_SetVolume(sVolume);
	Default_Play();
}

static void Default_Pause(void){
	if (m_hOutDefault) {
		BASS_ChannelPause(m_hOutDefault);
	}
}

static void Default_Stop(void){
	if (m_hOutDefault) {
		BASS_ChannelStop(m_hOutDefault);
	}
}

static void Default_FadeIn(float sVolume, DWORD dwTime){
	if (m_hOutDefault) {
		BASS_ChannelSlideAttribute(m_hOutDefault, BASS_ATTRIB_VOL, sVolume, dwTime);
		while (BASS_ChannelIsSliding(m_hOutDefault, BASS_ATTRIB_VOL)) Sleep(1);
	}
}
static void Default_FadeOut(DWORD dwTime){
	if (m_hOutDefault) {
		BASS_ChannelSlideAttribute(m_hOutDefault, BASS_ATTRIB_VOL, 0.0f, dwTime);
		while (BASS_ChannelIsSliding(m_hOutDefault, BASS_ATTRIB_VOL)) Sleep(1);
	}
}

// 音声出力デバイスが浮動小数点出力をサポートしているか調べる
static BOOL Default_IsSupportFloatOutput(void){
	DWORD floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, 0);
	if (floatable){
		BASS_StreamFree(floatable);
		return TRUE;
	}
	return FALSE;
}

#define NULLDRV_Init(h) Default_Init(h)
#define NULLDRV_GetStatus Default_GetStatus()
#define NULLDRV_Start(i, v, f) Default_Start(i, v, f)
#define NULLDRV_End Default_End()
#define NULLDRV_Play Default_Play()
#define NULLDRV_Pause Default_Pause()
#define NULLDRV_Stop Default_Stop()
#define NULLDRV_SetVolume(v) Default_SetVolume(v)
#define NULLDRV_FadeIn(v, t) Default_FadeIn(v, t)
#define NULLDRV_FadeOut(t) Default_FadeOut(t)
#define NULLDRV_IsSupportFloatOutput Default_IsSupportFloatOutput()
#else
#define NULLDRV_Init(h) 0
#define NULLDRV_GetStatus OUTPUT_PLUGIN_STATUS_STOP
#define NULLDRV_Start(i, v, f)
#define NULLDRV_End
#define NULLDRV_Play
#define NULLDRV_Pause
#define NULLDRV_Stop
#define NULLDRV_SetVolume(v)
#define NULLDRV_FadeIn(v, t)
#define NULLDRV_FadeOut(t)
#define NULLDRV_IsSupportFloatOutput FALSE
#endif

static int OPInit(OUTPUT_PLUGIN_INFO *pPlugin, DWORD dwId, HWND hWnd){
	m_pOutputPlugin = pPlugin;
	m_pOutputPlugin->hWnd = hWnd;
	m_pOutputPlugin->IsEndCue = OPCBIsEndCue;
	m_pOutputPlugin->PlayNext = OPCBPlayNext;
	m_pOutputPlugin->GetDecodeChannel = OPCBGetDecodeChannel;
	return m_pOutputPlugin->Init(dwId); 
}

static int InitOutputPlugin(HWND hWnd){
	WAEnumPlugins(NULL, "Plugins\\fop\\", "*.fop", RegisterOutputPlugin, hWnd);

	if (pOutputPluginList){
		struct OUTPUT_PLUGIN_NODE *pList = pOutputPluginList;
		DWORD dwDefaultDevice = 0xffffffff;
		OUTPUT_PLUGIN_INFO *pDefaultPlugin = 0;
		do {
			int i;
			OUTPUT_PLUGIN_INFO *pPlugin = pList->pInfo;
			DWORD dwDefaultId = pPlugin->GetDefaultDeviceID();
			if (dwDefaultDevice > dwDefaultId)
			{
				dwDefaultDevice = dwDefaultId;
				pDefaultPlugin = pPlugin;
			}
			int n = pPlugin->GetDeviceCount();
			for (i = 0; i < n; i++) {
				int nId = pPlugin->GetDeviceID(i);
				if (g_cfg.nOutputDevice != 0 && nId == g_cfg.nOutputDevice) {
					return OPInit(pPlugin, nId, hWnd); 
				}
			}
			pList = pList->pNext;
		} while (pList);
		if (pDefaultPlugin) {
			return OPInit(pDefaultPlugin, dwDefaultDevice, hWnd); 
		}
	}
	return NULLDRV_Init(hWnd);
}

static void OPTerm(){
	if (m_pOutputPlugin) m_pOutputPlugin->Term();
}

static int OPGetRate(){
	/*
		デバイスから取得しようかとも考えたが上手く動作しなかった
	*/
	return g_cfg.nOutputRate;
}

static BOOL OPSetup(HWND hWnd){
	if (m_pOutputPlugin) return m_pOutputPlugin->Setup(hWnd);
	return FALSE;
}

static int OPGetStatus(){
	return m_pOutputPlugin ? m_pOutputPlugin->GetStatus() : NULLDRV_GetStatus;
}

static void OPStart(BASS_CHANNELINFO *info, DWORD dwVol, BOOL fFloat){
	float sVolume = CalcBassVolume(dwVol);
	if (m_pOutputPlugin)
		m_pOutputPlugin->Start(info, sVolume, fFloat);
	else
		NULLDRV_Start(info, sVolume, fFloat);
}

static void OPEnd(){
	if (m_pOutputPlugin)
		m_pOutputPlugin->End();
	else
		NULLDRV_End;
}

static void OPPlay(){
	if (m_pOutputPlugin)
		m_pOutputPlugin->Play();
	else
		NULLDRV_Play;
}

static void OPPause(){
	if (m_pOutputPlugin)
		m_pOutputPlugin->Pause();
	else
		NULLDRV_Pause;
}

static void OPStop(){
	if (m_pOutputPlugin)
		m_pOutputPlugin->Stop();
	else
		NULLDRV_Stop;
}
static void OPSetVolume(int iVol){
	float sVolume = CalcBassVolume(iVol < 0 ? 0 : iVol);
#if defined(_MSC_VER) && defined(_DEBUG)
	char buf[128];
#if _MSC_VER >= 1500
	_snprintf_s(buf, 128, "%d - %f\n", iVol, sVolume);
#else
	sprintf(buf, "%d - %f\n", dwVol, (double)sVolume);
#endif
	OutputDebugStringA(buf);
#endif
	if (m_pOutputPlugin)
		m_pOutputPlugin->SetVolume(sVolume);
	else
		NULLDRV_SetVolume(sVolume);
}

static void OPSetFadeIn(DWORD dwVol, DWORD dwTime){
	if(g_cfg.nFadeOut){
		float sVolume = CalcBassVolume(dwVol);
		if (m_pOutputPlugin)
			m_pOutputPlugin->FadeIn(sVolume, dwTime);
		else
			NULLDRV_FadeIn(sVolume, dwTime);
	}
	OPSetVolume(dwVol);
}

static void OPSetFadeOut(DWORD dwTime){
	if(g_cfg.nFadeOut){
		if (m_pOutputPlugin)
			m_pOutputPlugin->FadeOut(dwTime);
		else
			NULLDRV_FadeOut(dwTime);
	}
}

static BOOL OPIsSupportFloatOutput(){
	if (m_pOutputPlugin)
		return m_pOutputPlugin->IsSupportFloatOutput();
	return NULLDRV_IsSupportFloatOutput;
}

#endif
