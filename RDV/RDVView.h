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

#include "..\Common.h"
#include "..\TCPSocket.h"
#include "..\DIBFrame.h"
#include "ATLImage.h"
#include <vector>

class CRDVView : public CScrollView
{
protected: // create from serialization only
	CRDVView();
	DECLARE_DYNCREATE(CRDVView)

// Attributes
public:
	CRDVDoc* GetDocument() const;

// Operations
public:
	void ConnectServer();
	void DisconnectServer();

// Overrides
public:
	virtual void OnDraw(CDC * pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual void OnInitialUpdate();
	virtual BOOL DestroyWindow();
	virtual BOOL OnPreparePrinting(CPrintInfo * pInfo);
	virtual void OnBeginPrinting(CDC * pDC, CPrintInfo * pInfo);
	virtual void OnPrint(CDC * pDC,CPrintInfo * pInfo);
	virtual void OnEndPrinting(CDC * pDC, CPrintInfo * pInfo);
	virtual void OnUpdate(CView * pSender,LPARAM lHint,CObject * pHint);

// Implementation
public:
	virtual ~CRDVView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Internal helpers
protected:
	void SetupGDI();
	void CleanupGDI();
	void DeleteSinkThreads();
	void SendMouseMessage(WORD wWM,UINT nFlags,CPoint Point,short zDelta = 0);
	void SendKeyBoardMessage(WORD wWM,UINT nChar,UINT nRepCnt,UINT nFlags);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

	// Handled socket event callbacks
	afx_msg LRESULT OnMakeConn(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnReceiveData(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCloseConn(WPARAM wParam,LPARAM lParam);

	// Capture UI events and send them to the server
	afx_msg void OnLButtonDblClk(UINT nFlags,CPoint Point);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint Point);
	afx_msg void OnLButtonUp(UINT nFlags,CPoint Point);
	afx_msg void OnMButtonDblClk(UINT nFlags,CPoint Point);
	afx_msg void OnMButtonDown(UINT nFlags,CPoint Point);
	afx_msg void OnMButtonUp(UINT nFlags,CPoint Point);
	afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint Point);
	afx_msg void OnMouseMove(UINT nFlags,CPoint Point);
	afx_msg void OnRButtonDblClk(UINT nFlags,CPoint Point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint Point);
	afx_msg void OnRButtonUp(UINT nFlags,CPoint Point);
	afx_msg void OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar,UINT nRepCnt,UINT nFlags);

	// Handle the cursor
	afx_msg BOOL OnSetCursor(CWnd * pWnd,UINT nHitTest,UINT message);

	// Timer for closing the document
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	// For resizing
	afx_msg void OnSize(UINT nType,int cx,int cy);

	// For zooming
	afx_msg void OnViewZoom();
	afx_msg void OnUpdateViewZoom(CCmdUI *pCmdUI);

// Data
protected:
	// The client connectiun
	CTCPSocket m_Socket;

	// Handle arrays
	HANDLE m_arrSinkHandles[MAXTHREADS];
	std::vector<CSocketWndSinkThread *> m_vecSinkThreads;

	// Number of update threads on the server
	int m_nGridThreads;
	int m_iGridThread;

	// Connection
	bool m_bConnected;

	// Dib bit count
	int m_nBitCount;

	// Dib dimensions
	int m_cxWidth;
	int m_cyHeight;
	DWORD m_dwDibSize;

	// Desktop DIB
	CDIBFrame m_DIB;
	CDIBFrame m_XOR;

	// GDI+ Image support
	CImage m_ImageDIB;

	// Cursor
	CCursor m_Cursor;

	// Multithreaded arithmetic encoder/decoder
	int m_nCompThreads;
	CDriveMultiThreadedCompression * m_pMTC;

	// Single threaded compression
	CArithmeticEncoder m_AC;
	CZLib m_ZLib;

	// Queue of keyboard messages
	std::vector<CKBMsg> m_vKBMsg;
	std::vector<CMouseMsg> m_vMouseMsg;

	// For zooming
	CSize m_ViewSize;
	BOOL m_bViewZoom;

	// Session Id
	UINT m_nSessionId;
};

#ifndef _DEBUG  // debug version in RDVView.cpp
inline CRDVDoc* CRDVView::GetDocument() const
   { return reinterpret_cast<CRDVDoc*>(m_pDocument); }
#endif

