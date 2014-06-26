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
#include "DIBFrame.h"

// Construct the frame
CDIBFrame::CDIBFrame() : m_x(0), m_y(0), m_nBitCount(32),
	m_hBkgFrame(NULL), m_pBkgBits(NULL), m_hLastBkgFrame(NULL), 
	m_pBitmapInfo(NULL), m_pBitmapInfoHdr(NULL), m_dwBkgImageBytes(0)
{
}

// Construct the frame
CDIBFrame::CDIBFrame(int x,int y,int nBitCount) : m_x(x), m_y(y), m_nBitCount(nBitCount),
	m_hBkgFrame(NULL), m_pBkgBits(NULL), m_hLastBkgFrame(NULL), 
	m_pBitmapInfo(NULL), m_pBitmapInfoHdr(NULL), m_dwBkgImageBytes(0)
{
	// Create the frame
	CreateFrame();
}

// Construct the frame from the buffer
CDIBFrame::CDIBFrame(BYTE * pBuffer,DWORD dwLen)
{
	// Create the frame
	CreateFrame(pBuffer,dwLen);
}

// Copy/Assignment constructor
CDIBFrame::CDIBFrame(const CDIBFrame & rhs)
{
	*this = rhs;
}

// Deconstruct the frame
CDIBFrame::~CDIBFrame()
{
	// Delete the frame
	DeleteFrame();
}

// Copy/Assignment operator
CDIBFrame & CDIBFrame::operator = (const CDIBFrame & rhs)
{
	if (this != &rhs)
	{
		// Copy the dimensions
		m_x = rhs.m_x;
		m_y = rhs.m_y;
		m_nBitCount = rhs.m_nBitCount;

		// Create this frame from this others buffer
		const std::vector<BYTE> & Buffer = rhs.m_Buffer;
		const BYTE * pBuffer = NULL;
		DWORD dwLen = (DWORD)Buffer.size();
		if (dwLen)
			pBuffer = &Buffer[0];
		CreateFrame(pBuffer,dwLen);
	}
	return *this;
}

// Set the dimensions and create the frame
void CDIBFrame::Init(int x,int y,int nBitCount)
{
	// Test the dimensions
	if (x < 1 || y < 1)
		return;

	// Test for already being initialized
	if (m_FrameDC && m_x == x && m_y == y)
		return;

	// Set the new dimensions
	m_x = x;
	m_y = y;
	m_nBitCount = nBitCount;

	// Create the frame
	CreateFrame();
}

// Create the frame
void CDIBFrame::CreateFrame(const BYTE * pBuffer,DWORD dwLen)
{
	// Cleanup the last frame
	DeleteFrame();

	// Create a DC for the compatible display
	CDC DisplayDC;
	DisplayDC.Attach(GetDC(GetDesktopWindow()));

	// Create the last frame DC
	m_FrameDC.CreateCompatibleDC(&DisplayDC);
	if (m_FrameDC)
	{
		if (!pBuffer)
		{
			// Calculate the size of the bitmap info structure (header + color table)
			DWORD dwLen = (DWORD)((WORD)sizeof(BITMAPINFOHEADER) + (m_nBitCount == 32 ? 0 : (m_nBitCount == 8 ? 256 : 16)) * sizeof(RGBQUAD));

			// Allocate the bitmap structure
			m_Buffer.resize(dwLen,0);
			BYTE * pBuffer = &m_Buffer[0];

			// Set up the bitmap info structure for the DIB section
			m_pBitmapInfo = (BITMAPINFO*)pBuffer;
			m_pBitmapInfoHdr = (BITMAPINFOHEADER*)&(m_pBitmapInfo->bmiHeader);
			m_pBitmapInfoHdr->biSize = sizeof(BITMAPINFOHEADER);
			m_pBitmapInfoHdr->biWidth = m_x;
			m_pBitmapInfoHdr->biHeight = m_y;
			m_pBitmapInfoHdr->biPlanes = 1;
			m_pBitmapInfoHdr->biBitCount = m_nBitCount;
			m_pBitmapInfoHdr->biCompression = BI_RGB;

			
			if (m_nBitCount == 8)
			{
				// 256 color gray scale color table embedded in the DIB
				RGBQUAD Colors;
				for (int iColor = 0;iColor < 256;iColor++)
				{
					Colors.rgbBlue = Colors.rgbGreen = Colors.rgbRed = Colors.rgbReserved = iColor;
					m_pBitmapInfo->bmiColors[iColor] = Colors;
				}
				m_pBitmapInfo->bmiHeader.biClrUsed = 256;
			}
			else if (m_nBitCount == 4)
			{
				// 16 color gray scale color for low bandwidth
				RGBQUAD Colors;
				for (int iColor = 0;iColor < 16;iColor++)
				{
					Colors.rgbBlue = Colors.rgbGreen = Colors.rgbRed = Colors.rgbReserved = iColor * 16;
					m_pBitmapInfo->bmiColors[iColor] = Colors;
				}
				m_pBitmapInfo->bmiHeader.biClrUsed = 16;
			}

			// Create the DIB for the frame
			m_hBkgFrame = CreateDIBSection(m_FrameDC,m_pBitmapInfo,DIB_RGB_COLORS,(void**)&m_pBkgBits,NULL,0);

			// Prepare the frame DIB for painting
			m_hLastBkgFrame = (HBITMAP)m_FrameDC.SelectObject(m_hBkgFrame);

			// Initialize the DIB to black
			m_FrameDC.PatBlt(0,0,m_x,m_y,BLACKNESS);
		}
		else
		{
			// Copy the buffer
			m_Buffer.resize(dwLen,0);
			memcpy(&m_Buffer[0],pBuffer,dwLen);

			// Create the DIB from the buffer
			m_pBitmapInfo = (BITMAPINFO *)pBuffer;
			m_pBitmapInfoHdr = &m_pBitmapInfo->bmiHeader;
			m_hBkgFrame = CreateDIBSection(m_FrameDC,m_pBitmapInfo,DIB_RGB_COLORS,(void**)&m_pBkgBits,NULL,0);
			m_hLastBkgFrame = (HBITMAP)m_FrameDC.SelectObject(m_hBkgFrame);
			memcpy(m_pBkgBits,pBuffer + m_pBitmapInfoHdr->biSize,m_pBitmapInfoHdr->biSizeImage); 
		}

		// Get the byte storage amount for the DIB bits
		BITMAP bmMask;
		GetObject(m_hBkgFrame,sizeof(BITMAP),&bmMask);
		m_dwBkgImageBytes = bmMask.bmWidthBytes * bmMask.bmHeight;
	}

	// Delete the dc's
	ReleaseDC(GetDesktopWindow(),DisplayDC.Detach());
}

// Delete the frame
void CDIBFrame::DeleteFrame()
{
	// Check for a frame to cleanup
	if (m_FrameDC)
	{
		// UnSelect the DIB
		m_FrameDC.SelectObject(m_hLastBkgFrame);

		// Delete the DIB for the frame
		DeleteObject(m_hBkgFrame);
		m_pBkgBits = NULL;

		// Delete the last frame DC
		m_FrameDC.DeleteDC();
	}
}