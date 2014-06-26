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

#include "Common.h"

// CArithmeticEncoding (high level implementation)
#define BITMASK 0x80
#define MSB 0x8000
#define NSB 0x4000
#define USB 0x3FFF

class CArithmeticEncoder
{
public:
	CArithmeticEncoder();
	bool EncodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount);
	bool DecodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char ** ppBufferOut,unsigned long * pulnDestCount,BOOL bAlloc = TRUE);

	CArithmeticEncoder(char * pBufferIn,unsigned long ulnSrcCount);
	void SetBuffer(char * pBufferIn,unsigned long ulnSrcCount);
	void GetBuffer(char * & pBufferOut,unsigned long & ulnDestCount);
	bool EncodeBuffer();
	bool DecodeBuffer();

	~CArithmeticEncoder();

protected:
	void InitModel();
	void ScaleCounts();
	unsigned int RangeCounts();
	void BuildMap();
	void OutputBit(unsigned short int usiBit,unsigned char & ucOutSymbol,unsigned char & ucBitMask,unsigned long & ulDestCount,char * pBuffer);
	void OutputUnderflowBits(unsigned short int usiBit,unsigned long & ulUnderflowBits,unsigned char & ucOutSymbol,unsigned char & ucBitMask,unsigned long & ulDestCount,char * pBuffer);
	void FlushBitMask(unsigned char & ucBitMask,unsigned char & ucOutSymbol,unsigned long & ulDestCount,char * pBuffer);

protected:
	char * m_pBufferIn;
	unsigned long m_ulnSrcCount;
	char * m_pBufferOut;
	unsigned long m_ulnDestCount;
	unsigned long m_ulnLastBuffer;

private:
	unsigned long m_ac[256];
	unsigned long m_ac2[256];
	unsigned short int m_ar[257];
	unsigned int m_aMap[16384];
};

class CZLib
{
public:
	CZLib();
	int EncodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount);
	int DecodeBuffer(char * pBufferIn,unsigned long ulnSrcCount,char * & pBufferOut,unsigned long & ulnDestCount,BOOL bAlloc = TRUE);

	CZLib(char * pBufferIn,unsigned long ulnSrcCount);
	void SetBuffer(char * pBufferIn,unsigned long ulnSrcCount,BOOL bEncode);
	void GetBuffer(char * & pBufferOut,unsigned long & ulnDestCount);
	int EncodeBuffer();
	int DecodeBuffer();

	~CZLib();

protected:
	char * m_pBufferIn;
	unsigned long m_ulnSrcCount;
	char * m_pBufferOut;
	unsigned long m_ulnDestCount;
	unsigned long m_ulnLastBuffer;

protected:
	int ZLIBCompress(Byte * Dest, uLong & DestLen, Byte * Src, uLong SrcLen);
	int ZLIBUncompress(Byte * Dest, uLong & DestLen, Byte * Src, uLong SrcLen);
};

// Worker thread that carries out the multi-threaded arithmetic encoder
class CMultiThreadedCompression : public CWinThread
{
	DECLARE_DYNCREATE(CMultiThreadedCompression)

private:
	CMultiThreadedCompression() : m_phHandle(NULL), m_pAC(NULL) {};

public:
	CMultiThreadedCompression(HANDLE * phHandle,CArithmeticEncoder * pAC,CZLib * pZLib);
	virtual ~CMultiThreadedCompression();
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	CArithmeticEncoder * GetAC() {return m_pAC;}
	CZLib * GetZLib() {return m_pZLib;}

protected:
	HANDLE * m_phHandle;
	CArithmeticEncoder * m_pAC;
	CZLib * m_pZLib;

protected:
	DECLARE_MESSAGE_MAP()

protected:
	afx_msg void OnProcessBuffer(WPARAM wParam,LPARAM lParam);
	afx_msg void OnEndThread(WPARAM wParam,LPARAM lParam);
};

// Driver that carries out the work of arithmetic encoding
class CDriveMultiThreadedCompression
{
	CDriveMultiThreadedCompression() throw(...) {throw;}
public:
	CDriveMultiThreadedCompression(int nTotalThreads);
	~CDriveMultiThreadedCompression();
	void SetEncoder(BOOL bAC);
	void SetBuffer(char * pBuffer,DWORD dwTotalBytes,BOOL bEncode);
	void GetBuffer(char ** ppBuffer,DWORD * pdwBytes,BOOL bEncode,BOOL bAlloc = TRUE);
	void GetBufferSize(BOOL bEncode,DWORD & dwBytes);
	bool Encode();
	bool Decode();

protected:
	bool ProcessBuffer(BOOL bEncode);

protected:
	HANDLE m_arrhAC[MAXTHREADS];
	int m_nTotalThreads;
	DWORD m_dwBytesPerThread;
	CMultiThreadedCompression ** m_ppMultiThreadedCompression;
	BOOL m_bAC;
	DWORD m_dwLastBytes;
	std::auto_ptr<char> m_Buffer;
	char * m_pBuffer;
};