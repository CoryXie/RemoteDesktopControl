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

// RDSDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RDS.h"
#include "RDSDlg.h"

#include "..\Common.h"
#include "..\TCPSocket.h"
#include "..\Packet.h"
#include "..\Registry.h"
#include <IPHlpApi.h>

#pragma comment(lib, "iphlpapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRDSDlg dialog
CRDSDlg::CRDSDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRDSDlg::IDD, pParent), m_TrayIcon(IDR_MAINFRAME),
	m_csPassword(_T("pass")), m_iPort(8370), m_nBitCount(32), 
	m_bUseCompression(TRUE), m_bAC(FALSE), m_bZLib(TRUE), m_pMTC(NULL),
	m_nCompThreads(0), m_cxWidth(0), m_cyHeight(0), m_dxWidthScalar(0), m_dyHeightScalar(0),
	m_iXScale(1), m_iYScale(1), m_nGridThreads(1), m_nIncoming(0), m_bAcceptUpdate(FALSE),
	m_bColors(TRUE), m_bBW(FALSE), m_bLBW(FALSE), m_bImgDIB(FALSE), m_bImgPNG(TRUE),
	m_nSessionId((UINT)-1), m_dwEnableLUA(0), m_bStarted(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Get the band resolution
	m_cxStart = GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_cyStart = GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_cxWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_cyHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Set the number of threads
	CPU cpu;
	m_nCompThreads = cpu.GetNbProcs();

	// Temporarily disable UAC
	// EnableLUA specifies whether Windows® User Account Controls (UAC) notifies the user when programs 
	// try to make changes to the computer. UAC was formerly known as Limited User Account (LUA).
	CRegistry Registry(TRUE,FALSE);
	if (Registry.Open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")))
	{
		if (!Registry.Read(_T("EnableLUA"), m_dwEnableLUA) || m_dwEnableLUA != 0)
			Registry.Write(_T("EnableLUA"), (DWORD)0);
	}
}

CRDSDlg::~CRDSDlg()
{
	// Restore the UAC
	if (m_dwEnableLUA != 0)
	{
		CRegistry Registry(TRUE,FALSE);
		if (Registry.Open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")))
			Registry.Write(_T("EnableLUA"), m_dwEnableLUA);
	}
}

void CRDSDlg::OnOK()
{
	EndDialog(IDOK);
}

void CRDSDlg::EnableControls(BOOL bOffline)
{
	// Update the controls
	m_UseCompression.EnableWindow(bOffline);
	m_AC.EnableWindow(bOffline && m_bUseCompression);
	m_ZLib.EnableWindow(bOffline && m_bUseCompression);
	m_ACThreads.EnableWindow(bOffline && m_bUseCompression);
	m_StaticACThreads.EnableWindow(bOffline && m_bUseCompression);
	m_Start.EnableWindow(bOffline);
	m_Stop.EnableWindow(!bOffline);
	m_StaticPassword.EnableWindow(bOffline);
	m_StaticPort.EnableWindow(bOffline);
	m_Password.EnableWindow(bOffline);
	m_Port.EnableWindow(bOffline);
	m_BW.EnableWindow(bOffline);
	m_LBW.EnableWindow(bOffline);
	m_Colors.EnableWindow(bOffline);
	m_ImgDIB.EnableWindow(bOffline);
	m_ImgPNG.EnableWindow(bOffline);
	m_StaticXScale.EnableWindow(bOffline);
	m_XScale.EnableWindow(bOffline);
	m_StaticYScale.EnableWindow(bOffline);
	m_YScale.EnableWindow(bOffline);
	m_StaticGridThreads.EnableWindow(bOffline);
	m_GridThreads.EnableWindow(bOffline);
}

void CRDSDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PWD, m_Password);
	DDX_Control(pDX, IDC_PORT, m_Port);
	DDX_Control(pDX, IDC_START, m_Start);
	DDX_Control(pDX, IDC_STOP, m_Stop);
	DDX_Control(pDX, IDC_STATIC_PWD, m_StaticPassword);
	DDX_Control(pDX, IDC_STATIC_PORT, m_StaticPort);
	DDX_Control(pDX, IDC_RGB, m_Colors);
	DDX_Control(pDX, IDC_BW, m_BW);
	DDX_Control(pDX, IDC_LBW, m_LBW);
	DDX_Control(pDX, IDC_IMGDIB, m_ImgDIB);
	DDX_Control(pDX, IDC_IMGPNG, m_ImgPNG);
	DDX_Control(pDX, IDC_COMPRESSION, m_UseCompression);
	DDX_Control(pDX, IDC_AC, m_AC);
	DDX_Control(pDX, IDC_ZLIB, m_ZLib);
	DDX_Control(pDX, IDC_ACTHREADS, m_ACThreads);
	DDX_Control(pDX, IDC_STATIC_ACTHREADS, m_StaticACThreads);
	DDX_Control(pDX, IDC_XSCALE, m_XScale);
	DDX_Control(pDX, IDC_STATIC_XSCALE, m_StaticXScale);
	DDX_Control(pDX, IDC_YSCALE, m_YScale);
	DDX_Control(pDX, IDC_STATIC_YSCALE, m_StaticYScale);
	DDX_Control(pDX, IDC_STATIC_GRIDTHREADS, m_StaticGridThreads);
	DDX_Control(pDX, IDC_GRIDTHREADS, m_GridThreads);

	DDX_Text(pDX, IDC_PWD, m_csPassword);
	DDV_MaxChars(pDX, m_csPassword, 255);
	DDX_Text(pDX, IDC_PORT, m_iPort);
	DDV_MinMaxInt(pDX, m_iPort, 1025, 65535);
	DDX_Check(pDX, IDC_RGB, m_bColors);
	DDX_Check(pDX, IDC_BW, m_bBW);
	DDX_Check(pDX, IDC_LBW, m_bLBW);
	DDX_Check(pDX, IDC_IMGDIB, m_bImgDIB);
	DDX_Check(pDX, IDC_IMGPNG, m_bImgPNG);
	DDX_Check(pDX, IDC_COMPRESSION, m_bUseCompression);
	DDX_Check(pDX, IDC_AC, m_bAC);
	DDX_Check(pDX, IDC_ZLIB, m_bZLib);
	DDX_Text(pDX, IDC_ACTHREADS, m_nCompThreads);
	DDV_MinMaxInt(pDX, m_nCompThreads, 0, 4);
	DDX_Text(pDX, IDC_XSCALE, m_iXScale);
	DDV_MinMaxInt(pDX, m_iXScale, 1, 4);
	DDX_Text(pDX, IDC_YSCALE, m_iYScale);
	DDV_MinMaxInt(pDX, m_iYScale, 1, 4);
	DDX_Text(pDX, IDC_GRIDTHREADS, m_nGridThreads);
	DDV_MinMaxInt(pDX, m_nGridThreads, 1, min(m_iXScale * m_iYScale, MAXTHREADS));
	DDX_Control(pDX, IDC_COMBO_IP, m_ComboIPAddresses);
}

BEGIN_MESSAGE_MAP(CRDSDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_ACCEPTCONN, &CRDSDlg::OnAcceptConn)
	ON_MESSAGE(WM_RECEIVEDATA, &CRDSDlg::OnReceiveData)
	ON_MESSAGE(WM_CLOSECONN, &CRDSDlg::OnCloseConn)
	ON_MESSAGE(WM_NOTIFY_TRAY, &CRDSDlg::OnNotifyTray)
	ON_COMMAND(ID_RDS_RESTORE, &CRDSDlg::OnRestore)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_START, &CRDSDlg::OnStart)
	ON_BN_CLICKED(IDC_STOP, &CRDSDlg::OnStop)
	ON_BN_CLICKED(IDC_COMPRESSION, &CRDSDlg::OnCompression)
	ON_BN_CLICKED(IDC_AC, &CRDSDlg::OnCompressionChoice)
	ON_BN_CLICKED(IDC_ZLIB, &CRDSDlg::OnCompressionChoice)
	ON_BN_CLICKED(IDC_BW, &CRDSDlg::OnBW)
	ON_BN_CLICKED(IDC_LBW, &CRDSDlg::OnLBW)
	ON_WM_TIMER()
	ON_MESSAGE(WM_IMAGECOMPRESS, &CRDSDlg::OnImageCompress)
	ON_MESSAGE(WM_IMAGEREFRESH, &CRDSDlg::OnImageRefresh)
	ON_MESSAGE(WM_IMAGERENEW, &CRDSDlg::OnImageRenew)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CRDSDlg message handlers
BOOL CRDSDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Enable the controls
	EnableControls();

	// Set up tray icon
	m_TrayIcon.SetNotificationWnd(this,WM_NOTIFY_TRAY);
	m_TrayIcon.SetIcon(IDR_MAINFRAME);
	
	GetIPAddresses();

	// Set the dialog title

	SetWindowText(CString(_T("Remote Desktop Server")));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRDSDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRDSDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Operator telling us that there is a pending connection to accept
LRESULT CRDSDlg::OnAcceptConn(WPARAM wParam,LPARAM lParam)
{
	// If this is a main connection then stop the timers and refresh threads until the connection is established
	if (!m_bAcceptUpdate)
	{
		// Allow a new connection
		CTCPSocket * pAccept = new CTCPSocket;
		CTCPSocket & Accept = *pAccept;

		// Set the parent for receiving socket notifications
		Accept.SetParent(GetSafeHwnd());

		// Accept the remote connection
		m_Operator.Accept(Accept);
		if (!Accept.InitTP())
			return 0;

		// Store the connection
		m_setAccept.insert(pAccept);

		// Send the number of compression threads
		CPacket Packet(m_nCompThreads,0);
		Accept << Packet;

		// Test for limiting connections to the server
		if (m_setAccept.size() >= 64)
		{
			// Don't accept anymore connections
			m_Operator.AsyncSelect(FD_READ | FD_CLOSE);
		}
	}
	else
	{
		// Allow a new connection that will be served screen updates
		CTCPSocket * pAccept = new CTCPSocket;
		CTCPSocket & Accept = *pAccept;

		// Accept the remote connection
		m_Operator.Accept(Accept);
		if (!Accept.InitTP())
			return 0;

		// Get the refresh thread that will handle this incoming connection
		size_t iPos = m_vRefreshThread.size() - m_nIncoming;
		CRefreshThread * pRefreshThread = m_vRefreshThread[iPos];

		// Assign the socket
		pRefreshThread->PostThreadMessage(WM_ACCEPTCONN,(WPARAM)pAccept->Detach(),(LPARAM)iPos);
		delete pAccept;

		// Update the incoming connections left to process
		m_nIncoming--;

		// Update the accept switch
		m_bAcceptUpdate = m_nIncoming == 0 ? FALSE : TRUE;
	}

	return 1;
}

// Connection telling us that there is pending data to receive
LRESULT CRDSDlg::OnReceiveData(WPARAM wParam,LPARAM lParam)
{
	// Get the connection
	CTCPSocket * pAccept = (CTCPSocket *)wParam;
	CTCPSocket & Accept = *pAccept;

	// Receive a packet of data
	CPacket Packet;
	Accept >> Packet;

	// Process the packet
	if (Packet.m_ucPacketType == 4)
	{
		// Get the collection of KB messages
		std::vector<CKBMsg> & vKBMsg = Packet.m_vKBMsg;

		// Get the count of messages
		UINT nKBMsg = (UINT)vKBMsg.size();

		// Issue each command
		for (UINT iKBMsg = 0;iKBMsg < nKBMsg;++iKBMsg)
		{
			// Get the command
			CKBMsg & KBMsg = vKBMsg[iKBMsg];

			// Execute the command
			SetKBMessage(KBMsg.m_wWM,KBMsg.m_nChar,KBMsg.m_nRepCnt,KBMsg.m_nFlags);
		}
	}
	else if (Packet.m_ucPacketType == 5)
	{
		// Get the collection of mouse messages
		std::vector<CMouseMsg> & vMouseMsg = Packet.m_vMouseMsg;

		// Get the count of messages
		UINT nMouseMsg = (UINT)vMouseMsg.size();

		// Issue each command
		for (UINT iMouseMsg = 0;iMouseMsg < nMouseMsg;++iMouseMsg)
		{
			// Get the command
			CMouseMsg & MouseMsg = vMouseMsg[iMouseMsg];

			// Execute the mouse command
			SetMouseMessage(MouseMsg.m_wWM,MouseMsg.m_MousePosition,MouseMsg.m_nFlags,MouseMsg.m_zDelta);
		}
	}
	else if (Packet.m_ucPacketType == 1)
	{
		// Reset the incoming connections
		m_nIncoming = 0;
		m_bAcceptUpdate = FALSE;

		// Create the verification packet
		CPacket Packet2(m_csPassword,0);

		// Test the submitted password hash
		if (Packet.m_csPasswordHash != Packet2.m_csPasswordHash)
		{
			// Close the connection
			Accept.Close();

			// Clean up and remove the closed connection
			std::set<CTCPSocket *>::iterator itAccept = m_setAccept.find(pAccept);
			if (itAccept != m_setAccept.end())
			{
				// Clean up the closed connection
				CTCPSocket * pAccept = *itAccept;
				delete pAccept;

				// Remove the closed connection
				m_setAccept.erase(itAccept);
			}
			return 0;
		}
		else
		{
			// Build the display attribute packet
			CPacket Packet(m_cxWidth,m_cyHeight,m_nBitCount,m_nThreads);

			// Send the packet
			Accept << Packet;
		}
	}
	else if (Packet.m_ucPacketType == 10)
	{
		if (Packet.m_nCompThreads != m_nCompThreads)
		{
			// Close the connection
			Accept.Close();

			// Clean up and remove the closed connection
			std::set<CTCPSocket *>::iterator itAccept = m_setAccept.find(pAccept);
			if (itAccept != m_setAccept.end())
			{
				// Clean up the closed connection
				CTCPSocket * pAccept = *itAccept;
				delete pAccept;

				// Remove the closed connection
				m_setAccept.erase(itAccept);
			}
			return 0;
		}
		else if (Packet.m_nSessionId == m_nSessionId - 1)
		{
			// Signifies the completion of the handshaking of the main connection
#if defined(_DEBUG)
			DebugMsg(_T("Session Id: %d\n"), Packet.m_nSessionId);
#endif
		}
		else
		{
			// Prepare for connections (Grid threads + cursor thread)
			m_bAcceptUpdate = TRUE;
			m_nIncoming = m_nThreads;

			// Build the final verification packet
			--m_nSessionId;
			CPacket Packet2(m_nCompThreads,m_nSessionId);

			// Send the packet
			Accept << Packet2;
		}
	}
	else
		return 0;
	return 1;
}

// Execute the mouse command
void CRDSDlg::SetMouseMessage(WORD wMM,CPoint MousePosition,UINT nFlags,short zDelta)
{
	// Read the mouse packet
	int mx = (int)((double)MousePosition.x * m_dxWidthScalar);
	int my = (int)((double)MousePosition.y * m_dyHeightScalar);

	#if defined(_DEBUG)
	DebugMsg(_T("MM=%d (%d,%d)\n"), wMM, mx, my);
	#endif

	// The time stamp of the operation in milli-seconds
	DWORD dwTime = 0;

	// Extract the flags
	bool bCtrl = nFlags & 1 ? true : false;
	bool bShift = nFlags & 2 ? true : false;

	// Set the number of commands in the input
	int nInputs = 0;

	// Test for the virtual keys
	if (bCtrl)
		nInputs += 2;
	if (bShift)
		nInputs += 2;

	// For the loop termination
	int nHalf = nInputs / 2;

	// Test for the mouse message
	nInputs += 1;
	if (wMM == WM_LBUTTONDBLCLK || wMM == WM_MBUTTONDBLCLK || wMM == WM_RBUTTONDBLCLK)
		nInputs += 3;

	// Create the maximum number of user inputs
	std::auto_ptr<INPUT> UserInput(new INPUT[nInputs]);
	INPUT * pUserInput = UserInput.get();

	// Set the starting input
	int nStart = 0;

	// Set the virtual keydown(s) for the mouse message
	if (bCtrl)
	{
		INPUT & KeyBoardInput = *(pUserInput + (nStart++));
		SetMouseKB(KeyBoardInput,VK_CONTROL,true);
	}
	if (bShift)
	{
		INPUT & KeyBoardInput = *(pUserInput + (nStart++));
		SetMouseKB(KeyBoardInput,VK_SHIFT,true);
	}

	// Build up the mouse input
	int iInput = nStart;
	for (;iInput < (nInputs - nHalf);iInput++,dwTime += 100)
	{
		// Get the mouse input
		INPUT & MouseInput = pUserInput[iInput];

		// Set the input type
		MouseInput.type = INPUT_MOUSE;

		// Get the mouse input structure
		MOUSEINPUT & Mouse = MouseInput.mi;

		// Set the absolute coordinates
		Mouse.dx = mx;
		Mouse.dy = my;

		// Set the mouse data
		Mouse.mouseData = zDelta;

		// Set the flags
		Mouse.dwFlags = MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE;

		// Set the button(s)
		if (wMM == WM_LBUTTONDBLCLK)
		{
			if (iInput == 0 || iInput == 2)
				Mouse.dwFlags |= MOUSEEVENTF_LEFTDOWN;
			else if (iInput == 1 || iInput == 3)
				Mouse.dwFlags |= MOUSEEVENTF_LEFTUP;
		}
		else if (wMM == WM_LBUTTONDOWN)
			Mouse.dwFlags |= MOUSEEVENTF_LEFTDOWN;
		else if (wMM == WM_LBUTTONUP)
			Mouse.dwFlags |= MOUSEEVENTF_LEFTUP;
		else if (wMM == WM_MBUTTONDBLCLK)
		{
			if (iInput == 0 || iInput == 2)
				Mouse.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
			else if (iInput == 1 || iInput == 3)
				Mouse.dwFlags |= MOUSEEVENTF_MIDDLEUP;
		}
		else if (wMM == WM_MBUTTONDOWN)
			Mouse.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
		else if (wMM == WM_MBUTTONUP)
			Mouse.dwFlags |= MOUSEEVENTF_MIDDLEUP;
		else if (wMM == WM_MOUSEWHEEL)
			Mouse.dwFlags |= MOUSEEVENTF_WHEEL;
		else if (wMM == WM_MOUSEMOVE)
			Mouse.dwFlags |= MOUSEEVENTF_MOVE;
		else if (wMM == WM_RBUTTONDBLCLK)
		{
			if (iInput == 0 || iInput == 2)
				Mouse.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
			else if (iInput == 1 || iInput == 3)
				Mouse.dwFlags |= MOUSEEVENTF_RIGHTUP;
		}
		else if (wMM == WM_RBUTTONDOWN)
			Mouse.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
		else if (wMM == WM_RBUTTONUP)
			Mouse.dwFlags |= MOUSEEVENTF_RIGHTUP;

		// Set the time
		Mouse.time = dwTime;
	}

	// Set the new starting place
	nStart = iInput;

	// Set the virtual keydown(s) for the mouse message
	if (bCtrl)
	{
		INPUT & KeyBoardInput = *(pUserInput + (nStart++));
		SetMouseKB(KeyBoardInput,VK_CONTROL,false);
	}
	if (bShift)
	{
		INPUT & KeyBoardInput = *(pUserInput + (nStart++));
		SetMouseKB(KeyBoardInput,VK_SHIFT,false);
	}

	// Send the user inputs for the mouse
	SendInput(nInputs,pUserInput,sizeof(INPUT));
}

// Execute the keyboard command
void CRDSDlg::SetKBMessage(WORD wMM,UINT nChar,UINT nRepCnt,UINT nFlags)
{
	// Create the maximum number of user inputs
	std::auto_ptr<INPUT> UserInput(new INPUT[nRepCnt]);
	INPUT * pUserInput = UserInput.get();

	// Build up the keyboard input
	for (UINT iInput = 0;iInput < nRepCnt;++iInput)
	{
		// Get the keyboard input
		INPUT & KeyBoardInput = pUserInput[iInput];

		// Set the input type
		KeyBoardInput.type = INPUT_KEYBOARD;

		// Get the keyboard input structure
		KEYBDINPUT & KeyBoard = KeyBoardInput.ki;

		// Set the scan code
		KeyBoard.wScan = 0;

		// Set the ignored fields
		KeyBoard.dwExtraInfo = 0;
		KeyBoard.time = 0;

		// Test for the key being pressed or released
		KeyBoard.dwFlags = wMM == WM_KEYDOWN ? 0 : KEYEVENTF_KEYUP;

		// Set the character
		KeyBoard.wVk = nChar;
	}

	// Send the user input for the keyboard
	SendInput(nRepCnt,pUserInput,sizeof(INPUT));
}

// Set the virtual keycode for the mouse message
void CRDSDlg::SetMouseKB(INPUT & KeyBoardInput,WORD wVk,bool bDown)
{
	// Set the input type
	KeyBoardInput.type = INPUT_KEYBOARD;

	// Get the keyboard input structure
	KEYBDINPUT & KeyBoard = KeyBoardInput.ki;

	// Set the scan code
	KeyBoard.wScan = 0;

	// Set the ignored fields
	KeyBoard.dwExtraInfo = 0;
	KeyBoard.time = 0;

	// Test for the key being pressed or released
	KeyBoard.dwFlags = bDown ? 0 : KEYEVENTF_KEYUP;

	// Set the virtual key
	KeyBoard.wVk = wVk;
}

// Refresh Thread event signaling to compress a screen update
LRESULT CRDSDlg::OnImageCompress(WPARAM wParam,LPARAM lParam) 
{
	// Get the image information structure for compression
	CompressionInfo & Info = *(CompressionInfo *)wParam;
	char * pInBuffer = Info.m_pInBuffer;
	DWORD dwSrcBytes = Info.m_dwSrcBytes;
	char * & pOutBuffer = *(Info.m_ppOutBuffer);
	DWORD & dwOutBytes = *(Info.m_pdwOutBytes);

	// Test for the multi-threaded compressor
	BOOL bMTC = m_nCompThreads ? TRUE : FALSE;

	// The temporary output buffer
	char * pTempBuffer = NULL;

	// Test for MT/ST compression
	if (bMTC)
	{
		// Set the type of encoder to the multi-threaded compressor
		m_pMTC->SetEncoder(m_bAC);

		// Last parameter determines either Multithreaded arithmentic encoding or zlib
		m_pMTC->SetBuffer(pInBuffer,dwSrcBytes,TRUE);
		m_pMTC->Encode();

		// Get the temporary output buffer
		m_pMTC->GetBuffer(&pTempBuffer,&dwOutBytes,TRUE);
	}
	else
	{
		// Test for the compression type
		if (m_bAC)
		{
			// Single-threaded arithmetic encoding
			m_AE.SetBuffer(pInBuffer,dwSrcBytes);
			m_AE.EncodeBuffer();
			m_AE.GetBuffer(pTempBuffer,dwOutBytes);
		}
		else
		{
			// Single-threaded ZLIB encoding
			m_ZL.SetBuffer(pInBuffer,dwSrcBytes,TRUE);
			m_ZL.EncodeBuffer();
			m_ZL.GetBuffer(pTempBuffer,dwOutBytes);
		}
	}

	// Copy to the output buffer
	memcpy(pOutBuffer,pTempBuffer,dwOutBytes);

	return 1;
}

// Refresh thread event signaling that a screen update was sent
LRESULT CRDSDlg::OnImageRefresh(WPARAM wParam,LPARAM lParam) 
{
	// Only send if the system is in a running state
	if (m_bStarted)
	{
		// Get the thread
		CRefreshThread * pRefreshThread = (CRefreshThread *)wParam;

		// Get the image packet
		CPacket * pDIBPacket = (CPacket *)lParam;

		// Queue the image for sending
		pRefreshThread->PostThreadMessage(WM_IMAGESEND,(WPARAM)pDIBPacket,0);
	}
	return 1;
}

// Refresh thread event signaling to start the next sequence
LRESULT CRDSDlg::OnImageRenew(WPARAM wParam,LPARAM lParam) 
{
	// Only send if the system is in a running state
	if (m_bStarted)
	{
		// Get the thread
		CRefreshThread * pRefreshThread = (CRefreshThread *)wParam;

		// Queue the image for sending
		pRefreshThread->PostThreadMessage(WM_IMAGEREFRESH,0,0);
	}
	return 1;
}

// Client connection has been closed
LRESULT CRDSDlg::OnCloseConn(WPARAM wParam,LPARAM lParam)
{
	// Get the connection
	CTCPSocket * pAccept = (CTCPSocket *)wParam;

	// Clean up and remove the closed connection
	std::set<CTCPSocket *>::iterator itAccept = m_setAccept.find(pAccept);
	if (itAccept != m_setAccept.end())
	{
		// Clean up the closed connection
		CTCPSocket * pAccept = *itAccept;
		delete pAccept;

		// Remove the closed connection
		m_setAccept.erase(itAccept);
	}

	// Test for limiting connections to the server
	if (m_setAccept.size() < 64)
	{
		// Accept connections again
		m_Operator.AsyncSelect(FD_ACCEPT | FD_READ | FD_CLOSE);
	}

	return 1;
}

// Handle notification from tray icon
LRESULT CRDSDlg::OnNotifyTray(WPARAM uID,LPARAM lEvent)
{
	// Let tray icon do default stuff
	return m_TrayIcon.OnNotifyTray(uID,lEvent);
}

// Hide the window for close notifications
void CRDSDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

// Show the window for the "restore" menu item
void CRDSDlg::OnRestore()
{
	ShowWindow(SW_SHOW);
}

// Start the listener
void CRDSDlg::OnStart()
{
	// Update the UI entries
	BOOL bUpdate = UpdateData();
	if (!bUpdate)
		return;

	// Set the started status
	m_bStarted = TRUE;

	// Disable the UI
	EnableControls(FALSE);

	// Get the bit count
	m_nBitCount = m_bColors ? 32 : (!m_bLBW ? 8 : 4);

	// Get the scalars for converting mouse coordinates
	m_dxWidthScalar = 65535.0 / (double)m_cxWidth;
	m_dyHeightScalar = 65535.0 / (double)m_cyHeight;

	// Create a listener
	m_Operator.Create(m_iPort);
	m_Operator.SetParent(GetSafeHwnd());

	// Create the threads to test for screen updates
	CreateRefreshThreads();

	// Hide the dialog
	OnClose();
}

// Stop the listener
void CRDSDlg::OnStop()
{
	// Set the started status
	m_bStarted = FALSE;

	// Clean up the refresh threads
	DeleteRefreshThreads();

	// Remove the inactive connection
	std::set<CTCPSocket *>::iterator itAccept;
	for (itAccept = m_setAccept.begin();itAccept != m_setAccept.end();++itAccept)
	{
		// Get the connection
		CTCPSocket * pAccept = *itAccept;
		CTCPSocket & Accept = *pAccept;

		// Shutdown and close the connection
		Accept.ShutDown();
		Accept.Close();

		// Clean up the connection
		delete pAccept;
	}

	// Remove all the closed connections
	m_setAccept.clear();

	// Shutdown and close the operator
	m_Operator.ShutDown();
	m_Operator.Close();

	// Enable the controls
	EnableControls();
}

// Compression UI
void CRDSDlg::OnCompression()
{
	// Update the IU entries
	UpdateData();

	// Enable/disable the compression choices
	m_AC.EnableWindow(m_bUseCompression);
	m_ZLib.EnableWindow(m_bUseCompression);

	// Enable/disable the AC threads
	m_StaticACThreads.EnableWindow(m_bUseCompression);
	m_ACThreads.EnableWindow(m_bUseCompression);
}

void CRDSDlg::OnCompressionChoice()
{
	// Update the IU entries
	UpdateData();

	// Enable/disable the AC threads
	m_StaticACThreads.EnableWindow(m_bUseCompression);
	m_ACThreads.EnableWindow(m_bUseCompression);
}

// Black and White compresses better as a DIB than PNG
void CRDSDlg::OnBW()
{
	m_bBW = TRUE;
	m_bColors = FALSE;
	m_bImgDIB = TRUE;
	m_bImgPNG = FALSE;
	UpdateData(FALSE);
}

// Low bandwidth is Black and White at 4BPP
void CRDSDlg::OnLBW()
{
	// Disable the controls for low bandwidth
	m_BW.EnableWindow(m_bLBW);
	m_Colors.EnableWindow(m_bLBW);
	m_ImgDIB.EnableWindow(m_bLBW);
	m_ImgPNG.EnableWindow(m_bLBW);
	m_StaticACThreads.EnableWindow(m_bLBW);
	m_ACThreads.EnableWindow(m_bLBW);

	// Set the control values
	m_bBW = TRUE;
	m_bColors = FALSE;
	m_bImgDIB = TRUE;
	m_bImgPNG = FALSE;
	m_bLBW = !m_bLBW;

	// Such a low amount of data that better compression should be achieved in 1 thread
	m_nCompThreads = 0;

	// Update the controls based on the new values
	UpdateData(FALSE);
}

// Create the refresh threads
void CRDSDlg::CreateRefreshThreads()
{
	// Create the multithreaded compressor
	if (m_nCompThreads)
		m_pMTC = new CDriveMultiThreadedCompression(m_nCompThreads);

	// Create the rectangular comparison region
	CPoint TopLeft(m_cxStart,m_cyStart);
	CPoint BottomRight(m_cxWidth / m_iXScale + m_cxStart,m_cyHeight / m_iYScale + m_cyStart);
	CRect DIBRect = CRect(TopLeft,BottomRight);

	// Calculate how many areas are updated per thread
	int nGrids = m_iXScale * m_iYScale;
	int nGridsPerThread = nGrids / m_nGridThreads;

	// Test for an uneven amount (some threads do more work than others...)
	if (nGridsPerThread * m_nGridThreads != nGrids)
		nGridsPerThread++;

	// Build the update areas per thread
	bool bThreads = true;
	for (int iThread = 0;iThread < m_nGridThreads;++iThread)
	{
		// Build the collection of rectangular regions that one thread processes
		std::vector<CRect> vDIBRect;
		for (int iGridsPerThread = 0;iGridsPerThread < nGridsPerThread;++iGridsPerThread)
		{
			// There could be an odd number of regions in the last thread
			if (bThreads)
			{
				// Debugging
				TopLeft = DIBRect.TopLeft();
				BottomRight = DIBRect.BottomRight();
				DebugMsg(_T("Thread=%d Rect=(%d,%d)-(%d,%d)\n"), iThread, TopLeft.x, TopLeft.y, BottomRight.x, BottomRight.y);
				
				// Store the rectangle
				vDIBRect.push_back(DIBRect);

				// Update the rectangle
				DIBRect.OffsetRect(DIBRect.Width(),0);

				// Boundary test
				if (DIBRect.left == (m_cxWidth + m_cxStart))
				{
					// Reset the boundary
					DIBRect.left = m_cxStart;
					DIBRect.right = m_cxWidth / m_iXScale + m_cxStart;

					// Update the rectangle
					DIBRect.OffsetRect(0,DIBRect.Height());

					// Boundary test
					if (DIBRect.top == (m_cyHeight + m_cyStart))
						bThreads = false;
				}
			}
		}

		if (!vDIBRect.empty())
		{
			// Create the refresh thread
			m_vRefreshThread.push_back(new CRefreshThread(GetSafeHwnd(),vDIBRect,m_nBitCount,m_bImgDIB,m_bUseCompression,m_bAC,iThread,m_nCompThreads));
		}
	}

	// Create the cursor thread
	m_vRefreshThread.push_back(new CRefreshThread(GetSafeHwnd(),std::vector<CRect>(),0,FALSE,FALSE,FALSE,0,0));

	// Count the threads
	m_nThreads = (DWORD)m_vRefreshThread.size();

	// Start the first sequence 
	std::vector<CRefreshThread *>::iterator itThread = m_vRefreshThread.begin();
	for (;itThread != m_vRefreshThread.end();++itThread)
	{
		CRefreshThread * pRefreshThread = *itThread;
		pRefreshThread->PostThreadMessage(WM_IMAGEREFRESH,0,0);
	}
}

// Clean up the refresh threads
void CRDSDlg::DeleteRefreshThreads()
{
	// Clean up the refresh threads
	std::vector<CRefreshThread *>::iterator itThreadPtr;
	for (itThreadPtr = m_vRefreshThread.begin();itThreadPtr != m_vRefreshThread.end();++itThreadPtr)
	{
		CRefreshThread * pRefreshThread = *itThreadPtr;
		pRefreshThread->PostThreadMessage(WM_ENDTHREAD,0,0);
		DWORD dwWait = WaitForSingleObject(pRefreshThread->m_hThread,5000);
		if (dwWait != WAIT_OBJECT_0)
			TerminateThread(pRefreshThread->m_hThread,1);
	}

	// Clear the vector of threads
	m_vRefreshThread.clear();

	// Clean up the multi-threaded compressor
	if (m_pMTC)
	{
		// Delete the multithreaded compressor
		delete m_pMTC;
		m_pMTC = NULL;
	}
}

#if 0
// Get a list of local IP addresses and attach them to the Combo List
int CRDSDlg::GetIPAddresses()
{
	char HostName[128];
	int numIps = 0;

	UpdateData(TRUE);

	//Get host name and Check is there was any error
	if (gethostname(HostName, sizeof(HostName)) == SOCKET_ERROR)
	{
		AfxMessageBox(_T("Error in Getting Host Info"));
		CDialog::OnCancel();
	}

	//Get the ip address of the machine using gethostbyname function
	struct hostent *IpList = gethostbyname(HostName);
	if (IpList == 0)
	{
		AfxMessageBox(_T("Yow! Bad host lookup."));
		CDialog::OnCancel();
	}

	//Enumerate list of Ip Address available in the machine
	for (int i = 0; IpList->h_addr_list[i] != 0; ++i)
	{
		struct in_addr addr;

		memcpy(&addr, IpList->h_addr_list[i], sizeof(struct in_addr));

		CString str = CString(inet_ntoa(addr));

		m_ComboIPAddresses.AddString(str);
	}

	numIps = m_ComboIPAddresses.GetCount();

	if (numIps > 0)
		m_ComboIPAddresses.SetCurSel(0);

	UpdateData(FALSE);

	return numIps;
}

#else
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

// Fetches the MAC address and prints it
int CRDSDlg::GetIPAddresses(void)
{
	int numIps = 0;
	int i;

	/* Variables used by GetIpAddrTable */
	PMIB_IPADDRTABLE pIPAddrTable;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	IN_ADDR IPAddr;

	// Before calling AddIPAddress we use GetIpAddrTable to get
	// an adapter to which we can add the IP.
	pIPAddrTable = (MIB_IPADDRTABLE *)MALLOC(sizeof (MIB_IPADDRTABLE));

	if (pIPAddrTable) {
		// Make an initial call to GetIpAddrTable to get the
		// necessary size into the dwSize variable
		if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
			FREE(pIPAddrTable);
			pIPAddrTable = (MIB_IPADDRTABLE *)MALLOC(dwSize);

		}
		if (pIPAddrTable == NULL) {
			AfxMessageBox(_T("Memory allocation failed for GetIpAddrTable\n"));
			return 0;
		}
	}

	// Make a second call to GetIpAddrTable to get the
	// actual data we want
	if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR) {
		AfxMessageBox(_T("GetIpAddrTable failed with error %d\n", dwRetVal));
		return 0;
	}

	m_ComboIPAddresses.Clear();

	for (i = 0; i < (int)pIPAddrTable->dwNumEntries; i++) {

		IPAddr.S_un.S_addr = (ULONG)pIPAddrTable->table[i].dwAddr;

		CString str = CString(inet_ntoa(IPAddr));
		
		str += "[ ";
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY)
			str += "Primary ";
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DYNAMIC)
			str += "Dynamic ";
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DISCONNECTED)
			str += "Disconnected ";
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DELETED)
			str += "Deleted ";
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_TRANSIENT)
			str += "Transient ";
		str += "]";

		m_ComboIPAddresses.AddString(str);

	}

	if (pIPAddrTable) {
		FREE(pIPAddrTable);
		pIPAddrTable = NULL;
	}

	numIps = m_ComboIPAddresses.GetCount();

	if (numIps > 0)
		m_ComboIPAddresses.SetCurSel(0);

	UpdateData(FALSE);

	return numIps;
}
#endif