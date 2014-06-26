#pragma once
/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998 by Shane Martin
// All rights reserved
//
// Distribute freely, except: don't remove my name from the source or
// documentation (don't take credit for my work), mark your changes (don't
// get me blamed for your possible bugs), don't alter or remove this
// notice.
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc., and
// I'll try to keep a version up to date.  I can be reached as follows:
//    shane.kim@kaiserslautern.netsurf.de
/////////////////////////////////////////////////////////////////////////////
#include <winreg.h>

#define REG_RECT	0x0001
#define REG_POINT	0x0002

class CRegistry : public CObject
{
	// Construction
public:
	CRegistry (BOOL bAdmin, BOOL bReadOnly);
	virtual ~CRegistry();

	struct REGINFO
	{
		LONG lMessage;
		DWORD dwType;
		DWORD dwSize;
	}
	m_Info;

	// Operations
public:
	void Attach(HKEY hKey);
	HKEY Detach();
	BOOL EnumKey(int index, CString& keyName);
	BOOL RecursiveDeleteKey(LPCTSTR pszPath, BOOL bAdmin = FALSE);

	BOOL VerifyKey (LPCTSTR pszPath);
	BOOL VerifyValue (LPCTSTR pszValue);
	BOOL CreateKey (LPCTSTR pszPath);
	BOOL Open (LPCTSTR pszPath);
	void Close();

	BOOL DeleteValue (LPCTSTR pszValue);
	BOOL DeleteKey (LPCTSTR pszPath, BOOL bAdmin = FALSE);

	BOOL Write (LPCTSTR pszKey, int iVal);
	BOOL Write (LPCTSTR pszKey, DWORD dwVal);
	BOOL Write (LPCTSTR pszKey, LPCTSTR pszVal);
	BOOL Write (LPCTSTR pszKey, CStringList& scStringList);
	BOOL Write (LPCTSTR pszKey, CByteArray& bcArray);
	BOOL Write (LPCTSTR pszKey, CStringArray& scArray);
	BOOL Write (LPCTSTR pszKey, CDWordArray& dwcArray);
	BOOL Write (LPCTSTR pszKey, CWordArray& wcArray);
	BOOL Write (LPCTSTR pszKey, const CRect& rect);
	BOOL Write (LPCTSTR pszKey, LPPOINT& lpPoint);
	BOOL Write (LPCTSTR pszKey, LPBYTE pData, UINT nBytes);
	BOOL Write (LPCTSTR pszKey, CObList& list);
	BOOL Write (LPCTSTR pszKey, CObject& obj);

	BOOL Read (LPCTSTR pszKey, int& iVal);
	BOOL Read (LPCTSTR pszKey, DWORD& dwVal);
	BOOL Read (LPCTSTR pszKey, CString& sVal);
	BOOL Read (LPCTSTR pszKey, CStringList& scStringList);
	BOOL Read (LPCTSTR pszKey, CStringArray& scArray);
	BOOL Read (LPCTSTR pszKey, CDWordArray& dwcArray);
	BOOL Read (LPCTSTR pszKey, CWordArray& wcArray);
	BOOL Read (LPCTSTR pszKey, CByteArray& bcArray);
	BOOL Read (LPCTSTR pszKey, LPPOINT& lpPoint);
	BOOL Read (LPCTSTR pszKey, CRect& rect);
	BOOL Read (LPCTSTR pszKey, BYTE** ppData, UINT* pBytes);
	BOOL Read (LPCTSTR pszKey, CObList& list);
	BOOL Read (LPCTSTR pszKey, CObject& obj);

protected:

	HKEY		m_hKey;
	CString		m_sPath;
	const BOOL	m_bReadOnly;
};