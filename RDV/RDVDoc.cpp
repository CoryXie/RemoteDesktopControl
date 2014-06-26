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
#include "RDVDoc.h"
#include "RDVView.h"
#include "NewConnectionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRDVDoc

IMPLEMENT_DYNCREATE(CRDVDoc, CDocument)

BEGIN_MESSAGE_MAP(CRDVDoc, CDocument)
END_MESSAGE_MAP()


// CRDVDoc construction/destruction

CRDVDoc::CRDVDoc()
{
}

CRDVDoc::~CRDVDoc()
{
}

// Return the view
CRDVView * CRDVDoc::GetView()
{
	CRDVView * pView = NULL;
	POSITION Pos = GetFirstViewPosition();
	if (Pos)
		pView = (CRDVView *)GetNextView(Pos);
	return pView;
}

BOOL CRDVDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// Display the connection dialog
	CNewConnectionDlg NewConnection;
	if (NewConnection.DoModal() != IDOK)
		return FALSE;

	// Set the connection specifics for a successful connection
	m_csIp = NewConnection.m_csIp;
	m_csPort = NewConnection.m_csPort;
	m_csPassword = NewConnection.m_csPassword;

	// Connect to the server
	GetView()->ConnectServer();

	return TRUE;
}

void CRDVDoc::OnCloseDocument()
{
	// Close all active connections
	CRDVView * pView = GetView();
	if (pView)
		pView->DisconnectServer();

	// Close the document
	CDocument::OnCloseDocument();
}

// CRDVDoc serialization

void CRDVDoc::Serialize(CArchive & ar)
{
	if (ar.IsStoring())
	{
		// Save the connection information
		ar << m_csIp;
		ar << m_csPort;
		ar << m_csPassword;
	}
	else
	{
		// Load the connection information
		ar >> m_csIp;
		ar >> m_csPort;
		ar >> m_csPassword;

		// Connect to the server
		GetView()->ConnectServer();
	}
}

// CRDVDoc diagnostics

#ifdef _DEBUG
void CRDVDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRDVDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

// CRDVDoc commands
