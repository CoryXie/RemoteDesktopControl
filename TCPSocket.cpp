// Remote Desktop System - remote controlling of multiple PC's
// Copyright (C) 2000-2009 GravyLabs LLC

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// TCPSocket.cpp : implementation file
// Implementation of AsyncSocket derived class.
// This class only works with versions of windows that recognize the TransmitPackets API function call
//

#include "stdafx.h"
#include "TCPSocket.h"
#include "Packet.h"

CTCPSocket::CTCPSocket() : baseclass(),
	m_bListening(FALSE),m_hWndParent(NULL),TransmitPackets(NULL)
{
}

CTCPSocket::~CTCPSocket()
{
}

// Initialize the transmit packet function pointer
BOOL CTCPSocket::InitTP()
{
	// Create the transmit packet function pointer
	DWORD len;
	static const GUID guid = WSAID_TRANSMITPACKETS;
	if (WSAIoctl(m_hSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,(void*)&guid,sizeof(guid),&TransmitPackets,sizeof(TransmitPackets),&len,NULL,NULL))
		return FALSE;
	return TRUE;
}

// Create a new socket
BOOL CTCPSocket::Create(int nSocketPort)
{
	BOOL bCreate = FALSE;
	if (nSocketPort == 0)
	{
		// Create the outbound connection
		bCreate = baseclass::Create();

		// Receive notifications for connections, reading data, and disconnections
		AsyncSelect(FD_CONNECT | FD_READ | FD_CLOSE);
	}
	else
	{
		// Create the listener socket
		bCreate = baseclass::Create(nSocketPort);

		// Receive notifications of a waiting connection, reading data, and disconnections
		AsyncSelect(FD_ACCEPT | FD_READ | FD_CLOSE);

		// Start listening
		if (bCreate)
			m_bListening = baseclass::Listen(SOMAXCONN);
	}

	// Create the transmit packet function pointer
	if (!InitTP())
		bCreate = FALSE;

	return bCreate;
}

// Set the parent window handle for events to be posted back to the parent with WM_APP messags
void CTCPSocket::SetParent(HWND hWndParent)
{
	m_hWndParent = hWndParent;
}

// Make a connection using a outbound socket
BOOL CTCPSocket::Connect(LPCTSTR lpszHostAddress,UINT nHostPort)
{
	if (!m_bListening)
		return baseclass::Connect(lpszHostAddress,nHostPort);
	return FALSE;
}

// Shutdown the socket
BOOL CTCPSocket::ShutDown(int nHow)
{
	return baseclass::ShutDown(nHow);
}

// Notification of a pending inbound connection
void CTCPSocket::OnAccept(int nErrorCode)
{
	ASSERT(m_bListening == TRUE);
	baseclass::OnAccept(nErrorCode);

	// Send the custom WM_ACCEPTCONN message
	if (m_hWndParent != NULL)
	{
		// Send and wait for processing
		SendMessage(m_hWndParent,WM_ACCEPTCONN,(WPARAM)this,0);
	}
}

// Notification that the connection has been successfully made
void CTCPSocket::OnConnect(int nErrorCode)
{
	baseclass::OnConnect(nErrorCode);

	// Send the custom WM_MAKECONN message
	if (m_hWndParent != NULL)
	{
		// Send and wait for processing
		SendMessage(m_hWndParent,WM_MAKECONN,(WPARAM)this,(LPARAM)nErrorCode);
	}
}

// Notification of readiness to receive data
void CTCPSocket::OnReceive(int nErrorCode)
{
	ASSERT(m_bListening == FALSE);
	baseclass::OnReceive(nErrorCode);

	// Send the custom WM_RECEIVEDATA message
	if (m_hWndParent != NULL)
	{
		// Send and wait for processing
		SendMessage(m_hWndParent,WM_RECEIVEDATA,(WPARAM)this,0);
	}
}

// Notification that the connection has been closed
void CTCPSocket::OnClose(int nErrorCode)
{
	baseclass::OnClose(nErrorCode);

	// Shutdown and close the socket
	ShutDown();
	Close();

	// Send the custom WM_CLOSECONN message
	if (m_hWndParent != NULL)
	{
		// Send and wait for processing
		SendMessage(m_hWndParent,WM_CLOSECONN,(WPARAM)this,0);
	}
}

// Helper for sorting, since order really doesn't matter, use the memory address
bool CTCPSocket::operator < (const CTCPSocket & rhs)
{
	return (DWORD_PTR)this < (DWORD_PTR)(&rhs);
}

// Helper to send a packet of data
CTCPSocket & CTCPSocket::operator << (CPacket & rhs)
{
	// Enable blocking mode and prevent events
	DWORD dwArgument = 0;
	AsyncSelect(dwArgument);
	IOCtl(FIONBIO,&dwArgument);

	// Send the packet
	rhs.SerializePacket(this,true);

	// Disable blocking mode and reinstate events
	dwArgument = FD_CONNECT | FD_READ | FD_CLOSE;
	IOCtl(FIONBIO,&dwArgument);
	AsyncSelect(dwArgument);

	// Return a reference to "this" class for chaining
	return *this;
}

// Helper to receive a packet
CTCPSocket & CTCPSocket::operator >> (CPacket & rhs)
{
	// Enable blocking mode and prevent events
	DWORD dwArgument = 0;
	AsyncSelect(dwArgument);
	IOCtl(FIONBIO,&dwArgument);

	// Receive the packet
	rhs.SerializePacket(this,false);

	// Disable blocking mode and reinstate events
	dwArgument = FD_CONNECT | FD_READ | FD_CLOSE;
	IOCtl(FIONBIO,&dwArgument);
	AsyncSelect(dwArgument);

	// Return a reference to "this" class for chaining
	return *this;
}

// Send the buffer of data
int CTCPSocket::Send(const void * lpBuf,int nBufLen,int nFlags)
{
	// Test for a function pointer to the TransmitPackets function
	if (!TransmitPackets)
		return 0;

	// Create the transmit buffer data structure
	_TRANSMIT_PACKETS_ELEMENT TPE;
	TPE.dwElFlags = TP_ELEMENT_MEMORY;
	TPE.cLength = nBufLen;
	TPE.pBuffer = (PVOID)lpBuf;

	// Transmit the packet
	BOOL bTransmit = TransmitPackets(m_hSocket,&TPE,1,nBufLen,NULL,TF_USE_KERNEL_APC);
	return nBufLen;
}

// Receive the buffer of data
int CTCPSocket::Receive(void * lpBuf,int nBufLen,int nFlags)
{
	int nTotalRecv = 0,nRecvAttempt = 0;
	while (nTotalRecv < nBufLen)
	{
		int nRecv = baseclass::Receive((LPSTR)lpBuf + nTotalRecv,nBufLen - nTotalRecv,nFlags);
		if (nRecv == SOCKET_ERROR)
			return nRecv;
		nTotalRecv += nRecv;

		// Test for a dropped connection during a receive
		if (nRecv == 0)
		{
			nRecvAttempt++;
			Sleep(300);
		}
		if (nRecvAttempt == 10)
			return 0;
	}
	return nTotalRecv;
}

/////////////////////////////////////////////////////////////////////////////////

// Handle the socket notifications as well as the basic initialization
BEGIN_MESSAGE_MAP(CSocketWndSink,CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_MESSAGE(WM_MAKECONN, &CSocketWndSink::OnMakeConn)
	ON_MESSAGE(WM_RECEIVEDATA, &CSocketWndSink::OnReceiveData)
	ON_MESSAGE(WM_CLOSECONN, &CSocketWndSink::OnCloseConn)
END_MESSAGE_MAP()

// Constructor
CSocketWndSink::CSocketWndSink(HWND hWndParent) : CWnd(), m_hWndParent(hWndParent), m_bConnected(FALSE)
{
	// Create the window
	if (!CreateEx(0,AfxRegisterWndClass(0),_T("RDVSink"),WS_OVERLAPPED,0,0,0,0,NULL,NULL))
		DebugMsg("%s\n",TEXT("Failed to create a RDV window notification sink"));
}

// Destructor
CSocketWndSink::~CSocketWndSink()
{
	if (m_bConnected)
		ShutDown();
}

// Create the window
int CSocketWndSink::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	MoveWindow(0,0,0,0);
	return 0;
}

// Paint the window
void CSocketWndSink::OnPaint()
{
	CPaintDC dc(this);
}

// Return the connected status
BOOL CSocketWndSink::IsConnected()
{
	return m_bConnected;
}

// Shutdown and close the connection
void CSocketWndSink::ShutDown()
{
	m_Socket.ShutDown();
	m_Socket.Close();
	m_bConnected = FALSE;
}

// Connection has been made
LRESULT CSocketWndSink::OnMakeConn(WPARAM wParam,LPARAM lParam)
{
	int nErrorCode = (int)lParam;
	if (nErrorCode)
	{
		// Get the error message
		CString csErrorMsg;
		switch( nErrorCode )
		{
			case WSAEADDRINUSE:
			{
				csErrorMsg = "The specified address is in use.\n";
				break;
			}
			case WSAEADDRNOTAVAIL:
			{
				csErrorMsg = "The specified address is not available from the local machine.\n";
				break;
			}
			case WSAEAFNOSUPPORT:
			{
				csErrorMsg = "Addresses in the specified family cannot be used with this socket.\n";
				break;
			}
			case WSAEDESTADDRREQ:
			{
				csErrorMsg = "A destination address is required.\n";
				break;
			}
			case WSAEFAULT:
			{
				csErrorMsg = "The lpSockAddrLen argument is incorrect.\n";
				break;
			}
			case WSAEINVAL:
			{
				csErrorMsg = "The socket is already bound to an address.\n";
				break;
			}
			case WSAEISCONN:
			{
				csErrorMsg = "The socket is already connected.\n";
				break;
			}
			case WSAEMFILE:
			{
				csErrorMsg = "No more file descriptors are available.\n";
				break;
			}
			case WSAENETUNREACH:
			{
				csErrorMsg = "The network cannot be reached from this host at this time.\n";
				break;
			}
			case WSAENOBUFS:
			{
				csErrorMsg = "No buffer space is available. The socket cannot be connected.\n";
				break;
			}
			case WSAENOTCONN:
			{
				csErrorMsg = "The socket is not connected.\n";
				break;
			}
			case WSAENOTSOCK:
			{
				csErrorMsg = "The descriptor is a file, not a socket.\n";
				break;
			}
			case WSAETIMEDOUT:
			{
				csErrorMsg = "The attempt to connect timed out without establishing a connection. \n";
				break;
			}
			case WSAECONNREFUSED:
			default:
			{
				csErrorMsg = "The attempt to connect was forcefully rejected.\n";
				break;
			}
		}

		// Notify
		AfxMessageBox(csErrorMsg);
	}
	else
	{
		// Set the connected state and connection event
		m_bConnected = TRUE;

#if defined(_DEBUG)
		DebugMsg("Connected to the server on thread %d\n",GetCurrentThreadId());
#endif
	}
	return lParam;
}

// Received data event from the desktop server, now read the data
LRESULT CSocketWndSink::OnReceiveData(WPARAM wParam,LPARAM lParam)
{
	// Receive the data
	CPacket Packet;

	// Receive the image
	m_Socket >> Packet;

	// Send the packet and wait for it to be processed
	::SendMessage(m_hWndParent,WM_RECEIVEDATA,(WPARAM)this,(LPARAM)&Packet);

	return 1;
}

// Connection telling us that a connection has been closed
LRESULT CSocketWndSink::OnCloseConn(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

// Make a connection to the server
void CSocketWndSink::Connect(CString csIp,CString csPort)
{
	// Create the connection oriented socket
	m_Socket.Create();

	// Set the parent for receiving socket notifications
	m_Socket.SetParent(GetSafeHwnd());

	// Attempt to connect to the server
	m_Socket.Connect(csIp,atoi(csPort));
}

/////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CSocketWndSinkThread, CWinThread)

CSocketWndSinkThread::CSocketWndSinkThread() throw(...)
{
	throw;
}

// Take the update rectangle
CSocketWndSinkThread::CSocketWndSinkThread(HANDLE * phWork,HWND hWndParent,CString csIp,CString csPort) : 
	m_phWork(phWork), m_hWndParent(hWndParent), m_csIp(csIp), m_csPort(csPort), m_pSocketWnd(NULL), m_bPumpMessage(false)
{
	// Create the initialization signaling event
	m_hPumpMessage = CreateEvent(NULL,FALSE,FALSE,NULL);

	// Create the work signaling event
	HANDLE & hWork = *m_phWork;
	hWork = CreateEvent(NULL,FALSE,FALSE,NULL);

	// Initialize the event
	SetEvent(hWork);

	// Create the thread
	CreateThread();
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	// Wait for the thread to initialize and signal its message pump has started
	WaitForSingleObject(m_hPumpMessage,INFINITE);

	// Close the initialization event
	CloseHandle(m_hPumpMessage);
	m_hPumpMessage = NULL;
}

CSocketWndSinkThread::~CSocketWndSinkThread()
{
	// Close the work signaling event
	HANDLE & hWork = *m_phWork;
	CloseHandle(hWork);
}

BOOL CSocketWndSinkThread::InitInstance()
{
	// Initialize sockets for each thread
	if (!AfxSocketInit())
		return FALSE;

	// Create the window for socket event notifications
	m_pSocketWnd = new CSocketWndSink(m_hWndParent);
	ASSERT(m_pSocketWnd != NULL);

#if defined(_DEBUG)
	DebugMsg("Thread %d ready\n",GetCurrentThreadId());
#endif

	return TRUE;
}

int CSocketWndSinkThread::ExitInstance()
{
	// Delete the socket window event notification window
	if (m_pSocketWnd)
	{
		// Shutdown the connection if it is still open
		if (m_pSocketWnd->IsConnected())
			m_pSocketWnd->ShutDown();

		// Destroy the window
		m_pSocketWnd->DestroyWindow();

		// Delete the object
		delete m_pSocketWnd;
		m_pSocketWnd = NULL;
	}

	// Base class handles normal exit sequence
	return CWinThread::ExitInstance();
}

// Use the message pump as a marker to indicate the thread can accept posted messages
BOOL CSocketWndSinkThread::PumpMessage()
{
	// Test for initialization
	if (!m_bPumpMessage)
	{
		// The message pump is active
		m_bPumpMessage = true;

		// Signal completion of this stage
		SetEvent(m_hPumpMessage);
	}

	// Allow the base class to handle the message pump
	return CWinThread::PumpMessage();
}

// Return the connected status
BOOL CSocketWndSinkThread::IsConnected()
{
	return m_pSocketWnd && m_pSocketWnd->IsConnected();
}

// These functions are called by PostThreadMessage
BEGIN_MESSAGE_MAP(CSocketWndSinkThread, CWinThread)
	ON_THREAD_MESSAGE(WM_CONNECTSERVER, &CSocketWndSinkThread::OnConnectServer)
	ON_THREAD_MESSAGE(WM_ENDTHREAD, &CSocketWndSinkThread::OnEndThread)
END_MESSAGE_MAP()

// Connect to the server
void CSocketWndSinkThread::OnConnectServer(WPARAM wParam,LPARAM lParam)
{
	// Create the connection event
	if (m_pSocketWnd)
		m_pSocketWnd->Connect(m_csIp,m_csPort);
}

void CSocketWndSinkThread::OnEndThread(WPARAM wParam,LPARAM lParam)
{
	// Shutdown and close the connection
	if (IsConnected())
		m_pSocketWnd->ShutDown();

	// End the thread
	PostQuitMessage(0);
}

//////////////////////////////////////////////////////////////////////////////////

// Handle the socket notifications as well as the basic initialization
BEGIN_MESSAGE_MAP(CSocketEvent,CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_MESSAGE(WM_RECEIVEDATA, &CSocketEvent::OnReceiveData)
	ON_MESSAGE(WM_CLOSECONN, &CSocketEvent::OnCloseConn)
END_MESSAGE_MAP()

// Constructor
CSocketEvent::CSocketEvent(CWinThread * pThread) : CWnd(), m_pThread(pThread), m_bClosed(FALSE)
{
	// Create the window
	if (!CreateEx(0,AfxRegisterWndClass(0),_T("RDSSink"),WS_OVERLAPPED,0,0,0,0,NULL,NULL))
		DebugMsg("%s\n",TEXT("Failed to create a RDS window notification sink"));
}

// Destructor
CSocketEvent::~CSocketEvent()
{
}

// Return the closed status of the socket
BOOL CSocketEvent::IsClosed()
{
	// A valid socket pointer means it received the closed event notification
	return m_bClosed;
}

// Create the window
int CSocketEvent::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	MoveWindow(0,0,0,0);
	return 0;
}

// Paint the window
void CSocketEvent::OnPaint()
{
	CPaintDC dc(this);
}

// The socket has data to be read
LRESULT CSocketEvent::OnReceiveData(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

// The socket has been closed
LRESULT CSocketEvent::OnCloseConn(WPARAM wParam,LPARAM lParam)
{
	// Update the closed status
	m_bClosed = TRUE;

	// Notify the parent
	m_pThread->PostThreadMessage(WM_CLOSECONN,wParam,lParam);

	return 1;
}