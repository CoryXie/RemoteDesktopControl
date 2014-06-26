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

// Packet.cpp : implementation file
//

#include "stdafx.h"
#include "Packet.h"
#include "AC.h"
#include "Hash.h"
#include "Common.h"
#include <vector>

// Helper class to automatically free the memory attached to a CMemFile
class CMemFileEx : public CMemFile
{
public:
	CMemFileEx(BYTE * lpBuffer,UINT nBufferSize) : 
		CMemFile(lpBuffer,nBufferSize,0)
	{
		m_bAutoDelete = TRUE;
	}
};

// Generic incoming packet
CPacket::CPacket() : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 0;

	// Initialize the buffer
	m_dwBytes = 0;
	m_dwBufferSize = 0;
	m_pBuffer = NULL;
}

// The password packet
CPacket::CPacket(CString csPassword,UINT nSessionId) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 1;

	// Copy the data
	CHash Pwd;
	Pwd.HashData((BYTE *)csPassword.GetBuffer(),csPassword.GetLength());
	Pwd.GetHash(m_csPasswordHash);
	m_nSessionId = nSessionId;
}

// The compression threads packet and session id
CPacket::CPacket(int nCompThreads,UINT nSessionId) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 10;

	// Copy the data
	m_nCompThreads = nCompThreads;
	m_nSessionId = nSessionId;
}

// The display attributes packet
CPacket::CPacket(int cxScreen,int cyScreen,int nBitCount,int nGridThreads) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 2;

	// Copy the data
	m_cxScreen = cxScreen;
	m_cyScreen = cyScreen;
	m_nBitCount = nBitCount;
	m_nGridThreads = nGridThreads;
}

// The DIB packet
CPacket::CPacket(CRect Rect,char * pBuffer,DWORD dwBytes,DWORD dwSrcBytes,int nCompThreads,BOOL bImgDIB,BOOL bUseCompression,BOOL bAC,BOOL bXOR,BOOL bDIB) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 7;

	// Copy the data
	m_Rect = Rect;
	m_bImgDIB = bImgDIB;
	m_bUseCompression = bUseCompression;
	m_bAC = bAC;
	m_bXOR = bXOR;
	m_bDIB = bDIB;
	m_pBuffer = pBuffer;
	m_dwBytes = dwBytes;
	m_dwSrcBytes = dwSrcBytes;
	m_nCompThreads = nCompThreads;
}

// The collection of keyboard events
CPacket::CPacket(std::vector<CKBMsg> vKBMsg) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 4;

	// Copy the data
	m_vKBMsg = vKBMsg;
}

// The collection of mouse events
CPacket::CPacket(std::vector<CMouseMsg> vMouseMsg) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 5;

	// Copy the data
	m_vMouseMsg = vMouseMsg;
}

// The cursor packet
CPacket::CPacket(DWORD dwFlags,DWORD dwXHotSpot,DWORD dwYHotSpot,int bmWidth,int bmHeight,WORD bmMaskPlanes,WORD bmMaskBitsPixel,WORD bmColorPlanes,WORD bmColorBitsPixel,std::vector<BYTE> MaskBits,std::vector<BYTE> ColorBits) : m_bDeleteBuffer(false)
{
	// Set the packet type
	m_ucPacketType = 3;

	// Copy the data
	m_dwFlags = dwFlags;
	m_dwXHotSpot = dwXHotSpot;
	m_dwYHotSpot = dwYHotSpot;
	m_bmWidth = bmWidth;
	m_bmHeight = bmHeight;
	m_bmMaskPlanes = bmMaskPlanes;
	m_bmMaskBitsPixel = bmMaskBitsPixel;
	m_bmColorPlanes = bmColorPlanes;
	m_bmColorBitsPixel = bmColorBitsPixel;
	m_MaskBits = MaskBits;
	m_ColorBits = ColorBits;
}

// Copy/Assignment constructor
CPacket::CPacket(const CPacket & rhs)
{
	*this = rhs;
}

// Cleanup
CPacket::~CPacket()
{
	if (m_bDeleteBuffer)
	{
		delete [] m_pBuffer;
		m_pBuffer = NULL;
	}
}

// Copy/Assignment operator
CPacket & CPacket::operator = (const CPacket & rhs)
{
	if (this != &rhs)
	{
		// Assign the packet type
		m_ucPacketType = rhs.m_ucPacketType;
		m_bDeleteBuffer = false;

		// Detect the type of assignment
		switch (m_ucPacketType)
		{
			// Assign the password hash
			case 1:
			{
				m_csPasswordHash = rhs.m_csPasswordHash;
				m_nSessionId = rhs.m_nSessionId;
				break;
			}

			// Assign the region and bit depth
			case 2:
			{
				m_cxScreen = rhs.m_cxScreen;
				m_cyScreen = rhs.m_cyScreen;
				m_nBitCount = rhs.m_nBitCount;
				m_nGridThreads = rhs.m_nGridThreads;
				break;
			}

			// Assign the cursor
			case 3:
			{
				m_dwFlags = rhs.m_dwFlags;
				m_dwXHotSpot = rhs.m_dwXHotSpot;
				m_dwYHotSpot = rhs.m_dwYHotSpot;
				m_bmWidth = rhs.m_bmWidth;
				m_bmHeight = rhs.m_bmHeight;
				m_bmMaskPlanes = rhs.m_bmMaskPlanes;
				m_bmMaskBitsPixel = rhs.m_bmMaskBitsPixel;
				m_bmColorPlanes = rhs.m_bmColorPlanes;
				m_bmColorBitsPixel = rhs.m_bmColorBitsPixel;
				m_MaskBits = rhs.m_MaskBits;
				m_ColorBits = rhs.m_ColorBits;
				break;
			}

			// Assign the collection
			case 4:
			{
				m_vKBMsg = rhs.m_vKBMsg;
				break;
			}

			// Assign the collection
			case 5:
			{
				m_vMouseMsg = rhs.m_vMouseMsg;
				break;
			}

			// Assign the DIB
			case 7:
			{
				m_Rect = rhs.m_Rect;
				m_bImgDIB = rhs.m_bImgDIB;
				m_bUseCompression = rhs.m_bUseCompression;
				m_bAC = rhs.m_bAC;
				m_bXOR = rhs.m_bXOR;
				m_bDIB = rhs.m_bDIB;
				
				// Grow the buffer as necessary
				if (m_dwBufferSize < rhs.m_dwBytes)
				{
					m_dwBufferSize = rhs.m_dwBytes;
					if (m_pBuffer)
					{
						delete [] m_pBuffer;
						m_pBuffer = NULL;
					}
				}
				m_dwBytes = rhs.m_dwBytes;
				if (!m_pBuffer)
					m_pBuffer = new char[m_dwBufferSize];
				memcpy(m_pBuffer,rhs.m_pBuffer,m_dwBytes);
				m_dwSrcBytes = rhs.m_dwSrcBytes;
				m_nCompThreads = rhs.m_nCompThreads;
				m_bDeleteBuffer = true;
				break;
			}

			// Assign the compression threads and session id
			case 10:
			{
				m_nCompThreads = rhs.m_nCompThreads;
				m_nSessionId = rhs.m_nSessionId;
				break;
			}

			default:
			{
				m_dwBufferSize = rhs.m_dwBufferSize;
				m_dwBytes = rhs.m_dwBytes;
				m_pBuffer = rhs.m_pBuffer;
				m_bDeleteBuffer = rhs.m_bDeleteBuffer;
				break;
			}
		}
	}
	return *this;
}

// Equality test
bool CPacket::operator == (const CPacket & rhs) const
{
	// Objects are equal to themselves
	bool bEqual = true;

	// Identity test
	if (this != &rhs)
	{
		// Packet type test
		bEqual = m_ucPacketType == rhs.m_ucPacketType;
		if (bEqual)
		{
			// Detect the type of equivalence test
			switch (m_ucPacketType)
			{
				// Test the password
				case 1:
				{
					bEqual = m_csPasswordHash == rhs.m_csPasswordHash &&
						m_nSessionId == rhs.m_nSessionId;
					break;
				}

				// Test the region and bit depth
				case 2:
				{
					bEqual = m_cxScreen == rhs.m_cxScreen && 
						m_cyScreen == rhs.m_cyScreen &&
						m_nBitCount == rhs.m_nBitCount &&
						m_nGridThreads == rhs.m_nGridThreads;
					break;
				}

				// Test the cursor
				case 3:
				{
					bEqual = m_dwFlags == rhs.m_dwFlags &&
						m_dwXHotSpot == rhs.m_dwXHotSpot &&
						m_dwYHotSpot == rhs.m_dwYHotSpot &&
						m_bmWidth == rhs.m_bmWidth &&
						m_bmHeight == rhs.m_bmHeight &&
						m_bmMaskPlanes == rhs.m_bmMaskPlanes &&
						m_bmMaskBitsPixel == rhs.m_bmMaskBitsPixel &&
						m_bmColorPlanes == rhs.m_bmColorPlanes &&
						m_bmColorBitsPixel == rhs.m_bmColorBitsPixel &&
						m_MaskBits == rhs.m_MaskBits &&
						m_ColorBits == rhs.m_ColorBits;
					break;
				}

				// Test the collection
				case 4:
				{
					bEqual = m_vKBMsg == rhs.m_vKBMsg;
					break;
				}

				// Test the collection
				case 5:
				{
					bEqual = m_vMouseMsg == rhs.m_vMouseMsg;
					break;
				}

				// Test the DIB
				case 7:
				{
					bEqual = m_Rect == rhs.m_Rect &&
						m_bImgDIB == rhs.m_bImgDIB &&
						m_bUseCompression == rhs.m_bUseCompression &&
						m_bAC == rhs.m_bAC &&
						m_bXOR == rhs.m_bXOR &&
						m_bDIB == rhs.m_bDIB &&
						m_dwBytes == rhs.m_dwBytes &&
						memcmp(m_pBuffer,rhs.m_pBuffer,min(m_dwBytes,rhs.m_dwBytes)) == 0 &&
						m_dwSrcBytes == rhs.m_dwSrcBytes &&
						m_nCompThreads == rhs.m_nCompThreads;
					break;
				}

				// Test the compression threads and session id
				case 10:
				{
					bEqual = m_nCompThreads == rhs.m_nCompThreads &&
						m_nSessionId == rhs.m_nSessionId;
					break;
				}

				default:
					break;
			}
		}
	}
	return bEqual;
}

// Send/Receive Data
void CPacket::SerializePacket(CAsyncSocket * pSocket,bool bSend)
{
	if (bSend)
	{
		// The cache for building up a block of data to send
		CMemFile SendCache;

		// Write the placeholder for the packet size
		UINT uiLen = 0;
		SendCache.Write(&uiLen,sizeof(uiLen));

		// Write the packet type
		SendCache.Write(&m_ucPacketType,sizeof(m_ucPacketType));

		switch (m_ucPacketType)
		{
			// The password
			case 1:
			{
				// Write the password hash length
				int nHashLen = m_csPasswordHash.GetLength();
				SendCache.Write(&nHashLen,sizeof(nHashLen));

				// Write the password hash data
				if (nHashLen)
					SendCache.Write((LPCTSTR)m_csPasswordHash,nHashLen);

				// Write the session id
				SendCache.Write(&m_nSessionId,sizeof(m_nSessionId));
				break;
			}

			// The dimensions
			case 2:
			{
				// Write the horizontal size
				SendCache.Write(&m_cxScreen,sizeof(m_cxScreen));

				// Write the vertical size
				SendCache.Write(&m_cyScreen,sizeof(m_cyScreen));

				// Write the bit depth
				SendCache.Write(&m_nBitCount,sizeof(m_nBitCount));

				// Write the number of threads handling the update rectangles
				SendCache.Write(&m_nGridThreads,sizeof(m_nGridThreads));
				break;
			}

			// The cursor
			case 3:
			{
				// Write the cursor flags
				SendCache.Write(&m_dwFlags,sizeof(m_dwFlags));

				// Write the X hot spot
				SendCache.Write(&m_dwXHotSpot,sizeof(m_dwXHotSpot));

				// Write the Y hot spot
				SendCache.Write(&m_dwYHotSpot,sizeof(m_dwYHotSpot));

				// Write the cursor width
				SendCache.Write(&m_bmWidth,sizeof(m_bmWidth));

				// Write the cursor height
				SendCache.Write(&m_bmHeight,sizeof(m_bmHeight));

				// Write the mask planes
				SendCache.Write(&m_bmMaskPlanes,sizeof(m_bmMaskPlanes));

				// Write the mask bits per pixel
				SendCache.Write(&m_bmMaskBitsPixel,sizeof(m_bmMaskBitsPixel));

				// Write the color planes
				SendCache.Write(&m_bmColorPlanes,sizeof(m_bmColorPlanes));

				// Write the color bits per pixel
				SendCache.Write(&m_bmColorBitsPixel,sizeof(m_bmColorBitsPixel));

				// Write the mask bitmap size
				WORD wMaskSize = (WORD)m_MaskBits.size();
				SendCache.Write(&wMaskSize,sizeof(wMaskSize));

				// Write the mask bitmap
				if (wMaskSize)
					SendCache.Write(&m_MaskBits[0],wMaskSize);

				// Write the color bitmap size
				WORD wColorSize = (WORD)m_ColorBits.size();
				SendCache.Write(&wColorSize,sizeof(wColorSize));

				// Write the color bitmap
				if (wColorSize)
					SendCache.Write(&m_ColorBits[0],wColorSize);
				break;
			}

			// The collection of keyboard messages
			case 4:
			{
				// Write the number of messages
				UINT nKBMsg = (UINT)m_vKBMsg.size();
				SendCache.Write(&nKBMsg,sizeof(nKBMsg));

				// Write each message
				for (UINT iKBMsg = 0;iKBMsg < nKBMsg;++iKBMsg)
				{
					// Get the KB message
					CKBMsg & KBMsg = m_vKBMsg[iKBMsg];

					// Write the keyboard message
					SendCache.Write(&(KBMsg.m_wWM),sizeof(KBMsg.m_wWM));

					// Write the keyboard character
					SendCache.Write(&(KBMsg.m_nChar),sizeof(KBMsg.m_nChar));

					// Write the repeat count
					SendCache.Write(&(KBMsg.m_nRepCnt),sizeof(KBMsg.m_nRepCnt));

					// Write the flags
					SendCache.Write(&(KBMsg.m_nFlags),sizeof(KBMsg.m_nFlags));
				}
				break;
			}

			// The collection of mouse messages
			case 5:
			{
				// Write the number of messages
				UINT nMouseMsg = (UINT)m_vMouseMsg.size();
				SendCache.Write(&nMouseMsg,sizeof(nMouseMsg));

				// Write each message
				for (UINT iMouseMsg = 0;iMouseMsg < nMouseMsg;++iMouseMsg)
				{
					// Get the Mouse message
					CMouseMsg & MouseMsg = m_vMouseMsg[iMouseMsg];

					// Write the mouse message
					SendCache.Write(&(MouseMsg.m_wWM),sizeof(MouseMsg.m_wWM));

					// Write the flags
					SendCache.Write(&(MouseMsg.m_nFlags),sizeof(MouseMsg.m_nFlags));

					// Write the mouse position
					SendCache.Write(&(MouseMsg.m_MousePosition),sizeof(MouseMsg.m_MousePosition));

					// Write the mouse wheel delta
					SendCache.Write(&(MouseMsg.m_zDelta),sizeof(MouseMsg.m_zDelta));
				}
				break;
			}

			// The DIB
			case 7:
			{
				// Write the image type (DIB = True, JPG = FALSE)
				SendCache.Write(&m_bDIB,sizeof(m_bDIB));

				// Write the XOR attribute
				SendCache.Write(&m_bXOR,sizeof(m_bXOR));

				// Write the image type
				SendCache.Write(&m_bImgDIB,sizeof(m_bImgDIB));

				// Write the compressed attribute
				SendCache.Write(&m_bUseCompression,sizeof(m_bUseCompression));

				// Write the type of compression attribute
				SendCache.Write(&m_bAC,sizeof(m_bAC));

				// Write the number of threads used to compress the image
				SendCache.Write(&m_nCompThreads,sizeof(m_nCompThreads));

				// Write the update coordinates
				SendCache.Write(&m_Rect,sizeof(m_Rect));

				// Write the DIB source size if ZLIB
				if (!m_bAC)
					SendCache.Write(&m_dwSrcBytes,sizeof(m_dwSrcBytes));

				// Write the DIB size
				SendCache.Write(&m_dwBytes,sizeof(m_dwBytes));

				// Write the DIB
				SendCache.Write(m_pBuffer,m_dwBytes);
				break;
			}

			// Compression threads
			case 10:
			{
				// Write the number of compression threads
				SendCache.Write(&m_nCompThreads,sizeof(m_nCompThreads));

				// Write the session id
				SendCache.Write(&m_nSessionId,sizeof(m_nSessionId));
				break;
			}

			// Fatal internal error
			default:
			{
				throw;
				break;
			}
		}

		// Get the buffer length and buffer
		uiLen = (UINT)SendCache.GetLength();
		BYTE * pBuffer = SendCache.Detach();

		// Autodelete the allocation
		CMemFileEx Buffer(pBuffer,uiLen);

		// Write in the packet size
		Buffer.Write(&uiLen,sizeof(uiLen));

		// Send the packet
		pSocket->Send(pBuffer,uiLen);
	}
	else
	{
		// Receive the packet size
		UINT uiLen = 0;
		if (!pSocket->Receive(&uiLen,sizeof(uiLen)))
			return;
		uiLen -= sizeof(uiLen);

		// The cache for receiving a block of sent data
		BYTE * pRecvCache = (BYTE *)malloc(uiLen);
		
		// Prepare the cache for reading
		CMemFileEx RecvCache(pRecvCache,uiLen);

		// Receive the packet
		if (!pSocket->Receive(pRecvCache,uiLen))
			return;

		// Read the packet type
		RecvCache.Read(&m_ucPacketType,sizeof(m_ucPacketType));

		switch (m_ucPacketType)
		{
			// The password
			case 1:
			{
				// Read the password hash length
				int nHashLen;
				RecvCache.Read(&nHashLen,sizeof(nHashLen));

				// Test for a password hash
				if (nHashLen)
				{
					// Read the password hash
					RecvCache.Read(m_csPasswordHash.GetBufferSetLength(nHashLen),nHashLen);
					m_csPasswordHash.ReleaseBuffer();
				}

				// Read the session id
				RecvCache.Read(&m_nSessionId,sizeof(m_nSessionId));
				break;
			}

			// The dimensions
			case 2:
			{
				// Read the horizontal width
				RecvCache.Read(&m_cxScreen,sizeof(m_cxScreen));

				// Read the vertical width
				RecvCache.Read(&m_cyScreen,sizeof(m_cyScreen));

				// Read the bit count
				RecvCache.Read(&m_nBitCount,sizeof(m_nBitCount));

				// Read the number of threads handling the update rectangles
				RecvCache.Read(&m_nGridThreads,sizeof(m_nGridThreads));
				break;
			}

			// The cursor
			case 3:
			{
				// Read the cursor flags
				RecvCache.Read(&m_dwFlags,sizeof(m_dwFlags));

				// Read the X hot spot
				RecvCache.Read(&m_dwXHotSpot,sizeof(m_dwXHotSpot));

				// Read the Y hot spot
				RecvCache.Read(&m_dwYHotSpot,sizeof(m_dwYHotSpot));

				// Read the cursor width
				RecvCache.Read(&m_bmWidth,sizeof(m_bmWidth));

				// Read the cursor height
				RecvCache.Read(&m_bmHeight,sizeof(m_bmHeight));

				// Read the mask planes
				RecvCache.Read(&m_bmMaskPlanes,sizeof(m_bmMaskPlanes));

				// Read the mask bits per pixel
				RecvCache.Read(&m_bmMaskBitsPixel,sizeof(m_bmMaskBitsPixel));

				// Read the color planes
				RecvCache.Read(&m_bmColorPlanes,sizeof(m_bmColorPlanes));

				// Read the color bits per pixel
				RecvCache.Read(&m_bmColorBitsPixel,sizeof(m_bmColorBitsPixel));

				// Read the mask bitmap size
				WORD wMaskSize = 0;
				RecvCache.Read(&wMaskSize,sizeof(wMaskSize));

				// Read the mask bitmap
				if (wMaskSize)
				{
					m_MaskBits.resize(wMaskSize,0);
					RecvCache.Read(&m_MaskBits[0],wMaskSize);
				}

				// Read the color bitmap size
				WORD wColorSize = 0;
				RecvCache.Read(&wColorSize,sizeof(wColorSize));

				// Read the color bitmap
				if (wColorSize)
				{
					m_ColorBits.resize(wColorSize,0);
					RecvCache.Read(&m_ColorBits[0],wColorSize);
				}
				break;
			}

			// The collection of keyboard messages
			case 4:
			{
				// Read the number of messages
				UINT nKBMsg;
				RecvCache.Read(&nKBMsg,sizeof(nKBMsg));

				// Read each message
				for (UINT iKBMsg = 0;iKBMsg < nKBMsg;++iKBMsg)
				{
					// Create the KB message
					CKBMsg KBMsg;

					// Read the keyboard message
					RecvCache.Read(&(KBMsg.m_wWM),sizeof(KBMsg.m_wWM));

					// Read the keyboard character
					RecvCache.Read(&(KBMsg.m_nChar),sizeof(KBMsg.m_nChar));

					// Read the repeat count
					RecvCache.Read(&(KBMsg.m_nRepCnt),sizeof(KBMsg.m_nRepCnt));

					// Read the flags
					RecvCache.Read(&(KBMsg.m_nFlags),sizeof(KBMsg.m_nFlags));

					// Add the KB message
					m_vKBMsg.push_back(KBMsg);
				}
				break;
			}

			// The collection of mouse messages
			case 5:
			{
				// Read the number of messages
				UINT nMouseMsg;
				RecvCache.Read(&nMouseMsg,sizeof(nMouseMsg));

				// Read each message
				for (UINT iMouseMsg = 0;iMouseMsg < nMouseMsg;++iMouseMsg)
				{
					// Create the Mouse message
					CMouseMsg MouseMsg;

					// Read the keyboard message
					RecvCache.Read(&(MouseMsg.m_wWM),sizeof(MouseMsg.m_wWM));

					// Read the flags
					RecvCache.Read(&(MouseMsg.m_nFlags),sizeof(MouseMsg.m_nFlags));

					// Read the mouse position
					RecvCache.Read(&(MouseMsg.m_MousePosition),sizeof(MouseMsg.m_MousePosition));

					// Read the mouse wheel
					RecvCache.Read(&(MouseMsg.m_zDelta),sizeof(MouseMsg.m_zDelta));

					// Add the mouse message
					m_vMouseMsg.push_back(MouseMsg);
				}
				break;
			}

			// The DIB
			case 7:
			{
				// Read the image type (DIB = True, JPG = FALSE)
				RecvCache.Read(&m_bDIB,sizeof(m_bDIB));

				// Read the XOR attribute
				RecvCache.Read(&m_bXOR,sizeof(m_bXOR));

				// Read the image type
				RecvCache.Read(&m_bImgDIB,sizeof(m_bImgDIB));

				// Read the compression attribute
				RecvCache.Read(&m_bUseCompression,sizeof(m_bUseCompression));

				// Read the type of compression attribute
				RecvCache.Read(&m_bAC,sizeof(m_bAC));

				// Read the number of compression threads
				RecvCache.Read(&m_nCompThreads,sizeof(m_nCompThreads));

				// Read the display band
				RecvCache.Read(&m_Rect,sizeof(m_Rect));

				// Read the DIB source size if ZLIB
				if (!m_bAC)
					RecvCache.Read(&m_dwSrcBytes,sizeof(m_dwSrcBytes));

				// Read the DIB size
				DWORD dwBytes = 0;
				RecvCache.Read(&dwBytes,sizeof(dwBytes));

				// Set decoded DIB for cleanup at dtor
				m_Buffer = std::auto_ptr<char>(new char[dwBytes]);
				m_pBuffer = m_Buffer.get();

				// Read the DIB
				RecvCache.Read(m_pBuffer,dwBytes);
				m_dwBytes = dwBytes;
				break;
			}

			// The compression threads
			case 10:
			{
				// Read the number of compression threads
				RecvCache.Read(&m_nCompThreads,sizeof(m_nCompThreads));

				// Read the session id
				RecvCache.Read(&m_nSessionId,sizeof(m_nSessionId));
				break;
			}

			// Fatal internal error
			default:
			{
				throw;
				break;
			}
		}
	}
}
