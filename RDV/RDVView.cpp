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

#include "stdafx.h"
#include "RDV.h"
#include "RDVDoc.h"
#include "RDVView.h"
#include "NewConnectionDlg.h"
#include "afxole.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRDVView

IMPLEMENT_DYNCREATE(CRDVView, CScrollView)

BEGIN_MESSAGE_MAP(CRDVView, CScrollView)
	ON_COMMAND(ID_FILE_PRINT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CScrollView::OnFilePrintPreview)

	// Socket notifications
	ON_MESSAGE(WM_MAKECONN, &CRDVView::OnMakeConn)
	ON_MESSAGE(WM_RECEIVEDATA, &CRDVView::OnReceiveData)
	ON_MESSAGE(WM_CLOSECONN, &CRDVView::OnCloseConn)

	// Socket messages
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()

	// Cursor
	ON_WM_SETCURSOR()

	// Timer
	ON_WM_TIMER()

	// Background
	ON_WM_ERASEBKGND()

	// Resizing
	ON_WM_SIZE()

	// Shrink to fit
	ON_COMMAND(ID_VIEW_ZOOM, &CRDVView::OnViewZoom)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM, &CRDVView::OnUpdateViewZoom)
END_MESSAGE_MAP()

// CRDVView construction/destruction
CRDVView::CRDVView()
{
	m_cxWidth = 0;
	m_cyHeight = 0;
	m_nBitCount = 0;
	m_nGridThreads = 0;
	m_iGridThread = 0;
	m_nCompThreads = 0;
	m_bConnected = FALSE;
	m_pMTC = NULL;
	m_bViewZoom = TRUE;
	m_nSessionId = 0;
}

CRDVView::~CRDVView()
{
	// Delete the multithreaded AC driver
	if (m_pMTC)
	{
		delete m_pMTC;
		m_pMTC = NULL;
	}
}

BOOL CRDVView::DestroyWindow()
{
	return CView::DestroyWindow();
}

// Initialize the scrollbars
void CRDVView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	m_ViewSize.cx = m_ViewSize.cy = 0;
	SetScrollSizes(MM_TEXT,m_ViewSize);
}

BOOL CRDVView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CScrollView::PreCreateWindow(cs);
}

// CRDVView drawing
void CRDVView::OnDraw(CDC* pDC)
{
	// Update the display
	pDC->BitBlt(0,0,m_cxWidth,m_cyHeight,(CDC *)m_DIB,0,0,SRCCOPY);
}

// CRDVView printing
BOOL CRDVView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// Print only 1 page, the current frame
	pInfo->SetMaxPage(1);

	// default preparation
	return DoPreparePrinting(pInfo);
}

void CRDVView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CRDVView::OnPrint(CDC * pDC,CPrintInfo * pInfo)
{
	// Get the printer width and height in pixels
	int cxPrintWidth = pDC->GetDeviceCaps(HORZRES);
	int cyPrintHeight = pDC->GetDeviceCaps(VERTRES);

	// Stretch the bitmap to the area of the printer
	StretchBlt(pDC->GetSafeHdc(),0,0,cxPrintWidth,cyPrintHeight,m_DIB,0,0,m_cxWidth,m_cyHeight,SRCCOPY);
}

void CRDVView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

// CRDVView diagnostics
#ifdef _DEBUG
void CRDVView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CRDVView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CRDVDoc * CRDVView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRDVDoc)));
	return (CRDVDoc *)m_pDocument;
}
#endif //_DEBUG

// Connect to the server
void CRDVView::ConnectServer()
{
	// Get the connection specifics
	CString csIp = GetDocument()->m_csIp;
	CString csPort = GetDocument()->m_csPort;

	// Create the outbound connection
	m_Socket.Create();

	// Set the parent for receiving socket notifications
	m_Socket.SetParent(GetSafeHwnd());

	// Attempt to connect to the server
	m_Socket.Connect(csIp,_ttoi(csPort));
}

// Disconnect from the server
void CRDVView::DisconnectServer()
{
	if (m_bConnected)
	{
		// Close the connection
		m_Socket.ShutDown();
		m_Socket.Close();

		// Delete the sink threads
		DeleteSinkThreads();

		// Stop the KB and Mouse timer
		KillTimer(4);

		// Update the connected status
		m_bConnected = FALSE;
	}

	// Clean up the GDI
	CleanupGDI();
}

// Connection has been made
LRESULT CRDVView::OnMakeConn(WPARAM wParam,LPARAM lParam)
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

		// Set the timer to close the document
		SetTimer(1,0,NULL);
	}
	return lParam;
}

// Received data event from the desktop server, now read the data
LRESULT CRDVView::OnReceiveData(WPARAM wParam,LPARAM lParam)
{
	// Receive the data
	CPacket * pPacket = NULL;

	// Receive the packet of data
	if (lParam)
	{
		// Image has been received in a thread and sent
		pPacket = (CPacket *)lParam;
	}
	else
	{
		// Read the packet from the main connection
		pPacket = new CPacket;
		CPacket & Packet = *pPacket;
		m_Socket >> Packet;
	}

	// Make it easy to work with either packet
	CPacket & Packet = *pPacket;

	// Process the data
	if (Packet.m_ucPacketType == 7)
	{
		// Create the temporary DIB
		CDIBFrame DIBPacket;
		DIBPacket.Init(Packet.m_Rect.Width(),Packet.m_Rect.Height(),m_nBitCount);

		// Point to the internal buffer of the DIB
		char * pBuffer = (LPSTR)DIBPacket;
		DWORD dwBytes = Packet.m_dwBytes;

		// Test for compression
		if (Packet.m_bUseCompression)
		{
			// Test for multi-threaded compression
			if (Packet.m_nCompThreads)
			{
				// Set the type of encoder
				m_pMTC->SetEncoder(Packet.m_bAC);

				// Multithreaded arithmetic decoding
				m_pMTC->SetBuffer(Packet.m_pBuffer,Packet.m_bAC ? Packet.m_dwBytes : Packet.m_dwSrcBytes,FALSE);
				m_pMTC->Decode();

				// Get the output buffer
				m_pMTC->GetBuffer(&pBuffer,&dwBytes,FALSE,FALSE);
			}
			else
			{
				if (Packet.m_bAC)
				{
					// Single-threaded arithmetic encoding
					m_AC.DecodeBuffer(Packet.m_pBuffer,Packet.m_dwBytes,&pBuffer,&dwBytes,FALSE);
				}
				else
				{
					// Single-threaded zlib uncompressing
					dwBytes = Packet.m_dwSrcBytes;
					m_ZLib.DecodeBuffer(Packet.m_pBuffer,Packet.m_dwBytes,pBuffer,dwBytes,FALSE);
				}
			}
		}
		else
		{
			// Copy the uncompressed DIB to the output
			memcpy(pBuffer,Packet.m_pBuffer,dwBytes);
		}

		// Get the image type
		BOOL bDIB = Packet.m_bDIB;

		// Get the reset attribute
		BOOL bXOR = Packet.m_bXOR;

		// Get the delta DIB
		CDIBFrame & XOR = m_XOR;

		// Get the viewed DIB
		CDIBFrame & DIB = m_DIB;

		// Get the rectangular coordinates
		CRect Rect = Packet.m_Rect;

		// Check for a PNG
		if (!bDIB)
		{
			// JPEG support
			COleStreamFile ImageStream;

			// Prepare the Image for memory stream serialization
			if (ImageStream.CreateMemoryStream())
			{
				// Write the packet to the stream
				ImageStream.Write(pBuffer,dwBytes);
				IStream * pImageStream = ImageStream.GetStream();

				// Load the image from the stream
				CImage ImageDIB;
				if (pImageStream && SUCCEEDED(ImageDIB.Load(pImageStream)))
				{
					// Transfer the PNG to a DIB
					ImageDIB.BitBlt(DIBPacket,0,0,SRCCOPY);

					// Release the stream
					pImageStream->Release();
				}
			}
		}

		if (bXOR)
		{
			// XOR the current desktop with the last
			BitBlt(DIB,Rect.left,Rect.top,Rect.Width(),Rect.Height(),DIBPacket,0,0,SRCINVERT);
		}
		else
		{
			// Copy over the current desktop
			BitBlt(DIB,Rect.left,Rect.top,Rect.Width(),Rect.Height(),DIBPacket,0,0,SRCCOPY);
		}

		// Update the last DIB
		BitBlt(XOR,0,0,m_cxWidth,m_cyHeight,DIB,0,0,SRCCOPY);

		// Update the screen
		InvalidateRect(NULL,FALSE);
		UpdateWindow();
	}
	else if (Packet.m_ucPacketType == 3) // Cursor
	{
		// Get the cursor data
		DWORD dwXHotSpot = Packet.m_dwXHotSpot;
		DWORD dwYHotSpot = Packet.m_dwYHotSpot;
		int nWidth = Packet.m_bmWidth;
		int nHeight = Packet.m_bmHeight;
		WORD bmMaskPlanes = Packet.m_bmMaskPlanes;
		WORD bmMaskBitsPixel = Packet.m_bmMaskBitsPixel;
		WORD bmColorPlanes = Packet.m_bmColorPlanes;
		WORD bmColorBitsPixel = Packet.m_bmColorBitsPixel;
		BYTE * pMaskBits = NULL;
		BYTE * pColorBits = NULL;
		if (Packet.m_MaskBits.size())
			pMaskBits = &(Packet.m_MaskBits[0]);
		if (Packet.m_ColorBits.size())
			pColorBits = &(Packet.m_ColorBits[0]);

		// Create a cursor
		m_Cursor.CreateCursor(dwXHotSpot,dwYHotSpot,nWidth,nHeight,bmMaskPlanes,bmMaskBitsPixel,bmColorPlanes,bmColorBitsPixel,pMaskBits,pColorBits);

		// Set the cursor
		SetClassLong(m_hWnd,-12,(LONG)(HCURSOR)m_Cursor);
		SetCursor(m_Cursor);
	}
	else if (Packet.m_ucPacketType == 2) // Display characteristics
	{
		// Free the previous multithread AC encoder
		if (m_pMTC)
		{
			delete m_pMTC;
			m_pMTC = NULL;
		}

		// Create the multithreaded AC driver
		m_pMTC = new CDriveMultiThreadedCompression(m_nCompThreads);

		// Display characteristics
		m_cxWidth = Packet.m_cxScreen;
		m_cyHeight = Packet.m_cyScreen;
		m_nBitCount = Packet.m_nBitCount;
		m_nGridThreads = Packet.m_nGridThreads;
		m_iGridThread = 0;

		// Set the new scroll sizes
		m_ViewSize.cx = m_cxWidth;
		m_ViewSize.cy = m_cyHeight;

		// Handle the initial condition
		if (m_bViewZoom)
		{
			// Provide a full view of the server with scroll bars
			SetScrollSizes(MM_TEXT,m_ViewSize);
			GetParentFrame()->RecalcLayout();
			ResizeParentToFit();
		}
		else
		{
			// Shrink to fit the server in the client window
			SetScaleToFitSize(m_ViewSize);
			GetParentFrame()->RecalcLayout();
		}

		// Setup the GDI
		SetupGDI();

		// Send the verification
		CPacket Packet2(m_nCompThreads,0);

		// Send the packet
		m_Socket << Packet2;
	}
	else if (Packet.m_ucPacketType == 10)
	{
		// Get the number of compression threads
		m_nCompThreads = Packet.m_nCompThreads;

		if (Packet.m_nSessionId == 0)
		{
			// Get the password for verification
			CString csPassword = GetDocument()->m_csPassword;

			// If the session is set then this verification to the server that the handshaking is over for the thread connection
			CPacket Packet2(csPassword,m_nSessionId);

			// Send the packet
			m_Socket << Packet2;
		}
		else
		{
			// Set the session id for the main connection and then set the timer for making the other connections
			if (!m_nSessionId)
			{
				// Get the session id
				m_nSessionId = Packet.m_nSessionId;

				// Make server connections for each update rectangle thread
				SetTimer(2,0,NULL);
			}
			else
			{
				// Close the document
				GetDocument()->OnCloseDocument();
			}
		}
	}

	// Clean up the packet if it needed to be created
	if (!lParam)
		delete pPacket;

	return 1;
}

// Connection telling us that a connection has been closed
LRESULT CRDVView::OnCloseConn(WPARAM wParam,LPARAM lParam)
{
	if (m_bConnected)
	{
		// Not connected
		m_bConnected = FALSE;

		// Delete the sink threads
		DeleteSinkThreads();

		// Notify
		AfxMessageBox(_T("The connection to the server has been dropped\n"));
	}
	else
	{
		// Notify
		AfxMessageBox(_T("The password is incorrect\n"));
	}

	// Set the timer to close the document
	SetTimer(1,0,NULL);

	return 1;
}

// Setup the GDI resources
void CRDVView::SetupGDI()
{
	// Initialize the DIB
	m_DIB.Init(m_cxWidth,m_cyHeight,m_nBitCount);

	// Initialize the delta DIB
	m_XOR.Init(m_cxWidth,m_cyHeight,m_nBitCount);
}

// Clean up the GDI resources
void CRDVView::CleanupGDI()
{
	// Cleanup the DIB band
	m_DIB.DeleteFrame();

	// Cleanup the XOR band
	m_XOR.DeleteFrame();
}

// Clean up the refresh sink threads
void CRDVView::DeleteSinkThreads()
{
	// Clean up the refresh threads
	std::vector<CSocketWndSinkThread *>::iterator itThreadPtr;
	for (itThreadPtr = m_vecSinkThreads.begin();itThreadPtr != m_vecSinkThreads.end();++itThreadPtr)
	{
		CSocketWndSinkThread * pSinkThread = *itThreadPtr;
		pSinkThread->PostThreadMessage(WM_ENDTHREAD,0,0);
		if (WaitForSingleObject(pSinkThread->m_hThread,5000) != WAIT_OBJECT_0)
			TerminateThread(pSinkThread->m_hThread,0);
	}

	// Clear the vector of threads
	m_vecSinkThreads.clear();
}

// Handle left button double click
void CRDVView::OnLButtonDblClk(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_LBUTTONDBLCLK,nFlags,Point);
	CScrollView::OnLButtonDblClk(nFlags,Point);
}

void CRDVView::OnLButtonDown(UINT nFlags,CPoint Point)
{
	// Test the keystate
	if (m_bConnected)
		SendMouseMessage(WM_LBUTTONDOWN,nFlags,Point);
	CScrollView::OnLButtonDown(nFlags,Point);
}

void CRDVView::OnLButtonUp(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_LBUTTONUP,nFlags,Point);
	CScrollView::OnLButtonUp(nFlags,Point);
}

void CRDVView::OnMButtonDblClk(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_MBUTTONDBLCLK,nFlags,Point);
	CScrollView::OnMButtonDblClk(nFlags,Point);
}

void CRDVView::OnMButtonDown(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_MBUTTONDOWN,nFlags,Point);
	CScrollView::OnMButtonDown(nFlags,Point);
}

void CRDVView::OnMButtonUp(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_MBUTTONUP,nFlags,Point);
	CScrollView::OnMButtonUp(nFlags,Point);
}

BOOL CRDVView::OnMouseWheel(UINT nFlags,short zDelta,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_MOUSEWHEEL,nFlags,Point,zDelta);
	return CScrollView::OnMouseWheel(nFlags,zDelta,Point);
}

void CRDVView::OnMouseMove(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_MOUSEMOVE,nFlags,Point);
	CScrollView::OnMouseMove(nFlags,Point);
}

void CRDVView::OnRButtonDblClk(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_RBUTTONDBLCLK,nFlags,Point);
	CScrollView::OnRButtonDblClk(nFlags,Point);
}

void CRDVView::OnRButtonDown(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_RBUTTONDOWN,nFlags,Point);
	CScrollView::OnRButtonDown(nFlags,Point);
}

void CRDVView::OnRButtonUp(UINT nFlags,CPoint Point)
{
	if (m_bConnected)
		SendMouseMessage(WM_RBUTTONUP,nFlags,Point);
	CScrollView::OnRButtonUp(nFlags,Point);
}

void CRDVView::OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags)
{
	if (m_bConnected)
		if (nChar < 256)
			SendKeyBoardMessage(WM_KEYDOWN,nChar,nRepCnt,nFlags);
	CScrollView::OnKeyDown(nChar,nRepCnt,nFlags);
}

void CRDVView::OnKeyUp(UINT nChar,UINT nRepCnt,UINT nFlags)
{
	if (m_bConnected)
		if (nChar < 256)
			SendKeyBoardMessage(WM_KEYUP,nChar,nRepCnt,nFlags);
	CScrollView::OnKeyUp(nChar,nRepCnt,nFlags);
}

// Send the mouse message
void CRDVView::SendMouseMessage(WORD wWM,UINT nFlags,CPoint Point,short zDelta)
{
	// Transmit the mouse event
	CPoint MousePosition = GetScrollPosition();

	// Test for zooming
	if (!m_bViewZoom)
	{
		// Get the client area
		CSize size,sizeSb;
		GetTrueClientSize(size,sizeSb);
		CRect Client;
		GetClientRect(&Client);

		// Get floating point versions of the coordinates
		double dPointX = (double)Point.x;
		double dPointY = (double)Point.y;
		double dWidth = (double)m_cxWidth;
		double dHeight = (double)m_cyHeight;
		double dSizeX = (double)Client.Width();
		double dSizeY = (double)Client.Height();

		// Adjust mouse coordinate to match the zoom out aspect ratio
		dPointX = dWidth / dSizeX * dPointX + 0.5;
		dPointY = dHeight / dSizeY * dPointY + 0.5;

		// Assign the new mouse coordinate
		Point.x = (LONG)dPointX;
		Point.y = (LONG)dPointY;
	}
	else
	{
		// Factor in the scroll positions
		Point += MousePosition;
	}

	// Test the keystate
	nFlags = 0;
	if (GetKeyState(VK_CONTROL) & 0x8000)
		nFlags |= 1;
	if (GetKeyState(VK_SHIFT) & 0x8000)
		nFlags |= 2;

	// Add the mouse message
	m_vMouseMsg.push_back(CMouseMsg(wWM,nFlags,Point,zDelta));
}

// Send the keyboard message
void CRDVView::SendKeyBoardMessage(WORD wWM,UINT nChar,UINT nRepCnt,UINT nFlags)
{
	// Add the KB message
	m_vKBMsg.push_back(CKBMsg(wWM,nChar,nRepCnt,nFlags));
}

// Handle the cursor
BOOL CRDVView::OnSetCursor(CWnd * pWnd,UINT nHitTest,UINT message)
{
	// Set the cursor
	if (nHitTest != HTCLIENT || (HCURSOR)m_Cursor == NULL)
		return CView::OnSetCursor(pWnd,nHitTest,message);
	SetClassLong(m_hWnd,-12,(LONG)(HCURSOR)m_Cursor);
	SetCursor(m_Cursor);
	return TRUE;
}

// Timer to handle mixing the windows message pump with thread messages
void CRDVView::OnTimer(UINT_PTR nIDEvent) 
{
	// Make sure we don't grow larger than server
	GetDocument()->UpdateAllViews(NULL);

	// Test the timer for different actions
	switch (nIDEvent)
	{
		// Close the document
		case 1:
		{
			// Kill the timer
			KillTimer(nIDEvent);

			// Close the document
			GetDocument()->OnCloseDocument();
			break;
		}

		case 2:
		{
			// Test for being done making the other connectionss
			if (m_iGridThread == m_nGridThreads)
			{
				// Done making connections, now receive data
				KillTimer(2);

				// Set the timer to send KB and Mouse messages
				SetTimer(4,0,NULL);

				// Send the verification to start getting screen updates
				CPacket Packet(m_nCompThreads,m_nSessionId - 1);

				// Send the packet
				m_Socket << Packet;

				// We are connected
				m_bConnected = TRUE;
			}
			else
			{
				// Create a new thread to manage the connection to the server for DIB updates
				CSocketWndSinkThread * pSinkThread = new CSocketWndSinkThread(&m_arrSinkHandles[m_iGridThread++],GetSafeHwnd(),GetDocument()->m_csIp,GetDocument()->m_csPort);

				// Connect to the server
				pSinkThread->PostThreadMessage(WM_CONNECTSERVER,0,0);

				// Track the connection
				m_vecSinkThreads.push_back(pSinkThread);

				// Kill the timer
				KillTimer(2);

				// Set the timer that tests the connection for being made
				SetTimer(3,0,NULL);
			}

			break;
		}

		// Test the connection
		case 3:
		{
			// Test the connection to the server
			CSocketWndSinkThread * pSinkThread = m_vecSinkThreads[m_vecSinkThreads.size() - 1];
			if (pSinkThread->IsConnected())
			{
				// Stop testing the connection to the server
				KillTimer(3);

				// Make the next connection to the server or start getting refreshes
				SetTimer(2,0,NULL);
			}
			break;
		}

		// KB and Mouse Messages
		case 4:
		{
			// Check for KB messages
			if (m_vKBMsg.size())
			{
				// Build a packet from the collection of KB messages
				CPacket Packet(m_vKBMsg);

				// Clear the collection of KB messages
				m_vKBMsg.clear();

				// Send the packet
				m_Socket << Packet;
			}
			
			// Check for mouse messages
			if (m_vMouseMsg.size())
			{
				// Build a packet from the collection of KB messages
				CPacket Packet(m_vMouseMsg);

				// Clear the collection of KB messages
				m_vMouseMsg.clear();

				// Send the packet
				m_Socket << Packet;
			}

			break;
		}

	default:
		break;
	}
}

// Handle sizing
void CRDVView::OnSize(UINT nType,int cx,int cy)
{
	// Call the base class to handle the scrolling
	CScrollView::OnSize(nType,cx,cy);

	// Update the view and make sure the view fits
	GetDocument()->UpdateAllViews(NULL);
}

// Make sure we don't grow larger than the server
void CRDVView::OnUpdate(CView * pSender,LPARAM lHint,CObject * pHint)
{
	if (m_cxWidth && m_cyHeight)
	{
		GetParentFrame()->RecalcLayout();
		ResizeParentToFit();
	}
}

// Toggle the zoom (either menu or toolbar)
void CRDVView::OnViewZoom()
{
	// Toggle the zoom
	m_bViewZoom = !m_bViewZoom;

	// Handle the changed condition
	if (m_bViewZoom)
	{
		// Provide a full view of the server with scroll bars
		SetScrollSizes(MM_TEXT,m_ViewSize);
		GetParentFrame()->RecalcLayout();
		ResizeParentToFit();
	}
	else
	{
		// Shrink to fit the server in the client window
		SetScaleToFitSize(m_ViewSize);
		GetParentFrame()->RecalcLayout();
	}
}

// Toggle the zoom UI (either menu or toolbar)
void CRDVView::OnUpdateViewZoom(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bConnected);
	pCmdUI->SetCheck(m_bConnected && m_bViewZoom);
}
