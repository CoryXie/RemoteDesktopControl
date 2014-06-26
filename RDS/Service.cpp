#include "stdafx.h"
#include "..\Common.h"
#include "Service.h"
#include <aclapi.h>

CControlService::CControlService(const TCHAR * pszServiceName) : m_hSCM(NULL), m_hService(NULL)
{
	// Open the SCM
	m_hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (m_hSCM)
	{
		if (pszServiceName)
			Open(pszServiceName);
	}
}

CControlService::~CControlService()
{
	Close();
}

bool CControlService::Open(const TCHAR * pszServiceName)
{
	try
	{
		if (IsOpen())
			Close();
		if (m_hSCM && pszServiceName)
		{
			// Open the service
			m_hService = OpenService(m_hSCM,pszServiceName,SC_MANAGER_ALL_ACCESS | SERVICE_INTERROGATE | SERVICE_USER_DEFINED_CONTROL);
			if (m_hService)
				return true;
		}
	}
	catch (...)
	{
	}
	return false;
}

void CControlService::Close()
{
	try
	{
		// Close the service
		if (m_hService)
		{
			CloseServiceHandle(m_hService);
			m_hService = NULL;
		}
	}
	catch (...)
	{
	}
}

// Test for the service being open and available for controlling or querying
bool CControlService::IsOpen()
{
	return m_hService != NULL ? true : false;
}

// Test the service for availability
bool CControlService::IsAvailable()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Query the service
		DWORD dwQuery = Query();
		switch (dwQuery)
		{
		case ERROR_SERVICE_NOT_ACTIVE:
		case SERVICE_STOPPED:
		case SERVICE_START_PENDING:
		case SERVICE_STOP_PENDING:
		case SERVICE_RUNNING:
		case SERVICE_CONTINUE_PENDING:
		case SERVICE_PAUSE_PENDING:
		case SERVICE_PAUSED:
			bFlag = true;
			break;
		default:
			break;
		}
	}
	return bFlag;
}

// Test the service for being in a running state
bool CControlService::IsRunning()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Query the service
		DWORD dwQuery = Query();
		switch (dwQuery)
		{
		case SERVICE_RUNNING:
			bFlag = true;
			break;
		default:
			break;
		}
	}
	return bFlag;
}

// Test the service for being in a paused state
bool CControlService::IsPaused()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Query the service
		DWORD dwQuery = Query();
		switch (dwQuery)
		{
		case SERVICE_PAUSED:
			bFlag = true;
			break;
		default:
			break;
		}
	}
	return bFlag;
}

// Test the service for being in a stopped state
bool CControlService::IsStopped()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Query the service
		DWORD dwQuery = Query();
		switch (dwQuery)
		{
		case SERVICE_STOPPED:
		case ERROR_SERVICE_NOT_ACTIVE:
			bFlag = true;
			break;
		default:
			break;
		}
	}
	return bFlag;
}

// Test the service for being in a pending state
bool CControlService::IsPending()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Query the service
		DWORD dwQuery = Query();
		switch (dwQuery)
		{
		case SERVICE_START_PENDING:
		case SERVICE_STOP_PENDING:
		case SERVICE_CONTINUE_PENDING:
		case SERVICE_PAUSE_PENDING:
			bFlag = true;
			break;
		default:
			break;
		}
	}
	return bFlag;
}

// Start the service
bool CControlService::Start()
{
	bool bFlag = false;
	if (IsOpen())
	{
		// Start the service
		bFlag = StartService(m_hService,0,NULL) ? true : false;
	}
	return bFlag;
}

// Stop the service
DWORD CControlService::Stop()
{
	return Control(SERVICE_CONTROL_STOP);
}

// Pause the service
DWORD CControlService::Pause()
{
	return Control(SERVICE_CONTROL_PAUSE);
}

// Continue the service
DWORD CControlService::Continue()
{
	return Control(SERVICE_CONTROL_CONTINUE);
}

// Synchronize the service to receive a named pipe connection
DWORD CControlService::UserSync(DWORD dwOp)
{
	return Control(SERVICE_CONTROL_USER + dwOp);
}

// Query the service status
DWORD CControlService::Query()
{
	return Control(SERVICE_CONTROL_INTERROGATE);
}

// Control or query the service
DWORD CControlService::Control(DWORD dwControl)
{
	DWORD dwRet = ERROR_ACCESS_DENIED;
	try
	{
		if (IsOpen())
		{
			// Intialize the service status
			SERVICE_STATUS ServiceStatus;
			memset(&ServiceStatus,0,sizeof(ServiceStatus));
			if (ControlService(m_hService,dwControl,&ServiceStatus))
			{
				dwRet = ServiceStatus.dwCurrentState;
				switch (dwRet)
				{
				case SERVICE_STOPPED:
					DebugMsg("The service is not running\n");
					break;
				case SERVICE_START_PENDING:
					DebugMsg("The service is starting\n");
					break;
				case SERVICE_STOP_PENDING:
					DebugMsg("The service is stopping\n");
					break;
				case SERVICE_RUNNING:
					DebugMsg("The service is running\n");
					break;
				case SERVICE_CONTINUE_PENDING:
					DebugMsg("The service continue is pending\n");
					break;
				case SERVICE_PAUSE_PENDING:
					DebugMsg("The service pause is pending\n");
					break;
				case SERVICE_PAUSED:
					DebugMsg("The service is paused\n");
					break;
				case ERROR_SERVICE_NOT_ACTIVE:
					DebugMsg("The service is not active\n");
					break;
				default:
					DebugMsg("The service returned code %d\n",dwRet);
					break;
				}
			}
			else
			{
				dwRet = GetLastError();
				DebugLastError();
			}
		}
	}
	catch (...)
	{
	}
	return dwRet;
}

// Install the service
bool CControlService::Install(const TCHAR * pszServiceName, const TCHAR * pszDescription, const TCHAR * pszFileName)
{
	bool bInstall = false;
	if (m_hSCM && !IsOpen())
	{
		// Create the service
		m_hService = CreateService(m_hSCM,pszServiceName,pszServiceName,SERVICE_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,pszFileName,NULL,NULL,NULL,NULL,NULL);
		if (m_hService)
		{
			// Set the description of the service
			SERVICE_DESCRIPTION Desc;
			Desc.lpDescription = (LPSTR)pszDescription;
			ChangeServiceConfig2(m_hService,SERVICE_CONFIG_DESCRIPTION,&Desc);
			bInstall = true;
		}
	}
	return bInstall;
}

// Remove the service
bool CControlService::Remove(const TCHAR * pszServiceName)
{
	bool bRemove = false;
	if (m_hSCM)
	{
		if (!IsOpen())
			Open(pszServiceName);
		if (IsOpen())
		{
			if (DeleteService(m_hService)) 
			{
				Close();
				bRemove = true;
			}
		}
	}
	return bRemove;
}