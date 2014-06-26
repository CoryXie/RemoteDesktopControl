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

#pragma once

#include <set>
#include "afxsock.h"
#include "Packet.h"
#include "Common.h"

#define WM_ACCEPTCONN (WM_APP + 1)
#define WM_MAKECONN (WM_APP + 2)
#define WM_RECEIVEDATA (WM_APP + 3)
#define WM_CLOSECONN (WM_APP + 4)

// TCP stream socket class
class CTCPSocket : public CAsyncSocket
{
	typedef CAsyncSocket baseclass;
private:
	CTCPSocket(const CTCPSocket & rhs); // no
	void operator=(const CTCPSocket & rhs); // no

public:
	CTCPSocket();
	virtual ~CTCPSocket();

	BOOL Create(int nSocketPort = 0);
	BOOL Connect(LPCTSTR lpszHostAddress,UINT nHostPort);
	void SetParent(HWND hWndParent);
	BOOL ShutDown(int nHow = CSocket::both);
	BOOL InitTP();

	// Helper to send a packet
	CTCPSocket & operator << (CPacket & rhs);

	// Helper to receive a packet
	CTCPSocket & operator >> (CPacket & rhs);

	// Helper for sorting
	bool operator < (const CTCPSocket & rhs);

protected:
	// Low-level socket event callbacks, reposted to parent via WM_APP messages
	virtual void OnAccept(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	// Low-level methods to send and receieve data over the socket
	virtual int Send(const void * lpBuf,int nBufLen,int nFlags = 0);
	virtual int Receive(void * lpBuf,int nBufLen,int nFlags = 0);

private:
	LPFN_TRANSMITPACKETS TransmitPackets;
	BOOL m_bListening;
	HWND m_hWndParent;
};

////////////////////////////////////////////////////////////////////////////
// Window sink message class
class CSocketWndSink : public CWnd
{
// Construction
public:
	CSocketWndSink(HWND hWndParent = NULL);
	~CSocketWndSink();

	void Connect(CString csIp,CString csPort);
	BOOL IsConnected();
	void ShutDown();

// Generated message map functions
protected:
	//{{AFX_MSG(CSocketWndSink)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG

protected:
	// The parent dialog
	HWND m_hWndParent;

	// The client connectiun
	CTCPSocket m_Socket;
	BOOL m_bConnected;

	DECLARE_MESSAGE_MAP()

	// Handled socket event callbacks
	afx_msg LRESULT OnMakeConn(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnReceiveData(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCloseConn(WPARAM wParam,LPARAM lParam);
};

// Connection to the server that receives DIB updates
class CSocketWndSinkThread : public CWinThread
{
	DECLARE_DYNCREATE(CSocketWndSinkThread)

	CSocketWndSinkThread();

public:
	CSocketWndSinkThread(HANDLE * phWork,HWND hWndParent,CString csIp,CString csPort);
	virtual ~CSocketWndSinkThread();
	virtual BOOL InitInstance();
	virtual BOOL PumpMessage();
	virtual int ExitInstance();
	BOOL IsConnected();

// Event synchronization
public:
	HANDLE m_hPumpMessage;

// Intialization
private:
	bool m_bPumpMessage;

protected:
	HANDLE * m_phWork;
	HWND m_hWndParent;
	CString m_csIp;
	CString m_csPort;
	CSocketWndSink * m_pSocketWnd;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnConnectServer(WPARAM wParam,LPARAM lParam);
	afx_msg void OnEndThread(WPARAM wParam,LPARAM lParam);
};

////////////////////////////////////////////////////////////////////////////
// Socket message event sink
class CSocketEvent : public CWnd
{
// Construction
public:
	CSocketEvent(CWinThread * pThread);
	~CSocketEvent();
	BOOL IsClosed();

// Generated message map functions
protected:
	//{{AFX_MSG(CSocketEvent)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG

protected:

	DECLARE_MESSAGE_MAP()

	// Socket events
	afx_msg LRESULT OnReceiveData(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCloseConn(WPARAM wParam,LPARAM lParam);

	// Back pointers
	CWinThread * m_pThread;
	BOOL m_bClosed;
};
