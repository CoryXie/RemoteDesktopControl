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
#include "RDV.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RDVDoc.h"
#include "RDVView.h"
#include <IPHlpApi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "mpr.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRDVApp

BEGIN_MESSAGE_MAP(CRDVApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CRDVApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_VIEW_PEERS, &CRDVApp::OnViewNeighbors)
END_MESSAGE_MAP()


// CRDVApp construction

CRDVApp::CRDVApp()
{
	// Initialize the O/S version information
	InitVersion();

	// Output the licensing disclaimer
	DebugMsg(_T("Remote Desktop System, Copyright (C) 2000-2009 GravyLabs LLC\n"));
	DebugMsg(_T("GravyLabs LLC can be reached at jones.gravy@gmail.com\n"));
	DebugMsg(_T("Remote Desktop System comes with ABSOLUTELY NO WARRANTY\n"));
	DebugMsg(_T("This is free software, and you are welcome to redistribute it\n"));
	DebugMsg(_T("under of the GNU General Public License as published by\n"));
	DebugMsg(_T("the Free Software Foundation; version 2 of the License.\n"));
}

// The one and only CRDVApp object
CRDVApp theApp;

// CRDVApp initialization
BOOL CRDVApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Test the O/S version information for XP or later
	if (!IsWinNT() || !IsWinVerOrHigher(5,1))
	{
		AfxMessageBox(_T("Requires Windows XP or later"));
		return FALSE;
	}

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Remote Desktop System"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_RDVTYPE,
		RUNTIME_CLASS(CRDVDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CRDVView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Prevent an initial document from being created
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CRDVApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CRDVApp message handlers

// View a list of neighbor machines with their host names and ip addresses
afx_msg void CRDVApp::OnViewNeighbors()
{
	NETRESOURCE *pNetResource;
	HANDLE		hEnum;
	DWORD		dwResult;
	DWORD		dwSize = 16386;
	DWORD		dwNum;
	struct  hostent* host = NULL;
	WSADATA		wsaData;

	CString msg;

	dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, RESOURCEDISPLAYTYPE_SERVER, NULL, &hEnum);
	if (dwResult != 0)
	{
		AfxMessageBox(_T("WNetOpenEnum failed.\n"));
		return;
	}

	pNetResource = (LPNETRESOURCE)GlobalAlloc(GPTR, dwSize);
	if (pNetResource == NULL)
	{
		AfxMessageBox(_T("GlobalAlloc failed.\n"));
		return;
	}

	dwNum = -1;

	dwResult = WNetEnumResource(hEnum, &dwNum, pNetResource, &dwSize);
	if (dwResult != 0)
	{
		AfxMessageBox(_T("WNetEnumResource failed.\n"));
		return;
	}

	for (DWORD i = 0; i < dwNum; i++)
	{
		if (pNetResource[i].lpRemoteName == NULL)
			continue;

		CString RemoteName = CString(pNetResource[i].lpRemoteName);

		AfxMessageBox(RemoteName);

		if (0 == RemoteName.Left(2).Compare(CString("\\\\")))
		{
			RemoteName = RemoteName.Right(RemoteName.GetLength() - 2);	

			host = gethostbyname((char*)(LPCTSTR)RemoteName);

			if (host != NULL)
			{
				msg.Format(_T("%s - %s\n"), host->h_name, inet_ntoa(*(in_addr *)host->h_addr));

				AfxMessageBox(msg);
			}
		}
	}

	GlobalFree(pNetResource);
	WNetCloseEnum(hEnum);
}
