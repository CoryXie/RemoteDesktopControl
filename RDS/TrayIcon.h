// Remote Desktop Server and Client
// CTrayIcon Copyright 1996 Microsoft Systems Journal, by Paul DiLascia

#ifndef _TRAYICON_H
#define _TRAYICON_H

#define WM_NOTIFY_TRAY WM_APP + 10

// CTrayIcon class
class CTrayIcon : public CCmdTarget
{
protected:
	DECLARE_DYNAMIC(CTrayIcon)
	NOTIFYICONDATA m_nid;// Shell_NotifyIcon args

public:
	CTrayIcon(UINT uID);
	~CTrayIcon();

	virtual LRESULT OnNotifyTray(WPARAM uID, LPARAM lEvent);

	// Call this to receive tray notifications
	void SetNotificationWnd(CWnd * pNotifyWnd,UINT uCbMsg);

	// SetIcon functions. To remove icon, call SetIcon(0)
	BOOL SetIcon(UINT uID);

protected:
	BOOL SetIcon(HICON hicon, LPCSTR lpTip);
	BOOL SetIcon(LPCTSTR lpResName,LPCSTR lpTip);
};

#endif
