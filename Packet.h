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

#include "AC.h"
#include "DIBFrame.h"
#include <vector>

// Class for containing the keyboard message
class CKBMsg
{
public:
	CKBMsg() {};
	CKBMsg(WORD wWM,UINT nChar,UINT nRepCnt,UINT nFlags) : m_wWM(wWM), m_nChar(nChar), m_nRepCnt(nRepCnt), m_nFlags(nFlags) {}
	CKBMsg(const CKBMsg & KBMsg) {*this = KBMsg;}
	CKBMsg & operator = (const CKBMsg & KBMsg)
	{
		if (this != &KBMsg)
		{
			m_wWM = KBMsg.m_wWM;
			m_nChar = KBMsg.m_nChar;
			m_nRepCnt = KBMsg.m_nRepCnt;
			m_nFlags = KBMsg.m_nFlags;
		}
		return *this;
	}
	bool operator == (const CKBMsg & rhs) const
	{
		if (this != &rhs)
		{
			return m_wWM == rhs.m_wWM &&
				m_nChar == rhs.m_nChar &&
				m_nRepCnt == rhs.m_nRepCnt &&
				m_nFlags == rhs.m_nFlags;
		}
		return true;
	}

	WORD m_wWM;
	UINT m_nChar;
	UINT m_nRepCnt;
	UINT m_nFlags;
};

// Class for containing the mouse message
class CMouseMsg
{
public:
	CMouseMsg() {};
	CMouseMsg(WORD wWM,UINT nFlags,CPoint MousePosition,short zDelta) : m_wWM(wWM), m_nFlags(nFlags), m_MousePosition(MousePosition), m_zDelta(zDelta) {}
	CMouseMsg(const CMouseMsg & MouseMsg) {*this = MouseMsg;}
	CMouseMsg & operator = (const CMouseMsg & MouseMsg)
	{
		if (this != &MouseMsg)
		{
			m_wWM = MouseMsg.m_wWM;
			m_nFlags = MouseMsg.m_nFlags;
			m_MousePosition = MouseMsg.m_MousePosition;
			m_zDelta = MouseMsg.m_zDelta;
		}
		return *this;
	}
	bool operator == (const CMouseMsg & rhs) const
	{
		if (this != &rhs)
		{
			return m_wWM == rhs.m_wWM &&
				m_nFlags == rhs.m_nFlags &&
				m_MousePosition.x == rhs.m_MousePosition.x &&
				m_MousePosition.y == rhs.m_MousePosition.y &&
				m_zDelta == rhs.m_zDelta;
		}
		return true;
	}

	WORD m_wWM;
	UINT m_nFlags;
	CPoint m_MousePosition;
	short m_zDelta;
};

// A packet makes up the information that will be sent and received
class CPacket
{
public:
	CPacket();
	CPacket(const CPacket & rhs);
	CPacket(CString csPassword,UINT nSessionId);
	CPacket(int nCompThreads,UINT nSessionId);
	CPacket(int cxScreen,int cyScreen,int nBitCount,int nGridThreads);
	CPacket(CRect Rect,char * pBuffer,DWORD dwBytes,DWORD dwSrcBytes,int nCompThreads,BOOL bImgDIB,BOOL bUseCompression,BOOL bAC,BOOL bXOR,BOOL bDIB);
	CPacket(std::vector<CKBMsg> vKBMsg);
	CPacket(std::vector<CMouseMsg> vMouseMsg);
	CPacket(DWORD dwFlags,DWORD dwXHotSpot,DWORD dwYHotSpot,int bmWidth,int bmHeight,WORD bmMaskPlanes,WORD bmMaskBitsPixel,WORD bmColorPlanes,WORD bmColorBitsPixel,std::vector<BYTE> MaskBits,std::vector<BYTE> ColorBits);
	CPacket & operator = (const CPacket & rhs);
	virtual ~CPacket();
	bool operator == (const CPacket & rhs) const;

	// Send the packet
	void SerializePacket(CAsyncSocket * pSocket,bool bSend);

	// Type of packet
	unsigned char m_ucPacketType;

	// m_ucPacketType = 1
	CString m_csPasswordHash;

	// m_ucPacketType = 10
	int m_nCompThreads;
	UINT m_nSessionId;

	// m_ucPacketType = 2
	int m_cxScreen;
	int m_cyScreen;
	int m_nBitCount;
	int m_nGridThreads;

	// m_ucPacketType = 7
	CRect m_Rect;
	DWORD m_dwSrcBytes,m_dwBytes;
	BOOL m_bImgDIB;
	BOOL m_bDIB;
	BOOL m_bUseCompression;
	BOOL m_bAC;
	BOOL m_bXOR;
	DWORD m_dwBufferSize;
	char * m_pBuffer;
	std::auto_ptr<char> m_Buffer;
	bool m_bDeleteBuffer;

	// Mouse and Keyboard windows message
	WORD m_wWM;
	UINT m_nFlags;

	// m_ucPacketType = 4
	std::vector<CKBMsg> m_vKBMsg;

	// m_ucPacketType = 5
	std::vector<CMouseMsg> m_vMouseMsg;

	// m_ucPacketType = 3
	DWORD m_dwFlags;
	DWORD m_dwXHotSpot;
	DWORD m_dwYHotSpot;
	int m_bmWidth;
	int m_bmHeight;
	WORD m_bmMaskPlanes;
	WORD m_bmMaskBitsPixel;
	WORD m_bmColorPlanes;
	WORD m_bmColorBitsPixel;
	std::vector<BYTE> m_MaskBits;
	std::vector<BYTE> m_ColorBits;
};