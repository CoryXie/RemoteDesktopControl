// Remote Desktop Server and Client
// CTrayIcon Copyright 1996 Microsoft Systems Journal, by Paul DiLascia

#include "stdafx.h"
#include "trayicon.h"
#include <afxpriv.h>		// for AfxLoadString

IMPLEMENT_DYNAMIC(CTrayIcon, CCmdTarget)

CTrayIcon::CTrayIcon(UINT uID)
{
	// Initialize NOTIFYICONDATA
	memset(&m_nid,0,sizeof(m_nid));
	m_nid.cbSize = sizeof(m_nid);
	m_nid.uID = uID;

	// Use resource string as tip if there is one
	AfxLoadString(uID,m_nid.szTip,sizeof(m_nid.szTip));
}

CTrayIcon::~CTrayIcon()
{
	// Remove icon from system tray
	SetIcon(0);
}

// Set notification window. It must created already.
void CTrayIcon::SetNotificationWnd(CWnd* pNotifyWnd, UINT uCbMsg)
{
	// If the following assert fails, you're probably
	// calling me before you created your window. Oops.
	ASSERT(pNotifyWnd==NULL || ::IsWindow(pNotifyWnd->GetSafeHwnd()));
	m_nid.hWnd = pNotifyWnd->GetSafeHwnd();

	ASSERT(uCbMsg==0 || uCbMsg>=WM_USER);
	m_nid.uCallbackMessage = uCbMsg;
}

// Sets both the icon and tooltip from resource ID, to remove the icon, call SetIcon(0)
BOOL CTrayIcon::SetIcon(UINT uID)
{ 
	HICON hicon = NULL;
	if (uID)
	{
		AfxLoadString(uID, m_nid.szTip, sizeof(m_nid.szTip));
		hicon = AfxGetApp()->LoadIcon(uID);
	}
	return SetIcon(hicon, NULL);
}

// Common SetIcon for all overloads. 
BOOL CTrayIcon::SetIcon(HICON hicon, LPCSTR lpTip) 
{
	UINT msg;
	m_nid.uFlags = 0;

	// Set the icon
	if (hicon)
	{
		// Add or replace icon in system tray
		msg = m_nid.hIcon ? NIM_MODIFY : NIM_ADD;
		m_nid.hIcon = hicon;
		m_nid.uFlags |= NIF_ICON;
	}
	else
	{
		// remove icon from tray
		if (m_nid.hIcon==NULL)
			return TRUE;
		msg = NIM_DELETE;
	}

	// Use the tip, if any
	if (lpTip)
		strncpy(reinterpret_cast<char*>(m_nid.szTip), reinterpret_cast<const char *>(lpTip), sizeof(m_nid.szTip));
	if (m_nid.szTip[0])
		m_nid.uFlags |= NIF_TIP;

	// Use callback if any
	if (m_nid.uCallbackMessage && m_nid.hWnd)
		m_nid.uFlags |= NIF_MESSAGE;

	// Do it
	BOOL bRet = Shell_NotifyIcon(msg, &m_nid);
	if (msg==NIM_DELETE || !bRet)
		m_nid.hIcon = NULL;	// failed
	return bRet;
}

BOOL CTrayIcon::SetIcon(LPCTSTR lpResName,LPCSTR lpTip)
{
	return SetIcon(lpResName ? AfxGetApp()->LoadIcon(lpResName) : NULL, lpTip);
}

// Default event handler handles right-menu and doubleclick.
LRESULT CTrayIcon::OnNotifyTray(WPARAM wID, LPARAM lEvent)
{
	if (wID!=m_nid.uID || (lEvent!=WM_RBUTTONUP && lEvent!=WM_LBUTTONDBLCLK))
		return 0;

	// If there's a resource menu with the same ID as the icon, use it as 
	// the right-button popup menu. CTrayIcon will interprets the first
	// item in the menu as the default command for WM_LBUTTONDBLCLK
	// 
	CMenu menu;
	if (!menu.LoadMenu(m_nid.uID))
		return 0;
	CMenu* pSubMenu = menu.GetSubMenu(0);
	if (!pSubMenu) 
		return 0;

	if (lEvent==WM_RBUTTONUP)
	{
		// Make first menu item the default (bold font)
		::SetMenuDefaultItem(pSubMenu->m_hMenu, 0, TRUE);

		// Display the menu at the current mouse location. There's a "bug"
		// (Microsoft calls it a feature) in Windows 95 that requires calling
		// SetForegroundWindow. To find out more, search for Q135788 in MSDN.
		//
		CPoint mouse;
		GetCursorPos(&mouse);
		::SetForegroundWindow(m_nid.hWnd);	
		::TrackPopupMenu(pSubMenu->m_hMenu, 0, mouse.x, mouse.y, 0,
			m_nid.hWnd, NULL);
	}
	else  // double click: execute first menu item
		::SendMessage(m_nid.hWnd, WM_COMMAND, pSubMenu->GetMenuItemID(0), 0);

	return 1; // handled
}