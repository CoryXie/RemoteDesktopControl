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

// RDSDlg.h : header file
//

#pragma once

// Socket
#include "..\TCPSocket.h"

// GDI+ images
#include "ATLImage.h"

// STL
#include <map>

// DIB Refresh
class CRefreshThread : public CWinThread
{
	DECLARE_DYNCREATE(CRefreshThread)

	CRefreshThread();

public:
	CRefreshThread(HWND hWndParent,std::vector<CRect> vDIBRect,int nBitCount,BOOL bImgDIB,BOOL bUseCompression,BOOL bAC,DWORD dwThread,int nCompThreads);
	virtual ~CRefreshThread();
	virtual BOOL InitInstance();
	virtual BOOL PumpMessage();
	virtual int ExitInstance();

// Event synchronization
protected:
	HWND m_hWndParent;
	HANDLE m_hEvent;

// Client connection
protected:
	std::set<CTCPSocket *> m_setAccept;
	std::map<CTCPSocket *,CSocketEvent *> m_mapAcceptEvent;

// DIB
protected:
	HDC m_hDisplayDC;
	std::vector<CDIBFrame> m_vDIB,m_vDIBLast;
	std::vector<CRect> m_vDIBRect;
	std::vector<BOOL> m_vComp;
	std::vector<BOOL> m_vInit;
	std::vector<CPacket> m_vDIBPacket;
	DWORD m_dwSleep;
	int m_iCurDIB;
	int m_nDIB;
	int m_nBitCount;

	// Thread and compression settings
	DWORD m_dwThread;
	int m_nCompThreads;
	BOOL m_bImgDIB;
	BOOL m_bUseCompression;
	BOOL m_bAC;

	// Compression output buffer (initially sized to the source size)
	std::auto_ptr<char> m_OutBuffer;

	// GDI+ Image support
	CImage m_ImageDIB;

	// Cursor data
	CPacket m_Cursor;

// Intialization
private:
	bool m_bPumpMessage;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAcceptConn(WPARAM wParam,LPARAM lParam);
	afx_msg void OnImageRefresh(WPARAM wParam,LPARAM lParam);
	afx_msg void OnImageSend(WPARAM wParam,LPARAM lParam);
	afx_msg void OnEndThread(WPARAM wParam,LPARAM lParam);
	afx_msg void OnCloseConn(WPARAM wParam,LPARAM lParam);
};

