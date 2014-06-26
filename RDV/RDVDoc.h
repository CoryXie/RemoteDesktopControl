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
class CRDVView;

class CRDVDoc : public CDocument
{
protected: // create from serialization only
	CRDVDoc();
	CRDVView * GetView();
	DECLARE_DYNCREATE(CRDVDoc)

// Attributes
public:
	CString m_csIp;
	CString m_csPort;
	CString m_csPassword;

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void OnCloseDocument();
	virtual void Serialize(CArchive & ar);

// Implementation
public:
	virtual ~CRDVDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


