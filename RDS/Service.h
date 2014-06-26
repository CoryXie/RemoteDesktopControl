#pragma once

#include <winsvc.h>

class CControlService
{
public:
	CControlService(const TCHAR * pszServiceName = NULL);
	~CControlService();

public:
	bool Open(const TCHAR * pszServiceName);
	void Close();
	bool IsOpen();
	bool IsAvailable();
	bool IsRunning();
	bool IsPaused();
	bool IsStopped();
	bool IsPending();
	bool Start();
	DWORD Stop();
	DWORD Pause();
	DWORD Continue();
	DWORD UserSync(DWORD dwOp = 0);
	bool Install(const TCHAR * pszServiceName,const TCHAR * pszDescription,const TCHAR * pszFileName);
	bool Remove(const TCHAR * pszServiceName);

protected:
	DWORD Query();
	DWORD Control(DWORD dwControl);

protected:
	SC_HANDLE m_hSCM;
	SC_HANDLE m_hService;
};
