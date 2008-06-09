#ifndef BLOCKINGCLIENT_H
#define BLOCKINGCLIENT_H

#include <winsock2.h>

#include <string>

/*************************************************************************/ /**
    Sockets-based client for sending and receiving commands down a socket
    using Winsock2.
******************************************************************************/
class BlockingClient
{
public:

    /*************************************************************************
        Exception class for when things go wrong.
    **************************************************************************/
    class NetworkException : public std::logic_error {
    public:
        NetworkException(const std::string& sMsg) : logic_error(sMsg) { }
    };

    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
	BlockingClient();

    /*********************************************************************/ /**
        Dtor
    **************************************************************************/
	virtual
	~BlockingClient();

    /*********************************************************************/ /**
        Connect to server.
        
        @param[in] sHost The host to connect to.
        @param[in] nPort The port on the host to connect to.
        
        @return The port number connected to. Port stepping will be used
                if the requested port number is taken.
    **************************************************************************/
    int
    Connect(
        const std::string& sHost,
        u_short            nPort);

    /*********************************************************************/ /**
        Send a string down the socket.

        @param[in] sData The data to send.
    **************************************************************************/
    void
    Send(
        const std::string& sData);

    /*********************************************************************/ /**
        Read a line from the socket.

        @param[out] sLine String the received line is written to.
    **************************************************************************/
    void
    Receive(
        std::string& sLine);

    /*********************************************************************/ /**
        Shut down connection gracefully.
    **************************************************************************/
    void
    ShutDown();

private:
    
    /*********************************************************************/ /**
        Given an address string, determine if it's a dotted-quad IP address
        or a domain address. If the latter, ask DNS to resolve it. In either
        case, return resolved IP address. Throws CNetworkException on fail.
    **************************************************************************/
    u_long
    LookUpAddress(
        const std::string& sHost);

    /*********************************************************************/ /**
        Connects to a given address, on a given port, both of which must be
        in network byte order. Returns newly-connected socket if we succeed,
        or INVALID_SOCKET if we fail.
    **************************************************************************/
    SOCKET
    ConnectToSocket(
        u_long  nRemoteAddr,
        u_short nPort);


    SOCKET mSocket;

    // Receives the actual port number of last connection attempt in case of
    // port stepping
    int    mnLastPort;

    bool   mbDoPortstepping;
};

#endif // BLOCKINGCLIENT_H
