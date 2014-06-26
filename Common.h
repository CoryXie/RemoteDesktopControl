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

#include <intrin.h>
#include <memory>
#include "zlib.h"

#if defined(_DEBUG)
#if !defined(X64)
#pragma comment(lib,"..\\Zlib32d.lib")
#else
#pragma comment(lib,"..\\Zlib64d.lib")
#endif
#else
#if !defined(X64)
#pragma comment(lib,"..\\Zlib32.lib")
#else
#pragma comment(lib,"..\\Zlib64.lib")
#endif
#endif

#define MAXTHREADS 64
#define WM_ENDTHREAD (WM_APP + 100)
#define WM_PROCESSBUFFER (WM_APP + 101)
#define WM_IMAGEREFRESH (WM_APP + 102)
#define WM_IMAGECOMPRESS (WM_APP + 103)
#define WM_IMAGESEND (WM_APP + 104)
#define WM_IMAGERENEW (WM_APP + 105)
#define WM_CONNECTSERVER (WM_APP + 106)

#define SERVICE_CONTROL_USER 128

void DebugMsg(const TCHAR * pwszFormat,...);
void DebugLastError();

// Debugging timer used for detecting the duration of an operation
class CDuration
{
   const TCHAR * p;
   clock_t start;
   clock_t finish;
public:
   CDuration(const TCHAR * sz) : p(sz),start(clock()),finish(start) {};
   ~CDuration()
   {
      finish = clock();
      double duration = (double)(finish - start) / CLOCKS_PER_SEC;
      DebugMsg("%s Duration = %f seconds\n",p,duration);
   }
};

// CPU id helper structure
struct CPU
{
	CPU() : m_r1(0), m_r2(0), m_nProc(0), m_r3(0), m_r4(0), m_r5(0) {Init();}
	CPU(int nProc) : m_r1(0), m_r2(0), m_nProc(nProc), m_r3(0), m_r4(0), m_r5(0) {Init();}
	~CPU() {};
	CPU(const CPU & rhs) {*this = rhs;}
	CPU & operator = (const CPU & rhs)
	{
		if (this != &rhs)
		{
			m_r1 = rhs.m_r1;
			m_r2 = rhs.m_r2;
			m_nProc = rhs.m_nProc;
			m_r3 = rhs.m_r3;
			m_r4 = rhs.m_r4;
			m_r5 = rhs.m_r5;
		}
		return *this;
	}
	void Init() {__cpuid((int*)this,1);}
	int GetNbProcs() {return m_nProc;}
	int GetNbThreads() {return min(GetNbProcs() * 4,MAXTHREADS);}

	unsigned m_r1 : 32;
	unsigned m_r2 : 16;
	unsigned m_nProc : 8; // Number of logical processors
	unsigned m_r3 : 32;
	unsigned m_r4 : 32;
	unsigned m_r5 : 4;
	unsigned m_HT : 1; // multi-threading / hyperthreading bit
	unsigned m_r6 : 3;
};

// Compression helper structure
struct CompressionInfo
{
	char * m_pInBuffer;
	DWORD m_dwSrcBytes;
	char ** m_ppOutBuffer;
	DWORD * m_pdwOutBytes;
};

void InitVersion();
BOOL IsNtVer(ULONG mj,ULONG mn);
BOOL IsWinNT();
BOOL IsWinVerOrHigher(ULONG mj, ULONG mn);
DWORD VersionMajor();
DWORD VersionMinor();