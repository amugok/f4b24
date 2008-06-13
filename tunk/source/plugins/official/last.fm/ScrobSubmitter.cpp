#include "ScrobSubmitter.h"
#include "EncodingUtils.h"

#include <windows.h>
#include <process.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/******************************************************************************
    Constants
******************************************************************************/
static const char* kVersion = "0.9.1";
static const char* kHost = "localhost";
static const int   kDefaultPort = 33367;
static const int   kLaunchWait = 60000; // in ms

/******************************************************************************
    ScrobSubmitter
******************************************************************************/
ScrobSubmitter::ScrobSubmitter() :
    mActualPort(kDefaultPort),
    mNextId(0),
    mStopThread(false)
{
    InitializeCriticalSection(&mMutex);
    
    mRequestAvailable = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual reset event
        FALSE,        // initial state is non-signaled
        NULL);        // object name

    mExit = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual reset event
        FALSE,        // initial state is non-signaled
        NULL);        // object name
}

/******************************************************************************
   ~ScrobSubmitter
******************************************************************************/
ScrobSubmitter::~ScrobSubmitter()
{
    // Not using dtor for thread termination as we can't be sure the client
    // is still alive so the status callbacks might fail.

    DeleteCriticalSection(&mMutex);

    CloseHandle(mRequestAvailable);
    CloseHandle(mExit);
}

/******************************************************************************
   Term
******************************************************************************/
void
ScrobSubmitter::Term()
{
    // Signal socket thread to exit
    mStopThread = true;
    SetEvent(mExit);

    // Wait for it to die before leaving
    WaitForSingleObject(mSocketThread, INFINITE);
    
    CloseHandle(mSocketThread);
}    

/******************************************************************************
    Init
******************************************************************************/
void
ScrobSubmitter::Init(
    const string& pluginId,
    StatusCallback callback,
    void* userData)
{
    mPluginId = pluginId;
    mpReportStatus = callback;
    mpUserData = userData;
        
    // Since AS doesn't allow multiple instances, we can just kick it off
    // regardless of whether it's running or not.

    // Read exe path from registry
    HKEY regKeyAS;
    LONG lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        "Software\\Last.fm\\Client",
        0,              // reserved
        KEY_ALL_ACCESS, // access mask
        &regKeyAS);
    
    if (lResult != ERROR_SUCCESS)
    {
        // AS isn't installed
        ReportStatus(-1, true,
            "Couldn't find Last.fm registry key. "
            "The Last.fm client doesn't seem to be installed.");
        return;
    }
    
    TCHAR acExe[MAX_PATH];
    DWORD nPathSize = MAX_PATH;
    lResult = RegQueryValueEx(
        regKeyAS,                           // key
        "Path",                             // value to query
        NULL,                               // reserved
        NULL,                               // tells you what type the value is
        reinterpret_cast<LPBYTE>(acExe),    // store result here
        &nPathSize);

    if (lResult != ERROR_SUCCESS)
    {
        // Couldn't read Path value
        ReportStatus(-1, true,
            "Couldn't read the Last.fm exe path from the registry.");
        return;
    }

    RegCloseKey(regKeyAS);

    // Work out what the app dir is
    string sPath(acExe);
    string::size_type pos = sPath.find_last_of('\\');
    string sDefaultDir = sPath.substr(0, pos);

    // Launch it
    HINSTANCE h = ShellExecute(
        NULL, "open", acExe, "-tray", sDefaultDir.c_str(), SW_SHOWNORMAL);
        
    if (reinterpret_cast<int>(h) <= 32) // Error
    {
        // Invalid handle means it didn't launch
        ostringstream os;
        os << "Failed launching Last.fm client. ShellExecute error: " << h;
        ReportStatus(-1, true, os.str());
    }
    else
    {
        ReportStatus(-1, false, "Launched Last.fm client.");
    }
    
    // Store time of launch
    mLaunchTime = GetTickCount();

    // Start socket thread
    unsigned threadId;

    mSocketThread = reinterpret_cast<HANDLE>(_beginthreadex( 
        NULL,                           // security crap
        0,                              // stack size
        SendToASThreadMain,             // start function
        reinterpret_cast<void*>(this),  // argument to thread
        0,                              // run straight away
        &threadId));                    // thread id out
    
    if (mSocketThread == 0) // Error
    {
        ReportStatus(-1, true, strerror(errno));
    }

}

/******************************************************************************
    Start
******************************************************************************/
int
ScrobSubmitter::Start(
    string artist,
    string track,
    string album,
    string mbId,
    int    length,
    string filename,
    Encoding encoding)
{
    ConvertToUTF8(artist, encoding);
    ConvertToUTF8(track, encoding);
    ConvertToUTF8(album, encoding);
    ConvertToUTF8(mbId, encoding);
    ConvertToUTF8(filename, encoding);

    ostringstream osCmd;
    osCmd << "START c=" << mPluginId        << "&" <<
                   "a=" << Escape(artist)   << "&" <<
                   "t=" << Escape(track)    << "&" <<
                   "b=" << Escape(album)    << "&" <<
                   "m=" << Escape(mbId)     << "&" <<
                   "l=" << length           << "&" <<
                   "p=" << Escape(filename) << endl;

    string cmd = osCmd.str();

    return SendToAS(cmd);
}

/******************************************************************************
    Stop
******************************************************************************/
int
ScrobSubmitter::Stop()
{
    ostringstream osCmd;
    osCmd << "STOP c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

/******************************************************************************
    Pause
******************************************************************************/
int
ScrobSubmitter::Pause()
{
    ostringstream osCmd;
    osCmd << "PAUSE c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

/******************************************************************************
    Resume
******************************************************************************/
int
ScrobSubmitter::Resume()
{
    ostringstream osCmd;
    osCmd << "RESUME c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

/******************************************************************************
    GetVersion
******************************************************************************/
string
ScrobSubmitter::GetVersion()
{
    return kVersion;
}

/******************************************************************************
    SendToAS
******************************************************************************/
int
ScrobSubmitter::SendToAS(
    const std::string& cmd)
{
    if (mPluginId == "")
    {
        ReportStatus(-1, true,
            "Init hasn't been called with a plugin ID");
        return -1;
    }

    EnterCriticalSection(&mMutex);

    // Push the sCmd on the queue
    mRequestQueue.push_back(make_pair(++mNextId, cmd));

    // If queue was empty, signal socket thread
    if (mRequestQueue.size() == 1)
    {
        SetEvent(mRequestAvailable);
    }
    
    LeaveCriticalSection(&mMutex);
    
    return mNextId;
}

/******************************************************************************
    SendToASThread
******************************************************************************/
void
ScrobSubmitter::SendToASThread()
{
    // By giving mRequestAvailable the lower index, it will take priority
    // when calling WaitForMultipleObjects and both events are signalled.
    HANDLE eventArray[2] = { mRequestAvailable, mExit };

    while (!mStopThread)
    {
        DWORD signalledHandle = WaitForMultipleObjects(
            2, eventArray, FALSE, INFINITE);
        
        // Check if it's the exit event
        if ((signalledHandle - WAIT_OBJECT_0) == 1) { continue; }

        EnterCriticalSection(&mMutex);

        // Pick first request from queue
        pair<int, string> reqPair = mRequestQueue.front();
        mRequestQueue.pop_front();

        // This means we will eat all the requests in the queue before
        // waiting again
        if (mRequestQueue.size() == 0)
        {
            ResetEvent(mRequestAvailable);
        }

        LeaveCriticalSection(&mMutex);
        
        int nId = reqPair.first;
        string sCmd = reqPair.second;
        
        bool connected = ConnectToAS(nId);
        if (!connected) { continue; }
        
        string sResponse;
        try
        {
            mSocket.Send(sCmd);
            mSocket.Receive(sResponse);

            // Can't throw
            mSocket.ShutDown();

            if (sResponse.substr(0, 2) != "OK")
            {
                ReportStatus(nId, true, sResponse);
            }
            else
            {
                ReportStatus(nId, false, sResponse);
            }
        }
        catch (BlockingClient::NetworkException& e)
        {
            // Shut socket down and report error
            mSocket.ShutDown();
            ReportStatus(nId, true, e.what());
        }

    } // end while

    _endthreadex( 0 );
}

/******************************************************************************
    ConnectToAS
******************************************************************************/
bool
ScrobSubmitter::ConnectToAS(
    int reqId)
{
    try
    {
        mActualPort = mSocket.Connect(kHost, mActualPort);
    }
    catch (BlockingClient::NetworkException& e)
    {
        bool isTerminating = mStopThread;
        
        // It might be the case that the app simply hasn't had time to
        // open its socket yet. If so, let's sleep for a bit and retry.
        DWORD nowTime = GetTickCount();
        DWORD timeSinceLaunch = nowTime - mLaunchTime;

        if (!isTerminating && (timeSinceLaunch < kLaunchWait))
        {
            ostringstream os;
            os << "Connect failed, sent command too early after startup. "
                "Time since launch: " << timeSinceLaunch << ". Retrying...";
            ReportStatus(reqId, false, os.str().c_str());

            Sleep(1000);
            return ConnectToAS(reqId);
        }
        else
        {
            ReportStatus(reqId, true, e.what());
            return false;
        }
    }
    
    return true;
}

/******************************************************************************
    ReportStatus
******************************************************************************/
void
ScrobSubmitter::ReportStatus(
    int reqId,
    bool error,
    const string& msg)
{
    if (!mStopThread && (mpReportStatus != NULL))
    {
        (*mpReportStatus)(reqId, error, msg, mpUserData);
    }
}    

/******************************************************************************
    ConvertToUTF8
******************************************************************************/
void
ScrobSubmitter::ConvertToUTF8(
	string&  text,
	Encoding encoding)
{
    if (encoding == UTF_8)
    {
        return;
    }
    
    switch (encoding)
    {
        case ISO_8859_1:
        {
            // A UTF-8 string can be up to 4 times as big as the ANSI
            size_t nUtf8MaxLen = text.size() * 4 + 1;
            char* pcUtf8 = new char[nUtf8MaxLen];
            EncodingUtils::AnsiToUtf8(text.c_str(),
                                      pcUtf8,
                                      static_cast<int>(nUtf8MaxLen));
            text = pcUtf8;
            delete[] pcUtf8;
        }
        break;
        
    }
}	

/******************************************************************************
    Escape
******************************************************************************/
string&
ScrobSubmitter::Escape(
    string& text)
{
    string::size_type nIdx = text.find("&");
    while (nIdx != string::npos)
    {
        text.replace(nIdx, 1, "&&");
        nIdx = text.find("&", nIdx + 2);
    }

    return text;
}    
