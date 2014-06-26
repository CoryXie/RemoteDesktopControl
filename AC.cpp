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
#include "AC.h"
#include "Common.h"

#include <memory>
using namespace std;

CArithmeticEncoder::CArithmeticEncoder() : 
	m_pBufferOut(NULL), m_ulnDestCount(0), m_ulnLastBuffer(0)
{
}

CArithmeticEncoder::CArithmeticEncoder(char * pBufferIn,unsigned long ulnSrcCount) :
	m_pBufferIn(pBufferIn), m_ulnSrcCount(ulnSrcCount), m_pBufferOut(NULL), m_ulnDestCount(0), m_ulnLastBuffer(0)
{
}

// Set the input buffer, clear the output
void CArithmeticEncoder::SetBuffer(char * pBufferIn,unsigned long ulnSrcCount)
{
	// Set the buffer
	m_pBufferIn = pBufferIn;
	m_ulnSrcCount = ulnSrcCount;
	m_ulnDestCount = 0;
}

// Get the output buffer
void CArithmeticEncoder::GetBuffer(char * & pBufferOut,unsigned long & ulnDestCount)
{
	// Set the buffer
	pBufferOut = m_pBufferOut;
	ulnDestCount = m_ulnDestCount;
}

CArithmeticEncoder::~CArithmeticEncoder()
{
	// Cleanup the output
	if (m_pBufferOut)
		delete [] m_pBufferOut;
}

// Initialze the modeling data
void CArithmeticEncoder::InitModel()
{
	memset(&m_ac,0,sizeof(m_ac));
	memset(&m_ac2,0,sizeof(m_ac2));
	memset(&m_ar,0,sizeof(m_ar));
	memset(&m_aMap,0,sizeof(m_aMap));
}

// Scale the counts for AC
__inline void CArithmeticEncoder::ScaleCounts()
{
	// Make a copy for 0 count checking
	for (int i = 0;i < 256;++i)
		m_ac2[i] = m_ac[i];

	// Scale to restrict to 14 bits for precision
	for (;;)
	{
		unsigned int uiTotal = 0;
		for (int i = 0;i < 256;++i)
		{
			uiTotal += m_ac2[i];
			if (uiTotal > 16383)
			{
				for (int j = 0;j < 256;++j)
				{
					m_ac2[j] >>= 1;
					if (m_ac2[j] == 0 && m_ac[j] != 0)
						m_ac2[j] = 1;
				}
				break;
			}
		}
		if (uiTotal > 16383)
			continue;
		break;
	}
}

// Build the scaled range values
__inline unsigned int CArithmeticEncoder::RangeCounts()
{
	unsigned int uiScale = 0;
	for (int i = 0;i < 256;++i)
	{
		m_ar[i] = uiScale;
		uiScale += m_ac2[i];
	}
	m_ar[256] = uiScale;

	// Return the scale
	return uiScale;
}

// Build the map that maps the range values back to a symbol for fast decoding
__inline void CArithmeticEncoder::BuildMap()
{
	for (unsigned int ui = 0;ui < 256;++ui)
		if (m_ac2[ui])
			for (unsigned short int uiRange = m_ar[ui];uiRange < m_ar[ui + 1];++uiRange)
				m_aMap[uiRange] = ui;
}

// Output a bit
__inline void CArithmeticEncoder::OutputBit(unsigned short int usiBit,unsigned char & ucOutSymbol,unsigned char & ucBitMask,unsigned long & ulDestCount,char * pBuffer)
{
	// Output the most significant bit
	if (usiBit)
		ucOutSymbol |= ucBitMask;
	ucBitMask >>= 1;

	// Test for output
	if (ucBitMask == 0)
	{
		// Output encoded byte
		pBuffer[ulDestCount++] = ucOutSymbol;
		ucOutSymbol = 0;
		ucBitMask = BITMASK;
	}
}

// Output the underflow bits
__inline void CArithmeticEncoder::OutputUnderflowBits(unsigned short int usiBit,unsigned long & ulUnderflowBits,unsigned char & ucOutSymbol,unsigned char & ucBitMask,unsigned long & ulDestCount,char * pBuffer)
{
	// Account for all underflow bits
	while (ulUnderflowBits)
	{
		// Output the most significant bit
		OutputBit(usiBit,ucOutSymbol,ucBitMask,ulDestCount,pBuffer);

		// Decrement the count
		ulUnderflowBits--;
	}
}

// Flush the bitmask, simulating shifting in bits
__inline void CArithmeticEncoder::FlushBitMask(unsigned char & ucBitMask,unsigned char & ucOutSymbol,unsigned long & ulDestCount,char * pBuffer)
{
	while (ucBitMask)
	{
		ucOutSymbol |= ucBitMask;
		ucBitMask >>= 1;
	}

	// Output remaining encoded byte
	pBuffer[ulDestCount++] = ucOutSymbol;
	ucOutSymbol = 0;
	ucBitMask = BITMASK;
}

// Implementation of Encode
bool CArithmeticEncoder::EncodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount)
{
	// Initialize the modeling data arrays
	InitModel();

	// Input buffer
	unsigned char * pBuffer = (unsigned char *)pBufferIn;

	// The symbol and count
	unsigned char ucSymbol = 0;
	unsigned int uinSymbol = 0;

	// Counts and scaled counts
	unsigned long ulByte;
	for (ulByte = 0;ulByte < ulnSrcCount;++ulByte)
		uinSymbol += (m_ac[pBuffer[ulByte]]++ ? 0 : 1);
	if (!uinSymbol)
		return false;

	// Scale the counts
	ScaleCounts();

	// Get the count ranges
	unsigned int uiScale = RangeCounts();

	// Allocate the maximum output buffer (Total bytes encoded + Total symbol count + Symbols and counts + Max Output data)
	unsigned long ulnBuffer = 4 + 1 + 1280 + ulnSrcCount;

	// Test for growing the buffer
	if (ulnBuffer > m_ulnLastBuffer)
	{
		if (pBufferOut)
			delete [] pBufferOut;
		pBufferOut = new char[ulnBuffer];
		m_ulnLastBuffer = ulnBuffer;
	}

	// Starting buffer position is at the total symbol count
	unsigned long ulDestCount = 4;

	// Write the count of symbols modeling data
	pBufferOut[ulDestCount++] = (unsigned char)(uinSymbol - 1);

	// Write the symbol counts modeling data
	ucSymbol = 0; 
	do
	{
		if (m_ac[ucSymbol])
		{
			pBufferOut[ulDestCount++] = ucSymbol;
			pBufferOut[ulDestCount++] = (unsigned char)((m_ac[ucSymbol] & 0xFF000000) >> 24);
			pBufferOut[ulDestCount++] = (unsigned char)((m_ac[ucSymbol] & 0x00FF0000) >> 16);
			pBufferOut[ulDestCount++] = (unsigned char)((m_ac[ucSymbol] & 0x0000FF00) >> 8);
			pBufferOut[ulDestCount++] = (unsigned char)(m_ac[ucSymbol] & 0x000000FF);
		}
	} while (++ucSymbol);

	// Encode the data
	unsigned short int usiLow = 0,usiHigh = 0xFFFF;
	unsigned int uiRange = 0;
	unsigned long ulUnderflowBits = 0;

	// Output tracking
	unsigned char ucOutSymbol = 0;
	unsigned char ucBitMask = BITMASK;

	// Output bit
	unsigned short int usiBit = 0;

	// Main loop
	for (ulByte = 0;ulByte < ulnSrcCount;++ulByte)
	{
		// Get the symbol
		ucSymbol = pBuffer[ulByte];

		// Calculate the range, high value, and low value
		uiRange = (unsigned int)(usiHigh - usiLow + 1);
		usiHigh = usiLow + (unsigned short int)((uiRange * m_ar[ucSymbol + 1]) / uiScale - 1);
		usiLow = usiLow + (unsigned short int)((uiRange * m_ar[ucSymbol]) / uiScale);

		// Build onto the output
		for (;;)
		{
			// Check for output
			usiBit = usiHigh & MSB;
			if (usiBit == (usiLow & MSB))
			{
				// Output the most significant bit
				OutputBit(usiBit,ucOutSymbol,ucBitMask,ulDestCount,pBufferOut);

				// Output previous underflow bits first
				if (ulUnderflowBits > 0)
				{
					// Get the underflow bit
					usiBit = ~usiHigh & MSB;

					// Output the underflow bits
					OutputUnderflowBits(usiBit,ulUnderflowBits,ucOutSymbol,ucBitMask,ulDestCount,pBufferOut);
				}
			}
			else if ((usiLow & NSB) && !(usiHigh & NSB))
			{
				// Underflow prevention
				ulUnderflowBits++;
				usiHigh |= NSB;
				usiLow &= USB;
			}
			else
				break;

			// Shift the inputs
			usiHigh = usiHigh << 1;
			usiHigh |= 1;
			usiLow = usiLow << 1;
		}

		// Update the symbol count
		if (!(--m_ac[ucSymbol]))
		{
			// Scale the counts for the removed symbol
			ScaleCounts();

			// Get the range for the counts
			uiScale = RangeCounts();
		}
	}

	// Flush the encoder of the 2 low MSB's and any remaing underflow
	usiBit = usiLow & NSB;
	OutputBit(usiBit,ucOutSymbol,ucBitMask,ulDestCount,pBufferOut);
	usiBit = ~usiLow & NSB;
	OutputBit(usiBit,ucOutSymbol,ucBitMask,ulDestCount,pBufferOut);

	// Output the remaining underflow bits
	if (ulUnderflowBits > 0)
		OutputUnderflowBits(usiBit,ulUnderflowBits,ucOutSymbol,ucBitMask,ulDestCount,pBufferOut);

	// Flush out the bitmask
	if (ucBitMask)
		FlushBitMask(ucBitMask,ucOutSymbol,ulDestCount,pBufferOut);

	// Output the total bytes encoded
	pBufferOut[0] = (unsigned char)((ulnSrcCount & 0xFF000000) >> 24);
	pBufferOut[1] = (unsigned char)((ulnSrcCount & 0x00FF0000) >> 16);
	pBufferOut[2] = (unsigned char)((ulnSrcCount & 0x0000FF00) >> 8);
	pBufferOut[3] = (unsigned char)(ulnSrcCount & 0x000000FF);

	// Set the ouput size
	ulnDestCount = ulDestCount;

	return true;
}

// Implementation of Encode
bool CArithmeticEncoder::EncodeBuffer()
{
	return EncodeBuffer(m_pBufferIn,m_ulnSrcCount,m_pBufferOut,m_ulnDestCount);
}

// Implementation of decode
bool CArithmeticEncoder::DecodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char ** ppBufferOut,unsigned long * pulnDestCount,BOOL bAlloc)
{
	// Initialize the modeling data arrays
	InitModel();

	// Input buffer
	unsigned char * pBuffer = (unsigned char *)pBufferIn;
	unsigned long ulSrcCount = 0;

	// Read the total bytes encoded
	unsigned char uc4 = pBuffer[ulSrcCount++];
	unsigned char uc3 = pBuffer[ulSrcCount++];
	unsigned char uc2 = pBuffer[ulSrcCount++];
	unsigned char uc1 = pBuffer[ulSrcCount++];

	// Calculate the total bytes
	unsigned long ulnBuffer = (uc4 << 24) + (uc3 << 16) + (uc2 << 8) + uc1;

	// Test for growing the buffer
	if (ulnBuffer > m_ulnLastBuffer)
	{
		if (bAlloc)
		{
			if (*ppBufferOut)
				delete [] *ppBufferOut;
			*ppBufferOut = new char[ulnBuffer];
		}
		m_ulnLastBuffer = ulnBuffer;
	}

	unsigned long ulTotal = ulnBuffer;
	unsigned char * pBufferOut = (unsigned char *)*ppBufferOut;
	*pulnDestCount = ulTotal;

	// The symbol
	unsigned char ucSymbol = 0;

	// Read the count of modeling data
	unsigned int uinSymbol = pBuffer[ulSrcCount++] + 1;

	// Read the modeling data
	for (unsigned int uin = 0;uin < uinSymbol;++uin)
	{
		ucSymbol = pBuffer[ulSrcCount++];
		uc4 = pBuffer[ulSrcCount++];
		uc3 = pBuffer[ulSrcCount++];
		uc2 = pBuffer[ulSrcCount++];
		uc1 = pBuffer[ulSrcCount++];
		unsigned long ulSymbolTotal = (uc4 << 24) + (uc3 << 16) + (uc2 << 8) + uc1;
		m_ac[ucSymbol] = ulSymbolTotal;
	}

	// Scale the counts
	ScaleCounts();

	// Get the range of counts
	unsigned int uiScale = RangeCounts();

	// Build the decode map of range to symbol for fast lookup
	BuildMap();

	// Initialize the code variables
	unsigned short int usiCode = 0;
	unsigned char ucCode = 0;

	// Read the first code word
	ucCode = pBuffer[ulSrcCount++];
	usiCode |= ucCode;
	usiCode <<= 8;
	ucCode = pBuffer[ulSrcCount++];
	usiCode |= ucCode;

	// Initialize the count of decoded characters and code
	unsigned long ulDestCount = 0;

	// Initialize the range
	unsigned short int usiLow = 0,usiHigh = 0xFFFF;
	unsigned int uiRange = 0;
	unsigned long ulUnderflowBits = 0;
	unsigned short int usiCount = 0;

	// The bit mask tracks the remaining bits in the input code buffer
	unsigned char ucBitMask = 0;

	// The main decoding loop
	do
	{
		// Get the range and count for the current arithmetic code
		uiRange = (unsigned int)(usiHigh - usiLow + 1);
		usiCount = (unsigned short int)((((usiCode - usiLow) + 1) * uiScale - 1) / uiRange);

		// Look up the symbol in the map
		ucSymbol = m_aMap[usiCount];

		// Output the symbol
		pBufferOut[ulDestCount++] = ucSymbol;
		if (ulDestCount >= ulTotal)
			break;

		// Calculate the high and low value of the symbol
		usiHigh = usiLow + (unsigned short int)((uiRange * m_ar[ucSymbol + 1]) / uiScale - 1);
		usiLow = usiLow + (unsigned short int)((uiRange * m_ar[ucSymbol]) / uiScale);

		// Remove the symbol from the stream
		for (;;)
		{
			if ((usiHigh & MSB) == (usiLow & MSB))
			{
				// Fall through to shifting
			}
			else if ((usiLow & NSB) == NSB && (usiHigh & NSB) == 0)
			{
				// Account for underflow
				usiCode ^= NSB;
				usiHigh |= NSB;
				usiLow &= USB;
			}
			else
				break;

			// Shift out a bit from the low and high values
			usiHigh <<= 1;
			usiHigh |= 1;
			usiLow <<= 1;

			// Shift out a bit from the code
			usiCode <<= 1;

			// Test for a needing an input symbol
			if (ucBitMask)
			{
shift:

				// Test for shifting in bits
				if (ucCode & ucBitMask)
					usiCode |= 1;
				ucBitMask >>= 1;
			}
			else
			{
				// Load up a new code
				if (ulSrcCount < ulnSrcCount)
				{
					ucCode = pBuffer[ulSrcCount++];
					ucBitMask = BITMASK;
					goto shift;
				}
				else if (ulDestCount < ulTotal)
				{
					// Helper with the last remaining symbols
					ucBitMask = BITMASK;
					goto shift;
				}
			}
		}

		// Update the symbol count
		if (!(--m_ac[ucSymbol]))
		{
			// Scale the counts for the removed symbol
			ScaleCounts();

			// Get the range of counts
			uiScale = RangeCounts();

			// Build the lookup map
			BuildMap();
		}
	} while (ulDestCount < ulTotal);

	return true;
}

// Implementation of decode
bool CArithmeticEncoder::DecodeBuffer()
{
	return DecodeBuffer(m_pBufferIn,m_ulnSrcCount,&m_pBufferOut,&m_ulnDestCount);
}

CZLib::CZLib() : 
	m_pBufferIn(NULL), m_ulnSrcCount(0), m_pBufferOut(NULL), m_ulnDestCount(0), m_ulnLastBuffer(0)
{
}

CZLib::CZLib(char * pBufferIn,unsigned long ulnSrcCount) :
	m_pBufferIn(pBufferIn), m_ulnSrcCount(ulnSrcCount), m_pBufferOut(NULL), m_ulnDestCount(0), m_ulnLastBuffer(0)
{
}

int CZLib::EncodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount)
{
	ulnDestCount = (uLong)((double)ulnSrcCount * 1.1) + 12;
	pBufferOut = new char[ulnDestCount];
	return ZLIBCompress((Byte *)pBufferOut,ulnDestCount,(Byte *)pBufferIn,ulnSrcCount);
}

int CZLib::DecodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount,BOOL bAlloc)
{
	if (bAlloc)
		pBufferOut = new char[ulnDestCount];
	return ZLIBUncompress((Byte *)pBufferOut,ulnDestCount,(Byte *)pBufferIn,ulnSrcCount);
}

// Set the input buffer, clear the output
void CZLib::SetBuffer(char * pBufferIn,unsigned long ulnSrcCount,BOOL bEncode)
{
	// Set the buffer
	m_pBufferIn = pBufferIn;

	// Set the input bytes
	m_ulnSrcCount = ulnSrcCount;

	// Size the output buffer
	unsigned long ulnBuffer = 0;
	if (bEncode)
		ulnBuffer = (unsigned long)((double)ulnSrcCount * 1.1) + 12;
	else
		ulnBuffer = ulnSrcCount;
	
	// Test for growing the buffer
	if (ulnBuffer > m_ulnLastBuffer)
	{
		if (m_pBufferOut)
			delete [] m_pBufferOut;
		m_pBufferOut = new char[ulnBuffer];
		m_ulnLastBuffer = ulnBuffer;
	}

	// Set the output bytes
	m_ulnDestCount = ulnBuffer;
}

// Get the output buffer
void CZLib::GetBuffer(char * & pBufferOut,unsigned long & ulnDestCount)
{
	// Set the buffer
	pBufferOut = m_pBufferOut;
	ulnDestCount = m_ulnDestCount;
}

CZLib::~CZLib()
{
	// Cleanup the output
	if (m_pBufferOut)
		delete [] m_pBufferOut;
}

int CZLib::EncodeBuffer()
{
	return ZLIBCompress((Byte *)m_pBufferOut,m_ulnDestCount,(Byte *)m_pBufferIn,m_ulnSrcCount);
}

int CZLib::DecodeBuffer()
{
	return ZLIBUncompress((Byte *)m_pBufferOut,m_ulnDestCount,(Byte *)m_pBufferIn,m_ulnSrcCount);
}

int CZLib::ZLIBCompress(Byte * Dest, uLong & DestLen, Byte * Src, uLong SrcLen)
{
	int ZLIBErr = compress2(Dest, &DestLen, Src, SrcLen, Z_BEST_COMPRESSION);
	return ZLIBErr;
}

int CZLib::ZLIBUncompress(Byte * Dest, uLong & DestLen, Byte * Src, uLong SrcLen)
{
	int ZLIBErr = uncompress(Dest, &DestLen, Src, SrcLen);
	return ZLIBErr;
}

IMPLEMENT_DYNCREATE(CMultiThreadedCompression, CWinThread)

CMultiThreadedCompression::CMultiThreadedCompression(HANDLE * phHandle,CArithmeticEncoder * pAC,CZLib * pZLib) : m_phHandle(phHandle), m_pAC(pAC), m_pZLib(pZLib)
{
	// Create the signaling event
	HANDLE & hHandle = *m_phHandle;
	hHandle = CreateEvent(NULL,FALSE,FALSE,NULL);

	// Create the thread
	CreateThread();
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);
}

CMultiThreadedCompression::~CMultiThreadedCompression()
{
	// Close the signaling event
	HANDLE & hHandle = *m_phHandle;
	CloseHandle(hHandle);
}

BOOL CMultiThreadedCompression::InitInstance()
{
	return TRUE;
}

int CMultiThreadedCompression::ExitInstance()
{
	return CWinThread::ExitInstance();
}

// These functions are called by PostThreadMessage
BEGIN_MESSAGE_MAP(CMultiThreadedCompression, CWinThread)
	ON_THREAD_MESSAGE(WM_PROCESSBUFFER,&CMultiThreadedCompression::OnProcessBuffer)
	ON_THREAD_MESSAGE(WM_ENDTHREAD,&CMultiThreadedCompression::OnEndThread)
END_MESSAGE_MAP()

void CMultiThreadedCompression::OnProcessBuffer(WPARAM wParam,LPARAM lParam)
{
	// Get the type of compressor
	BOOL bAC = (BOOL)lParam;

	// Do the work
	if (wParam)
	{
		if (bAC)
			m_pAC->EncodeBuffer();
		else
			m_pZLib->EncodeBuffer();
	}
	else
	{
		if (bAC)
			m_pAC->DecodeBuffer();
		else
			m_pZLib->DecodeBuffer();
	}

	// Signal completion
	HANDLE & hHandle = *m_phHandle;
	SetEvent(hHandle);
}

void CMultiThreadedCompression::OnEndThread(WPARAM wParam,LPARAM lParam)
{
	// Delete the arithmetic encoder object
	if (m_pAC)
		delete m_pAC;

	// Delete the zlib object
	if (m_pZLib)
		delete m_pZLib;

	// End the thread
	PostQuitMessage(0);
}

// Drive the multithreaded arithmetic encoder
CDriveMultiThreadedCompression::CDriveMultiThreadedCompression(int nTotalThreads) : m_dwLastBytes(0)
{
	// Set the total number of threads to do work
	m_nTotalThreads = nTotalThreads;

	// Construct the multithreaded driver array for the arithmetic encoder
	m_ppMultiThreadedCompression = new CMultiThreadedCompression * [m_nTotalThreads];

	// Construct the arithmetic encoders
	for (int iHandle = 0;iHandle < m_nTotalThreads;++iHandle)
	{
		// Construct the arithmetic encoder
		CArithmeticEncoder * pAC = new CArithmeticEncoder;
		CZLib * pZLib = new CZLib;

		// Construct the driver
		CMultiThreadedCompression * & pMultiThreadedArithmeticEncoder = m_ppMultiThreadedCompression[iHandle];
		pMultiThreadedArithmeticEncoder = new CMultiThreadedCompression(&m_arrhAC[iHandle],pAC,pZLib);
	}
}

// Clean up the multithreaded arithmetic encoder
CDriveMultiThreadedCompression::~CDriveMultiThreadedCompression()
{
	// Clean up the threads of work
	for (int iHandle = 0;iHandle < m_nTotalThreads;iHandle++)
	{
		// End the thread
		CMultiThreadedCompression * pMultiThreadedArithmeticEncoder = m_ppMultiThreadedCompression[iHandle];
		pMultiThreadedArithmeticEncoder->PostThreadMessage(WM_ENDTHREAD,0,0);

		// Wait for the thread to exit before cleaning it up
		if (WaitForSingleObject(pMultiThreadedArithmeticEncoder->m_hThread,5000) != WAIT_OBJECT_0)
			TerminateThread(pMultiThreadedArithmeticEncoder->m_hThread,0);
	}

	// Delete the driver array
	delete [] m_ppMultiThreadedCompression;
}

// Set the encoder
void CDriveMultiThreadedCompression::SetEncoder(BOOL bAC)
{
	m_bAC = bAC;
}

// Set the encode buffer
void CDriveMultiThreadedCompression::SetBuffer(char * pBuffer,DWORD dwTotalBytes,BOOL bEncode)
{
	// Get the amount of bytes to process per thread
	if (bEncode)
		m_dwBytesPerThread = dwTotalBytes / m_nTotalThreads;
	else
	{
		// Get the number of threads used to create this buffer
		memcpy(&m_nTotalThreads,pBuffer,sizeof(m_nTotalThreads));
		pBuffer += sizeof(m_nTotalThreads);
	}

	// Construct the arithmetic encoders
	for (int iHandle = 0;iHandle < m_nTotalThreads;++iHandle)
	{
		// Set the bytes to process
		DWORD dwBytes = 0;
		char * pData = NULL;

		// Test for encoding or decoding
		if (bEncode)
		{
			DWORD dwBegBytes = m_dwBytesPerThread * iHandle;
			DWORD dwEndBytes = dwBegBytes + m_dwBytesPerThread - 1;
			if ((iHandle + 1) == m_nTotalThreads)
				dwEndBytes = dwTotalBytes - 1;
			pData = pBuffer + dwBegBytes;
			dwBytes = dwEndBytes - dwBegBytes + 1;
		}
		else
		{
			// Copy the decode bytes
			memcpy(&dwBytes,pBuffer,sizeof(dwBytes));
			pBuffer += sizeof(dwBytes);

			// Get the buffer
			pData = pBuffer;
			if ((iHandle + 1) != m_nTotalThreads)
				pBuffer += dwBytes;
		}

		// Get the driver
		CMultiThreadedCompression * pMultiThreadedCompression = m_ppMultiThreadedCompression[iHandle];

		// Test the type of compression
		if (m_bAC)
		{
			// Get the arithmetic encoder
			CArithmeticEncoder * pAC = pMultiThreadedCompression->GetAC();

			// Set the buffer
			pAC->SetBuffer(pData,dwBytes);
		}
		else
		{
			// Get the arithmetic encoder
			CZLib * pZLib = pMultiThreadedCompression->GetZLib();

			// Set the buffer
			pZLib->SetBuffer(pData,bEncode ? dwBytes : dwTotalBytes,bEncode);
		}
	}
}

// Get the size of the output buffer
void CDriveMultiThreadedCompression::GetBufferSize(BOOL bEncode,DWORD & dwBytes)
{
	// Track the total output size, encoding adds in the byte counts, decoding removes them
	dwBytes = bEncode ? sizeof(m_nTotalThreads) : 0;

	// Calculate the output storage
	for (int iHandle = 0;iHandle < m_nTotalThreads;++iHandle)
	{
		// Get the driver
		CMultiThreadedCompression * pMultiThreadedCompression = m_ppMultiThreadedCompression[iHandle];

		// Get the compression object
		CArithmeticEncoder * pAC = NULL;
		CZLib * pZLib = NULL;
		if (m_bAC)
			pAC = pMultiThreadedCompression->GetAC();
		else
			pZLib = pMultiThreadedCompression->GetZLib();

		// Temporary buffer pointers
		char * pBufferOut = NULL;
		DWORD dwBytesOut = 0;
		
		// Get the buffer size and data pointer
		if (m_bAC)
			pAC->GetBuffer(pBufferOut,dwBytesOut);
		else
			pZLib->GetBuffer(pBufferOut,dwBytesOut);

		// Increment the output storage requirement
		if (bEncode)
			dwBytes += sizeof(dwBytesOut);
		dwBytes += dwBytesOut;
	}
}

// Get the buffer
void CDriveMultiThreadedCompression::GetBuffer(char ** ppBuffer,DWORD * pdwBytes,BOOL bEncode,BOOL bAlloc)
{
	// Get the total output size
	DWORD dwBytes = 0;
	GetBufferSize(bEncode,dwBytes);
	*pdwBytes = dwBytes;

	// Allocate the output buffer
	if (bAlloc)
	{
		if (dwBytes > m_dwLastBytes)
		{
			// Allocate a new buffer, release the old
			m_Buffer = std::auto_ptr<char>(new char[dwBytes]);
			m_pBuffer = m_Buffer.get();

			// Set the new buffer size
			m_dwLastBytes = dwBytes;
		}
	}

	// Allocate the output storage
	char * pBuffer = bAlloc ? m_pBuffer : *ppBuffer;
	DWORD dwPos = 0;

	if (bEncode)
	{
		// Copy the number of threads that created this storage
		memcpy(pBuffer + dwPos,&m_nTotalThreads,sizeof(m_nTotalThreads));
		dwPos += sizeof(m_nTotalThreads);
	}

	// Copy to the output storage
	for (int iHandle = 0;iHandle < m_nTotalThreads;++iHandle)
	{
		// Get the driver
		CMultiThreadedCompression * pMultiThreadedCompression = m_ppMultiThreadedCompression[iHandle];

		// Get the compression object
		CArithmeticEncoder * pAC = NULL;
		CZLib * pZLib = NULL;
		if (m_bAC)
			pAC = pMultiThreadedCompression->GetAC();
		else
			pZLib = pMultiThreadedCompression->GetZLib();

		// Get the buffer size and data pointer
		char * pBufferOut = NULL;
		DWORD dwBytesOut = 0;

		if (m_bAC)
			pAC->GetBuffer(pBufferOut,dwBytesOut);
		else
			pZLib->GetBuffer(pBufferOut,dwBytesOut);

		if (bEncode)
		{
			// For compression, copy the buffer size to the output storage
			memcpy(pBuffer + dwPos,&dwBytesOut,sizeof(dwBytesOut));
			dwPos += sizeof(dwBytesOut);
		}

		// Copy the buffer to the output storage
		memcpy(pBuffer + dwPos,pBufferOut,dwBytesOut);
		if ((iHandle + 1) < m_nTotalThreads)
			dwPos += dwBytesOut;
	}

	// Set the return
	if (bAlloc)
		*ppBuffer = m_pBuffer;
}

// Encode a buffer
bool CDriveMultiThreadedCompression::Encode()
{
	// Encode
	return ProcessBuffer(TRUE);
}

// Decode a buffer
bool CDriveMultiThreadedCompression::Decode()
{
	// Decode
	return ProcessBuffer(FALSE);
}

// Do the buffer encoding or decoding
bool CDriveMultiThreadedCompression::ProcessBuffer(BOOL bEncode)
{
	// Process the number of threads of work
	for (int iHandle = 0;iHandle < m_nTotalThreads;iHandle++)
	{
		// Perform the multithreaded arithmetic encoding
		CMultiThreadedCompression * pMultiThreadedArithmeticEncoder = m_ppMultiThreadedCompression[iHandle];
		pMultiThreadedArithmeticEncoder->PostThreadMessage(WM_PROCESSBUFFER,bEncode,(LPARAM)m_bAC);
	}

	// Wait for all the work to complete for this set of ranges
	WaitForMultipleObjects(m_nTotalThreads,&m_arrhAC[0],TRUE,INFINITE);
	return true;
}