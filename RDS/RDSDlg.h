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

// CPU Info
#include <intrin.h>

// Socket
#include "..\TCPSocket.h"

// UI components
#include "..\DIBFrame.h"
#include "..\Common.h"
#include "RefreshThread.h"
#include "TrayIcon.h"

// STL includes
#include <set>
#include <memory>
#include <map>

// MFC includes
#include <afxwin.h>
#include <afxpriv.h>

// CRDSDlg dialog
class CRDSDlg : public CDialog
{
// Construction
public:
	CRDSDlg(CWnd* pParent = NULL);	// standard constructor
	~CRDSDlg();

// Dialog Data
	enum { IDD = IDD_RDS_DIALOG };

	CString m_csPassword;
	int m_nBitCount;
	int m_nCompThreads;
	BOOL m_bAcceptUpdate;
	int m_nIncoming;
	BOOL m_bUseCompression;
	CButton m_Start;
	CButton m_Stop;
	CStatic m_StaticPassword;
	CEdit m_Password;
	CStatic m_StaticPort;
	CEdit m_Port;
	int m_iPort;
	CButton m_Colors;
	CButton m_BW;
	CButton m_LBW;
	CButton m_ImgDIB;
	CButton m_ImgPNG;
	BOOL m_bColors;
	BOOL m_bBW;
	BOOL m_bLBW;
	BOOL m_bImgDIB;
	BOOL m_bImgPNG;
	CButton m_UseCompression;
	CButton m_AC;
	BOOL m_bAC;
	CButton m_ZLib;
	BOOL m_bZLib;
	CEdit m_ACThreads;
	CStatic m_StaticACThreads;
	CStatic m_StaticXScale;
	CEdit m_XScale;
	int m_iXScale;
	CStatic m_StaticYScale;
	CEdit m_YScale;
	int m_iYScale;
	CStatic m_StaticGridThreads;
	CEdit m_GridThreads;
	int m_nGridThreads;
	BOOL m_bStarted;

	// Handle arrays
	std::vector<CRefreshThread *> m_vRefreshThread;
	DWORD m_nThreads;

	// Multithreaded compression
	CDriveMultiThreadedCompression * m_pMTC;

	// Single threaded compression
	CArithmeticEncoder m_AE;
	CZLib m_ZL;

protected:
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Data
protected:

	// Dialog and Tray icons
	HICON m_hIcon;
	CTrayIcon m_TrayIcon;

	// The "operator" socket
	CTCPSocket m_Operator;

	// The connected socket
	std::set<CTCPSocket *> m_setAccept;

	// Dib dimensions
	int m_cxStart,m_cyStart,m_cxWidth,m_cyHeight;
	double m_dxWidthScalar,m_dyHeightScalar;
	std::map<int,CRect> m_mapRect;

	// Session Id
	UINT m_nSessionId;

	// UAC
	DWORD m_dwEnableLUA;

// Implementation
protected:

	DECLARE_MESSAGE_MAP()

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// Socket event callback methods
	afx_msg LRESULT OnAcceptConn(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnReceiveData(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnCloseConn(WPARAM wParam,LPARAM lParam);

	// Image compress event callback method
	afx_msg LRESULT OnImageCompress(WPARAM wParam,LPARAM lParam);

	// Image refresh event callback method
	afx_msg LRESULT OnImageRefresh(WPARAM wParam,LPARAM lParam);

	// Image renew event callback method
	afx_msg LRESULT OnImageRenew(WPARAM wParam,LPARAM lParam);

	// Tray icon event callback method
	afx_msg LRESULT OnNotifyTray(WPARAM wParam,LPARAM lParam);

	// Methods for the tray icon
	afx_msg void OnClose();
	afx_msg void OnRestore();

	// Start and stop the server
	afx_msg void OnStart();
	afx_msg void OnStop();

	// Compression UI
	afx_msg void OnCompression();
	afx_msg void OnCompressionChoice();
	afx_msg void OnBW();
	afx_msg void OnLBW();

	// UI Stuff
	void EnableControls(BOOL bOffline = TRUE);

	// Initialize
	void CreateRefreshThreads();
	void DeleteRefreshThreads();

	// Helper for virtual key codes embedded in mouse messages
	void SetMouseMessage(WORD wMM,CPoint MousePosition,UINT nFlags,short zDelta);
	void SetKBMessage(WORD wMM,UINT nChar,UINT nRepCnt,UINT nFlags);
	void SetMouseKB(INPUT & KeyBoardInput,WORD wVk,bool bDown);
public:
	CComboBox m_ComboIPAddresses;
private:
	// Get a list of local IP addresses and attach them to the Combo List
	int GetIPAddresses();
};