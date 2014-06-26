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

#include <afxwin.h>
#include <vector>

// Wrapper class for the Cursor
class CCursor
{
public:
	CCursor() : m_hCursor(NULL) {}
	~CCursor()
	{
		Destroy();
	}
	operator HCURSOR() {return m_hCursor;}
	void CreateCursor(DWORD dwXHotSpot,DWORD dwYHotSpot,int nWidth,int nHeight,WORD bmMaskPlanes,WORD bmMaskBitsPixel,WORD bmColorPlanes,WORD bmColorBitsPixel,BYTE * pMaskBits,BYTE * pColorBits)
	{
		// Clean up
		Destroy();

		// Create a mask bitmap
		HBITMAP hMask = NULL;
		CBitmap Mask;
		if (pMaskBits)
		{
			Mask.CreateBitmap(nWidth,nHeight,bmMaskPlanes,bmMaskBitsPixel,pMaskBits);
			hMask = (HBITMAP)Mask;
		}

		// Create a color bitmap
		HBITMAP hColor = NULL;
		CBitmap Color;
		if (pColorBits)
		{
			Color.CreateBitmap(nWidth,nHeight,bmColorPlanes,bmColorBitsPixel,pColorBits);
			hColor = (HBITMAP)Color;
		}

		// Create an Icon from the "color" and "mask" bitmaps
		ICONINFO IconInfo;
		IconInfo.fIcon = FALSE;
		IconInfo.xHotspot = dwXHotSpot;
		IconInfo.yHotspot = dwYHotSpot;
		IconInfo.hbmMask = hMask;
		IconInfo.hbmColor = hColor;

		// Create the cursor
		m_hCursor = CreateIconIndirect(&IconInfo);
	}

protected:
	void Destroy()
	{
		if (m_hCursor)
		{
			DestroyCursor(m_hCursor);
			m_hCursor = NULL;
		}
	}

private:
	HCURSOR m_hCursor;
};

class CDIBFrame
{
public:
	CDIBFrame();
	CDIBFrame(int x,int y,int nBitCount);
	CDIBFrame(BYTE * pBuffer,DWORD dwLen);
	CDIBFrame(const CDIBFrame & rhs);
	~CDIBFrame();

public:
	CDIBFrame & operator = (const CDIBFrame & rhs);
	operator HBITMAP () {return m_hBkgFrame;}
	operator HDC () {return (HDC)m_FrameDC;}
	operator CDC * () {return &m_FrameDC;}
	operator CDC & () {return m_FrameDC;}
	operator LPSTR () {return m_pBkgBits;}
	operator LPSTR * () {return &m_pBkgBits;}
	operator BITMAPINFO * () {return m_pBitmapInfo;}
	operator BITMAPINFOHEADER * () {return m_pBitmapInfoHdr;}

public:
	void Init(int x,int y,int nBitCount);
	void CreateFrame(const BYTE * pBuffer = NULL,DWORD dwLen = 0);
	void DeleteFrame();

public:
	int m_x,m_y,m_nBitCount;
	DWORD m_dwBkgImageBytes;

protected:
	CDC m_FrameDC;
	HBITMAP m_hBkgFrame;
	LPSTR m_pBkgBits;
	HBITMAP m_hLastBkgFrame;
	std::vector<BYTE> m_Buffer;
	BITMAPINFO * m_pBitmapInfo;
	BITMAPINFOHEADER * m_pBitmapInfoHdr;
};