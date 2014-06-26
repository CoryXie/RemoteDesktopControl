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

// RDS.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "..\Common.h"
#include "RDS.h"
#include "RDSDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRDSApp

BEGIN_MESSAGE_MAP(CRDSApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRDSApp construction

CRDSApp::CRDSApp()
{
	// Initialize the O/S version information
	InitVersion();

	// Output the licensing disclaimer
    DebugMsg("Remote Desktop System, Copyright (C) 2000-2009 GravyLabs LLC\n");
    DebugMsg("GravyLabs LLC can be reached at jones.gravy@gmail.com\n");
    DebugMsg("Remote Desktop System comes with ABSOLUTELY NO WARRANTY\n");
    DebugMsg("This is free software, and you are welcome to redistribute it\n");
    DebugMsg("under of the GNU General Public License as published by\n");
	DebugMsg("the Free Software Foundation; version 2 of the License.\n");
}

CRDSApp::~CRDSApp()
{
}
// The one and only CRDSApp object

CRDSApp theApp;


// CRDSApp initialization

BOOL CRDSApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	// Test the O/S version information for XP or later
	if (IsWinNT() && IsWinVerOrHigher(5,1))
	{
		CRDSDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();
	}
	else
		AfxMessageBox("Requires Windows XP or later");

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}