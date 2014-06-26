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

#include "TCPSocket.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////
// Window sink message class
class CWndSink : public CWnd
{
// Construction
public:
	CWndSink();
	~CWndSink();

// Generated message map functions
protected:
	//{{AFX_MSG(CWndSink)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG

protected:
	// The client connectiun
	CTCPSocket m_Socket;

	DECLARE_MESSAGE_MAP()
};
