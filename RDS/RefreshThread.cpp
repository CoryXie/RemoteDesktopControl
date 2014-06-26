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
#include "..\Common.h"
#include "RefreshThread.h"
#include "afxole.h"
#include "resource.h"

IMPLEMENT_DYNCREATE(CRefreshThread, CWinThread)

CRefreshThread::CRefreshThread() throw(...)
{
	throw;
}

// Take the update rectangle
CRefreshThread::CRefreshThread(HWND hWndParent,std::vector<CRect> vDIBRect,int nBitCount,BOOL bImgDIB,BOOL bUseCompression,BOOL bAC,DWORD dwThread,int nCompThreads) : 
	m_bPumpMessage(false), m_hWndParent(hWndParent),
	m_vDIBRect(vDIBRect), m_nBitCount(nBitCount), 
	m_bImgDIB(bImgDIB), m_bUseCompression(bUseCompression), m_bAC(bAC),
	m_dwThread(dwThread), m_iCurDIB(0), m_nDIB(0),
	m_nCompThreads(nCompThreads)
{
	// Create the event signaling event
	m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	// Create a DC for the display
	m_hDisplayDC = GetDC(GetDesktopWindow());

	// Get the number of iterations for the thread
	m_nDIB = (int)m_vDIBRect.size();

	// Calculate the basic sleep cycle
	m_dwSleep = (DWORD)(50.0 / (double)m_nDIB + .5);

	// Initialize the DIBs for each iteration of the thread
	for (int iDIB = 0;iDIB < m_nDIB;++iDIB)
	{
		// Create a DIB rectangle
		CRect DIBRect = m_vDIBRect[iDIB];
		int nWidth = DIBRect.Width();
		int nHeight = DIBRect.Height();

		// Create a DIB for a refresh rectangle
		CDIBFrame DIB(nWidth,nHeight,nBitCount);
		m_vDIB.push_back(DIB);
		m_vDIBLast.push_back(DIB);
		m_vComp.push_back(FALSE);
		m_vInit.push_back(FALSE);
		m_vDIBPacket.push_back(CPacket());
	}

	// Create the thread
	CreateThread();
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	// Wait for the thread to initialize and signal its message pump has started
	WaitForSingleObject(m_hEvent,INFINITE);

	// Close the initialization event
	CloseHandle(m_hEvent);
	m_hEvent = NULL;
}

CRefreshThread::~CRefreshThread()
{
	// Delete the dc's
	ReleaseDC(GetDesktopWindow(),m_hDisplayDC);

	// Signal the ending
	SetEvent(m_hThread);
}

BOOL CRefreshThread::InitInstance()
{
	// Initialize sockets for each thread
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	return TRUE;
}

int CRefreshThread::ExitInstance()
{
	// Shutdown the connections
	if (!m_setAccept.empty())
	{
		std::set<CTCPSocket *>::iterator itAccept = m_setAccept.begin();
		for (;itAccept != m_setAccept.end();++itAccept)
		{
			// Close the connection
			CTCPSocket * pAccept = *itAccept;
			OnCloseConn((WPARAM)pAccept,0);
		}
	}

	// Base class handles normal exit sequence
	return CWinThread::ExitInstance();
}

// Use the message pump as a marker to indicate the thread can accept posted messages
BOOL CRefreshThread::PumpMessage()
{
	// Test for initialization
	if (!m_bPumpMessage)
	{
		// The message pump is active
		m_bPumpMessage = true;

		// Signal completion of this stage
		SetEvent(m_hEvent);
	}

	// Allow the base class to handle the message pump
	return CWinThread::PumpMessage();
}

// These functions are called by PostThreadMessage
BEGIN_MESSAGE_MAP(CRefreshThread, CWinThread)
	ON_THREAD_MESSAGE(WM_ACCEPTCONN, &CRefreshThread::OnAcceptConn)
	ON_THREAD_MESSAGE(WM_IMAGEREFRESH, &CRefreshThread::OnImageRefresh)
	ON_THREAD_MESSAGE(WM_IMAGESEND, &CRefreshThread::OnImageSend)
	ON_THREAD_MESSAGE(WM_ENDTHREAD, &CRefreshThread::OnEndThread)
	ON_THREAD_MESSAGE(WM_CLOSECONN, &CRefreshThread::OnCloseConn)
END_MESSAGE_MAP()

// Integrate an accepted connection for updates
void CRefreshThread::OnAcceptConn(WPARAM wParam,LPARAM lParam)
{
	// Get the accepted socket that has been passed from the main thread
	SOCKET hAccept = (SOCKET)wParam;

	// Initialize the connection
	CTCPSocket * pAccept = new CTCPSocket();
	CTCPSocket & Accept = *pAccept;

	// Create the socket
	Accept.Create();

	// Attach the accepted connection
	Accept.Attach(hAccept,FD_CONNECT | FD_READ | FD_CLOSE);

	// Create a socket event sink for the accepted connection
	CSocketEvent * pAcceptEvent = new CSocketEvent(this);

	// Set the parent for receiving event notifications
	Accept.SetParent(pAcceptEvent->GetSafeHwnd());

	// Store the connection
	m_setAccept.insert(pAccept);

	// Reset the sequence (comparison and dib initialization) for this connection
	for (int iDIB = 0;iDIB < m_nDIB;++iDIB)
	{
		m_vComp[iDIB] = FALSE;
		m_vInit[iDIB] = FALSE;
	}

	// Tie the accepted connection to the event sink for the accepted connection
	m_mapAcceptEvent[pAccept] = pAcceptEvent;

	#if defined(_DEBUG)
	DebugMsg(_T("Thread %d Accepted Connection\n"), m_dwThread);
	#endif
}

// Test for needing to refresh a DIB rectangle
void CRefreshThread::OnImageRefresh(WPARAM wParam,LPARAM lParam)
{
	// Test for connections
	if (m_setAccept.empty())
	{
		// There are no connections so wait for 1/10 a second
		Sleep(100);

		// Notify parent to start the next sequence
		PostMessage(m_hWndParent,WM_IMAGERENEW,(WPARAM)this,(LPARAM)m_dwThread);
		return;
	}

	// Test for a DIB or Cursor
	if (m_nDIB)
	{
		// Get the DIB for this thread
		CDIBFrame & DIB = m_vDIB[m_iCurDIB];
		CDIBFrame & DIBLast = m_vDIBLast[m_iCurDIB];
		CRect & DIBRect = m_vDIBRect[m_iCurDIB];
		BOOL & bSame = m_vComp[m_iCurDIB];
		BOOL & bInit = m_vInit[m_iCurDIB];
		CPacket & DIBPacket = m_vDIBPacket[m_iCurDIB];

		// Capture the DIB
		BitBlt(DIB,0,0,DIBRect.Width(),DIBRect.Height(),m_hDisplayDC,DIBRect.left,DIBRect.top,SRCCOPY);

		// Initialize the input
		char * pInBuffer = (LPSTR)DIB;
		DWORD dwSrcBytes = DIB.m_dwBkgImageBytes;
		HBITMAP hDIB = DIB;

		// Test for having the initial image already
		if (bInit)
		{
			// Get the last image
			char * pLastBuffer = (LPSTR)DIBLast;
			DWORD dwLastBytes = DIBLast.m_dwBkgImageBytes;

			// Compare the bitmaps
			ASSERT(dwSrcBytes == dwLastBytes);
			bSame = memcmp(pInBuffer,pLastBuffer,min(dwSrcBytes,dwLastBytes)) == 0;
			if (!bSame)
			{
				// XOR the current display into the last display
				BitBlt(DIBLast,0,0,DIBRect.Width(),DIBRect.Height(),DIB,0,0,SRCINVERT);

				// Set the buffer as the next XOR'ed DIB
				pInBuffer = (LPSTR)DIBLast;
				hDIB = DIBLast;
				dwSrcBytes = DIBLast.m_dwBkgImageBytes;
			}
		}
		else
		{
			// Initialize the output buffer
			m_OutBuffer = std::auto_ptr<char>(new char[dwSrcBytes]);
		}

		// Test for a change in the current DIB with the last DIB
		if (!bSame)
		{
			// The output buffer and size
			char * pOutBuffer = NULL;
			DWORD dwOutBytes = 0;

			// JPEG support
			COleStreamFile ImageStream;
			IStream * pImageStream = NULL;
			HGLOBAL hImageBuffer = NULL;

			// Check for GDI+ conversion of the DIB
			if (!m_bImgDIB)
			{
				// Attach the DIB to the Image
				m_ImageDIB.Attach(hDIB);

				// Prepare the Image for memory stream serialization
				if (ImageStream.CreateMemoryStream())
				{
					// Serialize the DIB to a memory stream of a GDI+ image format
					pImageStream = ImageStream.GetStream();
					if (pImageStream && SUCCEEDED(m_ImageDIB.Save(pImageStream,Gdiplus::ImageFormatPNG)))
					{
						// Get the new input buffer size and buffer
						dwSrcBytes = (DWORD)ImageStream.GetLength();

						// Get the input buffer
						if (SUCCEEDED(GetHGlobalFromStream(pImageStream,&hImageBuffer)))
							pInBuffer = (char *)GlobalLock(hImageBuffer);
					}
				}

				// Detach from the DIB
				m_ImageDIB.Detach();
			}

			// Test for compression of the DIB
			if (m_bUseCompression)
			{
				// Set the output buffer
				pOutBuffer = m_OutBuffer.get();

				// Initialize the image information for compression
				CompressionInfo Info;
				Info.m_pInBuffer = pInBuffer;
				Info.m_dwSrcBytes = dwSrcBytes;
				Info.m_ppOutBuffer = &pOutBuffer;
				Info.m_pdwOutBytes = &dwOutBytes;

				// Compress the image in the main thread to auto-throttle the system from doing to many compressions at once
				SendMessage(m_hWndParent,WM_IMAGECOMPRESS,(WPARAM)&Info,0);
			}
			else
			{
				// Leave the DIB uncompressed
				pOutBuffer = pInBuffer;

				// Initialize the output bytes
				dwOutBytes = dwSrcBytes;
			}

			// Build an image packet
			DIBPacket = CPacket(DIBRect,pOutBuffer,dwOutBytes,dwSrcBytes,m_nCompThreads,m_bImgDIB,m_bUseCompression,m_bAC,bInit,m_bImgDIB);

			// Notify parent that an image is ready to be sent
			PostMessage(m_hWndParent,WM_IMAGEREFRESH,(WPARAM)this,(LPARAM)&DIBPacket);

			// Update the last DIB
			BitBlt(DIBLast,0,0,DIBRect.Width(),DIBRect.Height(),DIB,0,0,SRCCOPY);

			// Test for GDI+ cleanup
			if (!m_bImgDIB)
			{
				// Clean up the GDI+ image
				if (pInBuffer)
					GlobalUnlock(hImageBuffer);

				// Release the stream
				if (pImageStream)
					pImageStream->Release();
			}

			// The DIB is now a delta of the current and last DIB's
			bInit = TRUE;
		}

		// Update the current DIB
		m_iCurDIB++;
		if (m_iCurDIB == m_nDIB)
		{
			// Reset the count
			m_iCurDIB = 0;
		}

		// If nothing changed then signal the work as completed
		if (bSame)
		{
			// Sleep for a moment
			Sleep(m_dwSleep);

			// Notify parent to start the next sequence
			PostMessage(m_hWndParent,WM_IMAGERENEW,(WPARAM)this,(LPARAM)m_dwThread);
		}
	}
	else
	{
		// Test the cursor for a change
		bool bChange = false;

		// Get the cursor information
		CURSORINFO CursorInfo;
		CursorInfo.cbSize = sizeof(CURSORINFO);
		if (GetCursorInfo(&CursorInfo) && CursorInfo.hCursor)
		{
			// Get the icon information from the cursor handle
			ICONINFO IconInfo;
			if (GetIconInfo(CursorInfo.hCursor,&IconInfo))
			{
				// Get the cursor information flags
				DWORD dwFlags = CursorInfo.flags;

				// Get the hotspot
				DWORD dwXHotSpot = IconInfo.xHotspot;
				DWORD dwYHotSpot = IconInfo.yHotspot;

				// The width and height of the cursor
				int bmWidth = 0;
				int bmHeight = 0;

				// The planes and bits per pixel
				WORD bmMaskPlanes = 0,bmMaskBitsPixel = 0;
				WORD bmColorPlanes = 0,bmColorBitsPixel = 0;

				// Storage for the bitmap bits of the icon
				std::vector<BYTE> MaskBits,ColorBits;

				// Test for a bitmap handle
				bool bMask = false;
				if (IconInfo.hbmMask)
				{
					// Get the bitmap information from the handle
					BITMAP Mask;
					GetObject(IconInfo.hbmMask,sizeof(BITMAP),&Mask);

					// Get the width and height
					bmWidth = Mask.bmWidth;
					bmHeight = Mask.bmHeight;

					// Get the planes and bits per pixel
					bmMaskPlanes = Mask.bmPlanes;
					bmMaskBitsPixel = Mask.bmBitsPixel;

					// Get the bits of the temporary mask bitmap object
					DWORD dwMaskBytes = Mask.bmWidthBytes * Mask.bmHeight;
					MaskBits.resize(dwMaskBytes,0);
					GetBitmapBits(IconInfo.hbmMask,dwMaskBytes,&MaskBits[0]);

					// Free the bitmap
					DeleteObject(IconInfo.hbmMask);
					bMask = true;
				}

				// Test for a bitmap handle
				bool bColor = false;
				if (IconInfo.hbmColor)
				{
					// Get the bitmap information from the handle
					BITMAP Color;
					GetObject(IconInfo.hbmColor,sizeof(BITMAP),&Color);

					// Get the planes and bits per pixel
					bmColorPlanes = Color.bmPlanes;
					bmColorBitsPixel = Color.bmBitsPixel;

					// Get the bits of the temporary color bitmap object
					DWORD dwColorBytes = Color.bmWidthBytes * Color.bmHeight;
					ColorBits.resize(dwColorBytes,0);
					GetBitmapBits(IconInfo.hbmColor,dwColorBytes,&ColorBits[0]);

					// Free the bitmap
					DeleteObject(IconInfo.hbmColor);
					bColor = true;
				}

				// Test for a change in the cursor
				if (bMask && MaskBits != m_Cursor.m_MaskBits)
					bChange = true;
				if (bColor && !bChange && ColorBits != m_Cursor.m_ColorBits)
					bChange = true;

				// Test for sending a cursor
				if (bChange)
				{
					#if defined(_DEBUG)
					DebugMsg(_T("Cursor changed\n"));
					#endif

					// Update the last cursor data
					m_Cursor = CPacket(dwFlags,dwXHotSpot,dwYHotSpot,bmWidth,bmHeight,bmMaskPlanes,bmMaskBitsPixel,bmColorPlanes,bmColorBitsPixel,MaskBits,ColorBits);

					// Notify parent that a cursor is ready to be sent
					PostMessage(m_hWndParent,WM_IMAGEREFRESH,(WPARAM)this,(LPARAM)&m_Cursor);
				}
				else
				{
					// Sleep
					Sleep(100);

					// Notify parent to start the next sequence
					PostMessage(m_hWndParent,WM_IMAGERENEW,(WPARAM)this,(LPARAM)m_dwThread);
				}
			}
		}
	}
}

// Send the image to all connected clients
void CRefreshThread::OnImageSend(WPARAM wParam,LPARAM lParam)
{
	// Get the DIB packet to send
	CPacket * pDIBPacket = (CPacket *)wParam;
	CPacket & DIBPacket = *pDIBPacket;

	#if defined(_DEBUG)
	if (m_nDIB)
		DebugMsg(_T("Rect: (%d,%d)-(%d,%d) Uncompressed: %d Compressed: %d\n"), DIBPacket.m_Rect.left, DIBPacket.m_Rect.top, DIBPacket.m_Rect.right, DIBPacket.m_Rect.bottom, DIBPacket.m_dwSrcBytes, DIBPacket.m_dwBytes);
	#endif

	// Send the update to all connected clients
	bool bSent = false;
	std::set<CTCPSocket *>::iterator itAccept = m_setAccept.begin();
	for (;itAccept != m_setAccept.end();++itAccept)
	{
		// Get the connection
		CTCPSocket * pAccept = *itAccept;
		CTCPSocket & Accept = *pAccept;

		// Look up the event handler for the socket
		std::map<CTCPSocket *,CSocketEvent *>::iterator itAcceptEvent = m_mapAcceptEvent.find(pAccept);
		if (itAcceptEvent != m_mapAcceptEvent.end())
		{
			// Get the event handler for the socket
			CSocketEvent * pAcceptEvent = itAcceptEvent->second;
			if (!pAcceptEvent->IsClosed())
			{
				// Send the update to the open connection
				Accept << DIBPacket;
				bSent = true;
			}
		}
	}

	// If an image was sent then renew the sequence
	if (bSent)
	{
		// Sleep for a moment
		Sleep(m_dwSleep);

		// Notify parent to start the next sequence
		PostMessage(m_hWndParent,WM_IMAGERENEW,(WPARAM)this,(LPARAM)m_dwThread);
	}
}

// The client closed the connection closed
void CRefreshThread::OnCloseConn(WPARAM wParam,LPARAM lParam)
{
	// Get the socket
	CTCPSocket * pAccept = (CTCPSocket *)wParam;

	// Look up the event handler for the socket
	std::map<CTCPSocket *,CSocketEvent *>::iterator itAcceptEvent;
	itAcceptEvent = m_mapAcceptEvent.find(pAccept);
	if (itAcceptEvent != m_mapAcceptEvent.end())
	{
		// Get the event handler for the socket
		CSocketEvent * pAcceptEvent = itAcceptEvent->second;
		ASSERT(pAcceptEvent->IsClosed());

		// Cleanup event notification
		pAcceptEvent->DestroyWindow();
		delete pAcceptEvent;
		m_mapAcceptEvent.erase(itAcceptEvent);
	}

	// Find the connection for removal
	std::set<CTCPSocket *>::iterator itAccept = m_setAccept.find(pAccept);
	if (itAccept != m_setAccept.end())
	{
		// Shutdown and close the connection
		pAccept->ShutDown();
		pAccept->Close();

		// Delete the connection
		delete pAccept;

		// Remove the connection
		m_setAccept.erase(itAccept);
	}
}

void CRefreshThread::OnEndThread(WPARAM wParam,LPARAM lParam)
{
	// End the thread
	PostQuitMessage(0);
}
