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

// NewConnectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RDV.h"
#include "NewConnectionDlg.h"


// CNewConnectionDlg dialog

IMPLEMENT_DYNAMIC(CNewConnectionDlg, CDialog)
CNewConnectionDlg::CNewConnectionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewConnectionDlg::IDD, pParent)
	, m_csIp(_T("localhost"))
	, m_csPort(_T("8370"))
	, m_csPassword(_T("pass"))
{
}

CNewConnectionDlg::~CNewConnectionDlg()
{
}

void CNewConnectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_IP, m_csIp);
	DDV_MaxChars(pDX, m_csIp, 255);
	DDX_Text(pDX, IDC_PORT, m_csPort);
	DDV_MaxChars(pDX, m_csPort, 6);
	DDX_Text(pDX, IDC_PASSWORD, m_csPassword);
	DDV_MaxChars(pDX, m_csPassword, 255);
}


BEGIN_MESSAGE_MAP(CNewConnectionDlg, CDialog)
END_MESSAGE_MAP()


// CNewConnectionDlg message handlers
