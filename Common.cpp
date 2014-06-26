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
#include "Common.h"
#include <vector>

// Debugging
void DebugMsg(const TCHAR * pwszFormat,...)
{
	TCHAR buf[1024] = {'\0'};
	va_list arglist;
	va_start(arglist, pwszFormat);
	int nBufSize = _vsctprintf(pwszFormat,arglist) + 1;
	if (nBufSize)
	{
		std::auto_ptr<TCHAR> Buffer(new TCHAR[nBufSize]);
		TCHAR * pBuffer = Buffer.get();
		_vstprintf_s(pBuffer,nBufSize,pwszFormat,arglist);
		va_end(arglist);
		OutputDebugString(pBuffer);
	}
}

// Breaks down "GetLastError"
void DebugLastError()
{
	LPVOID lpMsgBuf;
	DWORD dwLastError = GetLastError(); 
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,dwLastError,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR)&lpMsgBuf,0,NULL);
	DebugMsg("%s\n",lpMsgBuf);
	LocalFree(lpMsgBuf);
}

static OSVERSIONINFO g_osversioninfo;
static DWORD g_dwPlatformId;
static DWORD g_dwMajorVersion;
static DWORD g_dwMinorVersion;

void InitVersion()
{
    // Get the current OS version
    g_osversioninfo.dwOSVersionInfoSize = sizeof(g_osversioninfo);
    if (!GetVersionEx(&g_osversioninfo))
	    g_dwPlatformId = 0;
    g_dwPlatformId = g_osversioninfo.dwPlatformId;
	g_dwMajorVersion = g_osversioninfo.dwMajorVersion;
	g_dwMinorVersion = g_osversioninfo.dwMinorVersion;
}

BOOL IsNtVer(ULONG mj,ULONG mn)
{
	if (!IsWinNT())	
		return FALSE;
	return (VersionMajor() == mj && VersionMinor() == mn);
}

BOOL IsWinNT()
{
	return (g_dwPlatformId == VER_PLATFORM_WIN32_NT);
}

BOOL IsWinVerOrHigher(ULONG mj,ULONG mn)
{
	return (VersionMajor() > mj || (VersionMajor() == mj && VersionMinor() >= mn));
}

DWORD VersionMajor()
{
	return g_dwMajorVersion;
}

DWORD VersionMinor()
{
	return g_dwMinorVersion;
}

