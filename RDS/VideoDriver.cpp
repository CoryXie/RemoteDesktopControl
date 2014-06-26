//  Copyright (C) 2005-2006 Lev Kazarkin. All Rights Reserved.
//
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
#include "VideoDriver.h"
#include "..\Common.h"
//#include "vncDesktop.h"

char	vncVideoDriver::szDriverString[] = "Mirage Driver";
char	vncVideoDriver::szDriverStringAlt[] = "DemoForge Mirage Driver";
char	vncVideoDriver::szMiniportName[] = "dfmirage";

#define MINIPORT_REGISTRY_PATH	"SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services"

vncVideoDriver::vncVideoDriver()
{
	bufdata.buffer = NULL;
	bufdata.Userbuffer = NULL;
	m_fIsActive = false;
	m_fDirectAccessInEffect = false;
	m_fHandleScreen2ScreenBlt = false;
	*m_devname= 0;
	m_drv_ver_mj = 0;
	m_drv_ver_mn = 0;
}

vncVideoDriver::~vncVideoDriver()
{
	UnMapSharedbuffers();
	Deactivate();
	_ASSERTE(!m_fIsActive);
	_ASSERTE(!m_fDirectAccessInEffect);
}

#define	BYTE0(x)	((x) & 0xFF)
#define	BYTE1(x)	(((x) >> 8) & 0xFF)
#define	BYTE2(x)	(((x) >> 16) & 0xFF)
#define	BYTE3(x)	(((x) >> 24) & 0xFF)

BOOL vncVideoDriver::CheckVersion()
{
	_ASSERTE(IsWinNT());

	HDC	l_gdc= ::CreateDC(m_devname, NULL, NULL, NULL);
	if (!l_gdc)
	{
		DebugMsg("vncVideoDriver::CheckVersion: can't create DC on \"%s\"\n",m_devname);
		return FALSE;
	}

	Esc_dmf_Qvi_IN qvi_in;
	qvi_in.cbSize = sizeof(qvi_in);
	DebugMsg("Supported driver version is: min = %u.%u.%u.%u, cur = %u.%u.%u.%u\n",
		BYTE3(DMF_PROTO_VER_MINCOMPAT), BYTE2(DMF_PROTO_VER_MINCOMPAT), BYTE1(DMF_PROTO_VER_MINCOMPAT), BYTE0(DMF_PROTO_VER_MINCOMPAT),
		BYTE3(DMF_PROTO_VER_CURRENT), BYTE2(DMF_PROTO_VER_CURRENT), BYTE1(DMF_PROTO_VER_CURRENT), BYTE0(DMF_PROTO_VER_CURRENT));
	qvi_in.app_actual_version = DMF_PROTO_VER_CURRENT;
	qvi_in.display_minreq_version = DMF_PROTO_VER_MINCOMPAT;
	qvi_in.connect_options = 0;

	Esc_dmf_Qvi_OUT qvi_out;
	qvi_out.cbSize = sizeof(qvi_out);

	int drvCr = ExtEscape(
		l_gdc,
		ESC_QVI,
		sizeof(qvi_in), (LPSTR) &qvi_in,
		sizeof(qvi_out), (LPSTR) &qvi_out);
	DeleteDC(l_gdc);

	if (drvCr == 0)
	{
		DebugMsg("vncVideoDriver::CheckVersion: ESC_QVI not supported by this version of Mirage\n");
		return FALSE;
	}

	DebugMsg("Driver version is: display = %u.%u.%u.%u (build %u),"
		" miniport = %u.%u.%u.%u (build %u),"
		" appMinReq = %u.%u.%u.%u\n",
		BYTE3(qvi_out.display_actual_version), BYTE2(qvi_out.display_actual_version), BYTE1(qvi_out.display_actual_version), BYTE0(qvi_out.display_actual_version),
		qvi_out.display_buildno, 
		BYTE3(qvi_out.miniport_actual_version), BYTE2(qvi_out.miniport_actual_version), BYTE1(qvi_out.miniport_actual_version), BYTE0(qvi_out.miniport_actual_version),
		qvi_out.miniport_buildno,
		BYTE3(qvi_out.app_minreq_version), BYTE2(qvi_out.app_minreq_version), BYTE1(qvi_out.app_minreq_version), BYTE0(qvi_out.app_minreq_version));

	if (drvCr < 0)
	{
		DebugMsg("vncVideoDriver::CheckVersion: ESC_QVI call returned 0x%x\n",drvCr);
		return FALSE;
	}

	m_drv_ver_mj = BYTE3(qvi_out.display_actual_version);
	m_drv_ver_mn = BYTE2(qvi_out.display_actual_version);

	return TRUE;
}

BOOL vncVideoDriver::MapSharedbuffers(BOOL fForDirectScreenAccess)
{
	_ASSERTE(!m_fIsActive);
	_ASSERTE(!m_fDirectAccessInEffect);
	_ASSERTE(IsWinNT());

	HDC	l_gdc= ::CreateDC(m_devname, NULL, NULL, NULL);
	if (!l_gdc)
	{
		DebugMsg("vncVideoDriver::MapSharedbuffers: can't create DC on \"%s\"\n",m_devname);
		return FALSE;
	}

	oldCounter = 0;
	int drvCr = ExtEscape(
		l_gdc,
		MAP1,
		0, NULL,
		sizeof(GETCHANGESBUF), (LPSTR) &bufdata);
	DeleteDC(l_gdc);

	if (drvCr <= 0)
	{
		DebugMsg("vncVideoDriver::MapSharedbuffers: MAP1 call returned 0x%x\n",	drvCr);
		return FALSE;
	}
	m_fIsActive = true;
	if (fForDirectScreenAccess)
	{
		if (!bufdata.Userbuffer)
		{
			DebugMsg("vncVideoDriver::MapSharedbuffers: mirror screen view is NULL\n");
			return FALSE;
		}
		m_fDirectAccessInEffect = true;
	}
	else
	{
		if (bufdata.Userbuffer)
		{
			DebugMsg("vncVideoDriver::MapSharedbuffers: mirror screen view is mapped but direct access mode is OFF\n");
		}
	}

	// Screen2Screen support added in Mirage ver 1.2
	m_fHandleScreen2ScreenBlt = (m_drv_ver_mj > 1) || (m_drv_ver_mj == 1 && m_drv_ver_mn >= 2);

	return TRUE;	
}

BOOL vncVideoDriver::TestMapped()
{
	_ASSERTE(IsWinNT());

	TCHAR *pDevName;
	if (IsWinVerOrHigher(5, 0))
	{
		DISPLAY_DEVICE dd;
		INT devNum = 0;
		if (!LookupVideoDeviceAlt(szDriverString, szDriverStringAlt, devNum, &dd))
			return FALSE;
		pDevName = (TCHAR *)dd.DeviceName;
	}
	else
	{
		pDevName = "DISPLAY";
	}

	HDC	l_ddc = ::CreateDC(pDevName, NULL, NULL, NULL);
	if (l_ddc)
	{
		BOOL b = ExtEscape(l_ddc, TESTMAPPED, 0, NULL, 0, NULL);	
		DeleteDC(l_ddc);
		return b;
	}
	return FALSE;
}

void vncVideoDriver::UnMapSharedbuffers()
{
	_ASSERTE(IsWinNT());

	int DrvCr = 0;
	if (m_devname[0])
	{
		_ASSERTE(m_fIsActive);
		_ASSERTE(bufdata.buffer);
		_ASSERTE(!m_fDirectAccessInEffect || bufdata.Userbuffer);

		HDC	l_gdc= ::CreateDC(m_devname, NULL, NULL, NULL);
		if (!l_gdc)
		{
			DebugMsg("vncVideoDriver::UnMapSharedbuffers: can't create DC on \"%s\"\n",m_devname);
		}
		else
		{
			DrvCr = ExtEscape(
				l_gdc,
				UNMAP1,
				sizeof(GETCHANGESBUF), (LPSTR) &bufdata,
				0, NULL);
			DeleteDC(l_gdc);

			_ASSERTE(DrvCr > 0);
		}
	}
	// 0 return value is unlikely for Mirage because its DC is independent
	// from the reference device;
	// this happens with Quasar if its mode was changed externally.
	// nothing is particularly bad with it.

	if (DrvCr <= 0)
	{
		DebugMsg("vncVideoDriver::UnMapSharedbuffers failed. unmapping manually\n");
		if (bufdata.buffer)
		{
			BOOL br = UnmapViewOfFile(bufdata.buffer);
			DebugMsg("vncVideoDriver::UnMapSharedbuffers: UnmapViewOfFile(bufdata.buffer) returned %d\n",br);
		}
		if (bufdata.Userbuffer)
		{
			BOOL br = UnmapViewOfFile(bufdata.Userbuffer);
			DebugMsg("vncVideoDriver::UnMapSharedbuffers: UnmapViewOfFile(bufdata.Userbuffer) returned %d\n",br);
		}
	}
	m_fIsActive = false;
	m_fDirectAccessInEffect = false;
	m_fHandleScreen2ScreenBlt = false;
}

template <class TpFn>
HINSTANCE  LoadNImport(LPCTSTR szDllName, LPCTSTR szFName, TpFn &pfn)
{
	HINSTANCE hDll = LoadLibrary(szDllName);
	if (hDll)
	{
		pfn = (TpFn)GetProcAddress(hDll, szFName);
		if (pfn)
			return hDll;
		FreeLibrary(hDll);
	}
	DebugMsg("Can not import '%s' from '%s'.\n",szFName, szDllName);
	return NULL;
}

//BOOL vncVideoDriver::LookupVideoDevice(LPCTSTR szDeviceString, INT &devNum, DISPLAY_DEVICE *pDd)
BOOL vncVideoDriver::LookupVideoDeviceAlt(
	LPCTSTR szDevStr,
	LPCTSTR szDevStrAlt,
	INT &devNum,
	DISPLAY_DEVICE *pDd)
{
	_ASSERTE(IsWinVerOrHigher(5, 0));

	pEnumDisplayDevices pd = NULL;
	HINSTANCE  hInstUser32 = LoadNImport("User32.DLL", "EnumDisplayDevicesA", pd);
	if (!hInstUser32) return FALSE;

	ZeroMemory(pDd, sizeof(DISPLAY_DEVICE));
	pDd->cb = sizeof(DISPLAY_DEVICE);
	BOOL result;
	while (result = (*pd)(NULL,devNum, pDd, 0))
	{
		if (strcmp((const char *)pDd->DeviceString, szDevStr) == 0 ||
			szDevStrAlt && strcmp((const char *)pDd->DeviceString, szDevStrAlt) == 0)
		{
			DebugMsg("Found display device \"%s\": \"%s\"\n",pDd->DeviceString,pDd->DeviceName);
			break;
		}
		devNum++;
	}

	FreeLibrary(hInstUser32);
	return result;
}

HKEY vncVideoDriver::CreateDeviceKey(LPCTSTR szMpName)
{
	HKEY hKeyProfileMirror = (HKEY)0;
	if (RegCreateKey(
		HKEY_LOCAL_MACHINE,
		(MINIPORT_REGISTRY_PATH),
		&hKeyProfileMirror) != ERROR_SUCCESS)
	{
		DebugMsg("Can't access registry.\n");
		return FALSE;
	}
	HKEY hKeyProfileMp = (HKEY)0;
	LONG cr = RegCreateKey(
		hKeyProfileMirror,
		szMpName,
		&hKeyProfileMp);
	RegCloseKey(hKeyProfileMirror);
	if (cr != ERROR_SUCCESS)
	{
		DebugMsg("Can't access \"%s\" hardware profiles key.\n",szMpName);
		return FALSE;
	}
	HKEY hKeyDevice = (HKEY)0;
	if (RegCreateKey(
		hKeyProfileMp,
		("DEVICE0"),
		&hKeyDevice) != ERROR_SUCCESS)
	{
		DebugMsg("Can't access DEVICE0 hardware profiles key.\n");
	}
	RegCloseKey(hKeyProfileMp);
	return hKeyDevice;
}

BOOL vncVideoDriver::Activate(
							  BOOL fForDirectAccess,
							  const RECT *prcltarget)
{
	_ASSERTE(IsWinNT());

	if (IsWinVerOrHigher(5, 0))
	{
		return Activate_NT50(fForDirectAccess, prcltarget);
	}
	else
	{
		// NOTE: prcltarget is just a SourceDisplayRect.
		// there is only 1 possibility on NT4, so safely ignore it
		return Activate_NT46(fForDirectAccess);
	}
}

void vncVideoDriver::Deactivate()
{
	_ASSERTE(IsWinNT());

	if (IsWinVerOrHigher(5, 0))
	{
		Deactivate_NT50();
	}
	else
	{
		Deactivate_NT46();
	}
}

BOOL vncVideoDriver::Activate_NT50(
								   BOOL fForDirectAccess,
								   const RECT *prcltarget)
{
	HDESK   hdeskInput;
	HDESK   hdeskCurrent;

	DISPLAY_DEVICE dd;
	INT devNum = 0;
	if (!LookupVideoDeviceAlt(szDriverString, szDriverStringAlt, devNum, &dd))
	{
		DebugMsg("No '%s' or '%s' found.\n", szDriverString, szDriverStringAlt);
		return FALSE;
	}

	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL change = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (prcltarget)
	{
		// we always have to set position or
		// a stale position info in registry would come into effect.
		devmode.dmFields |= DM_POSITION;
		devmode.dmPosition.x = prcltarget->left;
		devmode.dmPosition.y = prcltarget->top;

		devmode.dmPelsWidth = prcltarget->right - prcltarget->left;
		devmode.dmPelsHeight = prcltarget->bottom - prcltarget->top;
	}

	devmode.dmDeviceName[0] = '\0';

	DebugMsg("DevNum:%d\nName:%s\nString:%s\n\n", devNum, &dd.DeviceName[0], &dd.DeviceString[0]);
	DebugMsg("Screen Top-Left Position: (%i, %i)\n", devmode.dmPosition.x, devmode.dmPosition.y);
	DebugMsg("Screen Dimensions: (%i, %i)\n", devmode.dmPelsWidth, devmode.dmPelsHeight);
	DebugMsg("Screen Color depth: %i\n", devmode.dmBitsPerPel);

	HKEY hKeyDevice = CreateDeviceKey(szMiniportName);
	if (hKeyDevice == NULL)
		return FALSE;

	// TightVNC does not use these features
	RegDeleteValue(hKeyDevice, ("Screen.ForcedBpp"));
	RegDeleteValue(hKeyDevice, ("Pointer.Enabled"));

	DWORD dwVal = fForDirectAccess ? 3 : 0;
	// NOTE that old driver ignores it and mapping is always ON with it
	if (RegSetValueEx(
		hKeyDevice,
		("Cap.DfbBackingMode"),
		0,
		REG_DWORD,
		(unsigned char *)&dwVal,
		4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set \"Cap.DfbBackingMode\" to %d\n", dwVal);
		return FALSE;
	}

	dwVal = 1;
	if (RegSetValueEx(
		hKeyDevice,
		("Order.BltCopyBits.Enabled"),
		0,
		REG_DWORD,
		(unsigned char *)&dwVal,
		4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set Order.BltCopyBits.Enabled to %d\n", dwVal);
		return FALSE;
	}

	dwVal = 1;
	if (RegSetValueEx(
		hKeyDevice,
		("Attach.ToDesktop"),
		0,
		REG_DWORD,
		(unsigned char *)&dwVal,
		4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set Attach.ToDesktop to %d\n", dwVal);
		return FALSE;
	}

	pChangeDisplaySettingsEx pCDS = NULL;
	HINSTANCE  hInstUser32 = LoadNImport("User32.DLL", "ChangeDisplaySettingsExA", pCDS);
	if (!hInstUser32) return FALSE;

	// Save the current desktop
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL)
	{
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL) 
			SetThreadDesktop(hdeskInput);
	}
	// 24 bpp screen mode is MUNGED to 32 bpp.
	// the underlying buffer format must be 32 bpp.
	// see vncDesktop::ThunkBitmapInfo()
	if (devmode.dmBitsPerPel==24) devmode.dmBitsPerPel = 32;
	LONG cr = (*pCDS)(
		(TCHAR *)dd.DeviceName,
		&devmode,
		NULL,
		CDS_UPDATEREGISTRY,NULL);
	if (cr != DISP_CHANGE_SUCCESSFUL)
	{
		DebugMsg("ChangeDisplaySettingsEx failed on device \"%s\" with status: 0x%x\n",dd.DeviceName,cr);
	}

	strcpy(m_devname, (const char *)dd.DeviceName);

	// Reset desktop
	SetThreadDesktop(hdeskCurrent);
	// Close the input desktop
	CloseDesktop(hdeskInput);
	RegCloseKey(hKeyDevice);
	FreeLibrary(hInstUser32);

	return TRUE;
}

BOOL vncVideoDriver::Activate_NT46(BOOL fForDirectAccess)
{
	HKEY hKeyDevice = CreateDeviceKey(szMiniportName);
	if (hKeyDevice == NULL)
		return FALSE;

	// TightVNC does not use these features
	RegDeleteValue(hKeyDevice, ("Screen.ForcedBpp"));
	RegDeleteValue(hKeyDevice, ("Pointer.Enabled"));

	DWORD dwVal = fForDirectAccess ? 3 : 0;
	// NOTE that old driver ignores it and mapping is always ON with it
	if (RegSetValueEx(
		hKeyDevice,
		("Cap.DfbBackingMode"),
		0,
		REG_DWORD,
		(unsigned char *)&dwVal,
		4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set \"Cap.DfbBackingMode\" to %d\n", dwVal);
		return FALSE;
	}

	dwVal = 1;
	if (RegSetValueEx(
		hKeyDevice,
		("Order.BltCopyBits.Enabled"),
		0,
		REG_DWORD,
		(unsigned char *)&dwVal,
		4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set Order.BltCopyBits.Enabled to %d\n", dwVal);
		return FALSE;
	}

	// NOTE: we cannot truly load the driver
	// but ChangeDisplaySettings makes PDEV to reload
	// and thus new settings come into effect

	// TODO

	strcpy(m_devname, "DISPLAY");

	RegCloseKey(hKeyDevice);
	return TRUE;
}

void vncVideoDriver::Deactivate_NT50()
{
	HDESK   hdeskInput;
	HDESK   hdeskCurrent;

	// it is important to us to be able to deactivate
	// even what we have never activated. thats why we look it up, all over
	//	if (!m_devname[0])
	//		return;
	// ... and forget the name
	*m_devname = 0;

	DISPLAY_DEVICE dd;
	INT devNum = 0;
	if (!LookupVideoDeviceAlt(szDriverString, szDriverStringAlt, devNum, &dd))
	{
		DebugMsg("No '%s' or '%s' found.\n", szDriverString, szDriverStringAlt);
		return;
	}

	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL change = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	devmode.dmDeviceName[0] = '\0';

	HKEY hKeyDevice = CreateDeviceKey(szMiniportName);
	if (hKeyDevice == NULL)
		return;

	DWORD one = 0;
	if (RegSetValueEx(hKeyDevice,("Attach.ToDesktop"), 0, REG_DWORD, (unsigned char *)&one,4) != ERROR_SUCCESS)
	{
		DebugMsg("Can't set Attach.ToDesktop to 0x1\n");
	}

	// reverting to default behavior
	RegDeleteValue(hKeyDevice, ("Cap.DfbBackingMode"));
	RegDeleteValue(hKeyDevice, ("Order.BltCopyBits.Enabled"));

	pChangeDisplaySettingsEx pCDS = NULL;
	HINSTANCE  hInstUser32 = LoadNImport("User32.DLL", "ChangeDisplaySettingsExA", pCDS);
	if (!hInstUser32) return;

	// Save the current desktop
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent != NULL)
	{
		hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
		if (hdeskInput != NULL)
			SetThreadDesktop(hdeskInput);
	}
	// 24 bpp screen mode is MUNGED to 32 bpp. see vncDesktop::ThunkBitmapInfo()
	if (devmode.dmBitsPerPel==24) devmode.dmBitsPerPel = 32;

	// Add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
	(*pCDS)((TCHAR *)dd.DeviceName, &devmode, NULL, CDS_UPDATEREGISTRY, NULL);

	// Reset desktop
	SetThreadDesktop(hdeskCurrent);
	// Close the input desktop
	CloseDesktop(hdeskInput);
	RegCloseKey(hKeyDevice);
	FreeLibrary(hInstUser32);
}

void vncVideoDriver::Deactivate_NT46()
{
	// ... and forget the name
	*m_devname = 0;

	HKEY hKeyDevice = CreateDeviceKey(szMiniportName);
	if (hKeyDevice == NULL)
		return;

	// reverting to default behavior
	RegDeleteValue(hKeyDevice, ("Cap.DfbBackingMode"));

	//	RegDeleteValue(hKeyDevice, ("Order.BltCopyBits.Enabled"));
	// TODO: remove "Order.BltCopyBits.Enabled"
	// now we don't touch this important option
	// because we dont apply the changed values

	RegCloseKey(hKeyDevice);
}

void vncVideoDriver::HandleDriverChanges(int xoffset,int yoffset)
{
	ULONG snapshot_counter = bufdata.buffer->counter;
	if (oldCounter == snapshot_counter)
		return;

	if (oldCounter < snapshot_counter)
	{
		HandleDriverChangesSeries(xoffset, yoffset,bufdata.buffer->pointrect + oldCounter,bufdata.buffer->pointrect + snapshot_counter);
	}
	else
	{
		HandleDriverChangesSeries(xoffset,yoffset,bufdata.buffer->pointrect + oldCounter,bufdata.buffer->pointrect + MAXCHANGES_BUF);

		HandleDriverChangesSeries(xoffset,yoffset,bufdata.buffer->pointrect,bufdata.buffer->pointrect + snapshot_counter);
	}

	oldCounter = snapshot_counter;
}

void vncVideoDriver::HandleDriverChangesSeries(int xoffset,int yoffset,const CHANGES_RECORD *i,const CHANGES_RECORD *last)
{
	for (; i < last; i++)
	{
		if (i->type >= dmf_dfo_SCREEN_SCREEN && i->type <= dmf_dfo_TEXTOUT)
		{
			DebugMsg("Rect (%d,%d,%d,%d)\n",i->rect.left,i->rect.top,i->rect.right,i->rect.bottom);
		}
	}
}

// Ported from Tight VNC
BOOL CRDSDlg::InitVideoDriver()
{
	// Mirror video drivers supported under Win2K, WinXP, WinVista
	// and Windows NT 4.0 SP3 (we assume SP6).
	if (!IsWinNT())
		return FALSE;

	// FIXME: Windows NT 4.0 support is broken and thus we disable it here.
	if (!IsWinVerOrHigher(5, 0))
		return FALSE;

	BOOL bSolicitDASD = FALSE;

	_ASSERTE(!m_videodriver);
	m_videodriver = new vncVideoDriver;
	if (!m_videodriver)
	{
		DebugMsg("failed to create vncVideoDriver object\n");
		return FALSE;
	}

	if (IsWinVerOrHigher(5, 0))
	{
// restart the driver if left running.
// NOTE that on NT4 it must be running beforehand
		if (m_videodriver->TestMapped())
		{
			DebugMsg("found abandoned Mirage driver running. restarting.\n");
			m_videodriver->Deactivate();
		}
		_ASSERTE(!m_videodriver->TestMapped());
	}

	{
		CRect vdesk_rect(m_cxStart,m_cyStart,m_cxWidth,m_cyHeight);
		BOOL b = m_videodriver->Activate(bSolicitDASD,&vdesk_rect);
	}

	if (!m_videodriver->CheckVersion())
	{
		DebugMsg("******** PLEASE INSTALL NEWER VERSION OF MIRAGE DRIVER! ********\n");
// IMPORTANT: fail on NT46
		if (IsNtVer(4, 0))
			return FALSE;
	}

	if (m_videodriver->MapSharedbuffers(bSolicitDASD))
	{
		DebugMsg("video driver interface activated\n");
	}
	else
	{
		delete m_videodriver;
		m_videodriver = NULL;
		DebugMsg("failed to activate video driver interface\n");
		return FALSE;
	}
	_ASSERTE(bSolicitDASD == m_videodriver->IsDirectAccessInEffect());
	return TRUE;
}

// Ported from Tight VNC
void CRDSDlg::ShutdownVideoDriver()
{
	if (m_videodriver == NULL)
		return;
	delete m_videodriver;
	m_videodriver = NULL;
	DebugMsg("video driver interface deactivated\n");
}

