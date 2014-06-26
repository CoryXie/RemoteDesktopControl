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
#include "WndSink.h"

// Handle the socket notifications as well as the basic initialization
BEGIN_MESSAGE_MAP(CWndSink,CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

// Constructor
CWndSink::CWndSink() : CWnd()
{
	// Create the window
	if (!CreateEx(0,AfxRegisterWndClass(0),_T("Window Notification Sink"),WS_OVERLAPPED,0,0,0,0,NULL,NULL))
		DebugMsg("%s\n",TEXT("Failed to create a Window Notification Sink"));
}

// Destructor
CWndSink::~CWndSink()
{
}

// Create the window
int CWndSink::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	MoveWindow(0,0,0,0);
	return 0;
}

// Paint the window
void CWndSink::OnPaint()
{
	CPaintDC dc(this);
}