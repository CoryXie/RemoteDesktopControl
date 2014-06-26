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

// CNewConnectionDlg dialog

class CNewConnectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CNewConnectionDlg)

public:
	CNewConnectionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNewConnectionDlg();

// Dialog Data
	enum { IDD = IDD_NEWCONNECTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// The server ip or name
	CString m_csIp;
	// The negotiating port on the server
	CString m_csPort;
	// Password for the server
	CString m_csPassword;
};
