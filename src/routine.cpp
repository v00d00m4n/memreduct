/************************************
*  	Routine Functions
*	Copyright © 2013 Henry++
*
*	GNU General Public License v2
*	http://www.gnu.org/licenses/
*
*	http://www.henrypp.org/
*************************************/

// lastmod: 30/03/13

#include "routine.h"

// Insert Item (TreeView)
HTREEITEM Tv_InsertItem(HWND hWnd, INT iCtrlId, CString lpszText, HTREEITEM hParent, INT iImage, INT iSelImage, LPARAM lParam)
{
	TVINSERTSTRUCT tvi = {0};
	
	tvi.hParent = hParent;
	tvi.hInsertAfter = TVI_LAST;
	tvi.itemex.mask = TVIF_TEXT;
	tvi.itemex.pszText = lpszText.GetBuffer();
	tvi.itemex.cchTextMax = lpszText.GetLength();

	if(lParam != -1)
	{
		tvi.itemex.mask |= TVIF_PARAM;
		tvi.itemex.lParam = lParam;
	}

	if(!hParent)
	{
		tvi.itemex.mask |= TVIF_STATE;
		tvi.itemex.state = TVIS_EXPANDED;
		tvi.itemex.stateMask = TVIS_EXPANDED;
	}

	if(iImage != -1)
	{
		tvi.itemex.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.itemex.iImage = iImage;
		tvi.itemex.iSelectedImage = (iSelImage == -1) ? iImage : iSelImage;
	}

	return (HTREEITEM)SendDlgItemMessage(hWnd, iCtrlId, TVM_INSERTITEM, 0, (LPARAM)&tvi);
}

// Set Extended Style (TreeView)
HRESULT Tv_SetStyleEx(HWND hWnd, INT iCtrlId, DWORD dwExStyle, BOOL bExplorerStyle, INT iItemHeight)
{
	if(bExplorerStyle)
		SetWindowTheme(GetDlgItem(hWnd, iCtrlId), L"Explorer", 0);

	if(iItemHeight)
		SendDlgItemMessage(hWnd, iCtrlId, TVM_SETITEMHEIGHT, iItemHeight, 0);

	return (HRESULT)SendDlgItemMessage(hWnd, iCtrlId, TVM_SETEXTENDEDSTYLE, 0, dwExStyle);
}

// Set Extended Style (ListView)
DWORD Lv_SetStyleEx(HWND hWnd, INT iCtrlId, DWORD dwExStyle, BOOL bExplorerStyle, BOOL bGroupView)
{
	if(bExplorerStyle)
		SetWindowTheme(GetDlgItem(hWnd, iCtrlId), L"Explorer", 0);

	SendDlgItemMessage(hWnd, iCtrlId, LVM_ENABLEGROUPVIEW, bGroupView, 0);

	return SendDlgItemMessage(hWnd, iCtrlId, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwExStyle);
}

// Get Item Text (ListView)
CString Lv_GetItemText(HWND hWnd, INT iCtrlId, INT cchTextMax, INT iItem, INT iSubItem)
{
	CString buffer;

	LVITEM lvi = {0};

	lvi.iSubItem = iSubItem;
	lvi.pszText = buffer.GetBuffer(cchTextMax);
	lvi.cchTextMax = cchTextMax;

	SendDlgItemMessage(hWnd, iCtrlId, LVM_GETITEMTEXT, iItem, (LPARAM)&lvi);
	buffer.ReleaseBuffer();

	return buffer;
}

// Insert Group (ListView)
INT Lv_InsertGroup(HWND hWnd, INT iCtrlId, CString lpszText, INT iGroupId, UINT uAlign, UINT uState)
{
	LVGROUP lvg = {0};

	lvg.cbSize = sizeof(lvg);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID;
	lvg.pszHeader = lpszText.GetBuffer();
	lvg.cchHeader = lpszText.GetLength();
	lvg.iGroupId = iGroupId;
	
	if(uAlign)
	{
		lvg.mask |= LVGF_ALIGN;
		lvg.uAlign = uAlign;
	}
	
	if(uState)
	{
		lvg.mask |= LVGF_STATE;
		lvg.state = uState;
	}

	return SendDlgItemMessage(hWnd, iCtrlId, LVM_INSERTGROUP, iGroupId, (LPARAM)&lvg);
}

// Insert Column (ListView)
INT Lv_InsertColumn(HWND hWnd, INT iCtrlId, CString lpszText, INT iWidth, INT iItem, INT iFmt)
{
	LVCOLUMN lvc = {0};

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
	lvc.pszText = lpszText.GetBuffer();
	lvc.cchTextMax = lpszText.GetLength();
	lvc.fmt = iFmt;
	lvc.cx = iWidth;
	lvc.iSubItem = iItem;

	return SendDlgItemMessage(hWnd, iCtrlId, LVM_INSERTCOLUMN, iItem, (LPARAM)&lvc);
}

// Insert Item (ListView)
INT Lv_InsertItem(HWND hWnd, INT iCtrlId, CString lpszText, INT iItem, INT iSubItem, INT iImage, INT iGroupId, LPARAM lParam)
{
	LVITEM lvi = {0}, lvi_tmp = {0};
	
	lvi.mask = LVIF_TEXT;
	lvi.pszText = lpszText.GetBuffer();
	lvi.cchTextMax = lpszText.GetLength();
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;

	if(iImage != -1)
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = iImage;
	}

	if(iGroupId != -1)
	{
		lvi.mask |= LVIF_GROUPID;
		lvi.iGroupId = iGroupId;
	}

	if(lParam && !iSubItem)
	{
		lvi.mask |= LVIF_PARAM;
		lvi.lParam = lParam;
	}
	else if(lParam && iSubItem)
	{
		lvi_tmp.mask = LVIF_PARAM;
		lvi_tmp.iItem = iItem;
		lvi_tmp.lParam = lParam;

		SendDlgItemMessage(hWnd, iCtrlId, LVM_SETITEM, 0, (LPARAM)&lvi_tmp);
	}

	return SendDlgItemMessage(hWnd, iCtrlId, (iSubItem > 0) ? LVM_SETITEM : LVM_INSERTITEM, 0, (LPARAM)&lvi);
}

// Number Format
CString number_format(LONGLONG lNumber, LPCWSTR lpszAppend, CONST WCHAR cSeparator)
{
	CString buffer = L"0";
	BOOL bHasNegative = lNumber < 0;

    if(lNumber)
	{
		buffer.Empty();

		do {
			if((buffer.GetLength() + 1) % 4 == 0)
				buffer.AppendChar(cSeparator);

			INT iMod = lNumber % 10;

			if(lNumber < 0)
				iMod = -iMod;

			buffer.AppendChar(iMod + L'0');
		} while(lNumber /= 10);

		if(bHasNegative)
			buffer.AppendChar(L'-');

		buffer.MakeReverse();
	}

	if(lpszAppend)
		buffer.Append(lpszAppend);

	buffer.AppendChar(L'\0');

	return buffer;
}

// Show Balloon Tip for Edit Control
BOOL ShowEditBalloonTip(HWND hWnd, INT iCtrlId, LPCTSTR lpcszTitle, LPCTSTR lpcszText, INT iIcon)
{
	EDITBALLOONTIP ebt = {0};

	ebt.cbStruct = sizeof(ebt);
	ebt.pszTitle = lpcszTitle;
	ebt.pszText = lpcszText;
	ebt.ttiIcon = iIcon;

	return (BOOL)SendDlgItemMessage(hWnd, iCtrlId, EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
}

// Admin Rights Checker
BOOL IsAdmin()
{
	BOOL bResult = FALSE;
	DWORD dwStatus = 0, dwACLSize = 0, dwStructureSize = sizeof(PRIVILEGE_SET);
	PACL pACL = NULL;
	PSID psidAdmin = NULL;

	HANDLE hToken = NULL, hImpersonationToken = NULL;

	PRIVILEGE_SET ps = {0};
	GENERIC_MAPPING gm = {0};

	PSECURITY_DESCRIPTOR psdAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	__try
	{
		if(!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &hToken))
		{
			if(GetLastError() != ERROR_NO_TOKEN || !OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &hToken))
				__leave;
		}

		if(!DuplicateToken(hToken, SecurityImpersonation, &hImpersonationToken))
			__leave;

		if(!AllocateAndInitializeSid(&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin))
			__leave;

		psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		
		if(!psdAdmin || !InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
			__leave;

		dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdmin) - sizeof(DWORD);
		pACL = (PACL)LocalAlloc(LPTR, dwACLSize);

		if(!pACL || !InitializeAcl(pACL, dwACLSize, ACL_REVISION2) || !AddAccessAllowedAce(pACL, ACL_REVISION2, ACCESS_READ | ACCESS_WRITE, psidAdmin) || !SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
			__leave;

		SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
		SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

		if(!IsValidSecurityDescriptor(psdAdmin))
			__leave;

		gm.GenericRead = ACCESS_READ;
		gm.GenericWrite = ACCESS_WRITE;
		gm.GenericExecute = 0;
		gm.GenericAll = ACCESS_READ | ACCESS_WRITE;

		if(!AccessCheck(psdAdmin, hImpersonationToken, ACCESS_READ, &gm, &ps, &dwStructureSize, &dwStatus, &bResult))
		{
			bResult = FALSE;
			__leave;
		}
	}

	__finally
	{
		if(pACL)
			LocalFree(pACL);

		if(psdAdmin)
			LocalFree(psdAdmin);

		if(psidAdmin)
			FreeSid(psidAdmin);

		if(hImpersonationToken)
			CloseHandle(hImpersonationToken);

		if(hToken)
			CloseHandle(hToken);
	}

	return bResult;
}

// UAC elevation checker
BOOL IsUnderUAC()
{
	HANDLE hToken = NULL; 
	TOKEN_ELEVATION_TYPE tet; 
	DWORD dwSize = 0;;

	// for older OS compatible
	if(!ValidWindowsVersion(6, 0))
		return FALSE;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) && GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(tet), &dwSize) && tet == TokenElevationTypeLimited)
		return TRUE;

	if(hToken)
		CloseHandle(hToken);

	return FALSE;
}

// Check is File Exists
BOOL FileExists(LPCTSTR lpcszPath)
{
	return (GetFileAttributes(lpcszPath) != INVALID_FILE_ATTRIBUTES);
}

// Privilege Enabler
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpcszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp = {0};
	LUID luid = {0};

    if(!LookupPrivilegeValue(NULL, lpcszPrivilege, &luid))
		return FALSE; 

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    if(!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return FALSE; 

    if(GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		return FALSE;

    return TRUE;
}

// Validate Windows Version
BOOL ValidWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEX osvi = {0};
	DWORDLONG dwMask = 0;

	// Initialize Structure
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;

	// Initialize Condition Mask
	VER_SET_CONDITION(dwMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	// Return Result
	return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwMask);
}

// Set Window Always on Top
VOID SetAlwaysOnTop(HWND hWnd, BOOL bEnable)
{
	SetWindowPos(hWnd, (bEnable ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

// Centering Window by Parent
VOID CenterDialog(HWND hWnd)
{
     HWND hParent = GetParent(hWnd);
	 RECT rc_child = {0}, rc_parent = {0};

	 // If Parent Doesn't Exists or Invisible - Use Desktop
	 if(!hParent || !IsWindowVisible(hParent))
		 hParent = GetDesktopWindow();

    GetWindowRect(hWnd, &rc_child);
    GetWindowRect(hParent, &rc_parent);
 
    INT iWidth = rc_child.right - rc_child.left, iHeight = rc_child.bottom - rc_child.top;
    INT iX = ((rc_parent.right - rc_parent.left) - iWidth) / 2 + rc_parent.left, iY = ((rc_parent.bottom - rc_parent.top) - iHeight) / 2 + rc_parent.top;
    INT iScreenWidth = GetSystemMetrics(SM_CXSCREEN), iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    if(iX < 0) iX = 0;
    if(iY < 0) iY = 0;
    if(iX + iWidth > iScreenWidth) iX = iScreenWidth - iWidth;
    if(iY + iHeight > iScreenHeight) iY = iScreenHeight - iHeight;
 
    MoveWindow(hWnd, iX, iY, iWidth, iHeight, 0);
}

// Toggle Visiblity for Window
VOID ToggleVisible(HWND hWnd)
{
	if(IsWindowVisible(hWnd))
	{
		ShowWindow(hWnd, SW_HIDE);
	}
	else
	{
		ShowWindow(hWnd, SW_SHOWNORMAL);
		SetForegroundWindow(hWnd);
	}
}

// Create Autorun Entry in Registry
VOID CreateAutorunEntry(LPCWSTR lpszName, BOOL bCreate)
{
	HKEY hKey = NULL;
	WCHAR szBuffer[MAX_PATH] = {0};

	if(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		return;

	if(bCreate)
	{
		GetModuleFileName(NULL, szBuffer, MAX_PATH);
		PathQuoteSpaces(szBuffer);

		RegSetValueEx(hKey, lpszName, 0, REG_SZ, (LPBYTE)szBuffer, MAX_PATH);
	}
	else
	{
		RegDeleteValue(hKey, lpszName);
	}

	RegCloseKey(hKey);
}

// Check Autorun Entry in Registry
BOOL IsAutorunExists(LPCWSTR lpszName)
{
	HKEY hKey = NULL;

	if(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		return FALSE;

	LONG dwRetCode = RegQueryValueEx(hKey, lpszName, 0, 0, 0, 0);

	RegCloseKey(hKey);

	return dwRetCode == ERROR_SUCCESS;
}

// String Localizer
CString ls(HINSTANCE hInstance, UINT uID)
{
	CString buffer;

	if(!buffer.LoadString(hInstance, uID) && hInstance)
		buffer.LoadString(0, uID);

	if(buffer.IsEmpty())
		buffer.Format(L"%d", uID);

	return buffer;
}

// Get Clipboard Text
CString ClipboardGet()
{
	CString buffer;

	if(OpenClipboard(NULL))
	{
		HGLOBAL hGlobal = GetClipboardData(CF_UNICODETEXT);

		if(hGlobal)
		{
			buffer = (LPWSTR)GlobalLock(hGlobal);

			if(!buffer.IsEmpty())
				GlobalUnlock(hGlobal);
		}
	}

	CloseClipboard();

	return buffer;
}

// Insert Text Into Clipboard
BOOL ClipboardPut(CString buffer)
{
	BOOL bResult = FALSE;

	if(OpenClipboard(NULL))
	{
		if(EmptyClipboard())
		{
			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (buffer.GetLength() + 1) * sizeof(TCHAR));

			if(hGlobal) 
			{
				LPWSTR pBuffer = (LPWSTR)GlobalLock(hGlobal);

				if(pBuffer)
				{
					StringCchCopy(pBuffer, buffer.GetLength() + 1, buffer);
					SetClipboardData(CF_UNICODETEXT, hGlobal);

					bResult = TRUE;

					GlobalUnlock(hGlobal);
				}
				else
				{
					GlobalFree(hGlobal);
				}
			}
		}

		CloseClipboard();
	}

	return bResult;
}

// Add Icon to ImageList
VOID ImageList_Add(HIMAGELIST hImgList, LPWSTR lpszIco)
{
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), lpszIco);
	ImageList_ReplaceIcon(hImgList, -1, hIcon);
	DestroyIcon(hIcon);
}

// Convert UnixTime to Windows SYSTEMTIME
BOOL UnixTimeToSystemTime(time_t t, SYSTEMTIME* pst)
{
	LONGLONG ll = 0; // 64 bit value 
	FILETIME ft = {0};

	ll = Int32x32To64(t, 10000000) + 116444736000000000ui64;

	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32); 

	return FileTimeToSystemTime(&ft, pst);
}

// Convert Windows SYSTEMTIME to UnixTime
time_t SystemTimeToUnixTime(SYSTEMTIME* pst)
{
	LONGLONG ll = 0; // 64 bit value 
	FILETIME ft = {0};

	SystemTimeToFileTime(pst, &ft);

	return (((((LONGLONG)(ft.dwHighDateTime)) << 32) + ft.dwLowDateTime) - 116444736000000000ui64) / 10000000ui64;
}

// Retrieve Window Coordinates (Width or Height) ("iVector" has "WIDTH" or "HEIGHT" macroses
INT GetWindowDimension(HWND hWnd, INT iVector, BOOL bClientOnly)
{
	RECT rc = {0};

	if(bClientOnly)
		GetClientRect(hWnd, &rc);
	else
		GetWindowRect(hWnd, &rc);

	if(IsRectEmpty(&rc))
		return 0;

	return iVector == WIDTH ? (rc.right - rc.left) : (rc.bottom - rc.top);
}

// MessageBox with printf routine
INT MessageBox(HWND hWnd, UINT uType, LPCWSTR lpcszCaption, LPCWSTR lpcszFormat, ...)
{
	va_list pArgList;
	va_start(pArgList, lpcszFormat);

	CString buffer;
	buffer.FormatV(lpcszFormat, pArgList);

	va_end(pArgList);

	return MessageBox(hWnd, buffer, lpcszCaption, uType);
}

// Create Tooltip and set to Control
HWND SetDlgItemTooltip(HWND hWnd, INT iDlgItem, LPWSTR lpszText)
{
	// Create Tooltips
	HWND hTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, GetModuleHandle(0), NULL);
                   
	TOOLINFO ti = {0};

	ti.cbSize = sizeof(ti);
	ti.hwnd = hWnd;
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.lpszText = lpszText;
	ti.uId = (UINT_PTR)GetDlgItem(hWnd, iDlgItem);

	SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

	return hTip;
}

// WM_MUTEX Wrapper
INT WmMutexWrapper(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
	if(wParam && (GetCurrentProcessId() != wParam))
	{
		if(!lParam)
		{
			SendMessage(hwndDlg, WM_CLOSE, 0, 0);
			return 1;
		}
		else
		{
			ShowWindow(hwndDlg, SW_SHOW);
			SetForegroundWindow(hwndDlg);

			return 1;
		}
	}

	return 0;
}

// About Dialog Callback
LRESULT CALLBACK AboutBoxProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_CREATE:
		{
			// Disable Parent
			if(GetParent(hwndDlg))
				EnableWindow(GetParent(hwndDlg), FALSE);

			// Centering By Parent
			CenterDialog(hwndDlg);

			break;
		}
		
		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
		{
			LONG lExStyle = GetWindowLong(hwndDlg, GWL_EXSTYLE);

			if(!(lExStyle & WS_EX_LAYERED))
				SetWindowLong(hwndDlg, GWL_EXSTYLE, lExStyle | WS_EX_LAYERED);

			SetLayeredWindowAttributes(hwndDlg, 0, 100, uMsg == WM_ENTERSIZEMOVE ? LWA_ALPHA : 0);
			SetCursor(LoadCursor(0, uMsg == WM_ENTERSIZEMOVE ? IDC_SIZEALL : IDC_ARROW));

			break;
		}

		case WM_LBUTTONDBLCLK:
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}

		case WM_DESTROY:
		{
			// Restore Parent
			if(GetParent(hwndDlg))
			{
				EnableWindow(GetParent(hwndDlg), TRUE);
				SetActiveWindow(GetParent(hwndDlg));
			}

			PostQuitMessage(0);

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
			RECT rc = {0};

			HDC hDC = BeginPaint(hwndDlg, &ps);

			GetClientRect(hwndDlg, &rc);
			rc.top = rc.bottom - 43;

			// Instead FillRect
			COLORREF clrOld = SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
			SetBkColor(hDC, clrOld);

			// Draw Line
			for(int i = 0; i < rc.right; i++)
				SetPixel(hDC, i, rc.top, GetSysColor(COLOR_BTNSHADOW));

			EndPaint(hwndDlg, &ps);

			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			SendMessage(hwndDlg, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			return (LPARAM)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK lpnmlnk = (PNMLINK)lParam;

					if(lpnmlnk->item.szUrl[0])
						ShellExecute(hwndDlg, 0, lpnmlnk->item.szUrl, NULL, NULL, SW_SHOWDEFAULT);
	
					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if(LOWORD(wParam) == 100 || LOWORD(wParam) == IDCANCEL)
				DestroyWindow(hwndDlg);

			break;
		}
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

// About Dialog
INT AboutBoxCreate(HWND hParent, LPWSTR lpszIcon, LPCWSTR lpcszTitle, LPCWSTR lpszAppName, LPCWSTR lpcszCopyright)
{
	MSG msg = {0};
	WNDCLASSEX wcex = {0};
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Check for Duplicate
	if(FindWindowEx(NULL, NULL, L"AboutBox", NULL))
		return 1;

	// Register Class
	if(!GetClassInfoEx(hInstance, L"AboutBox", &wcex))
	{
		wcex.cbSize = sizeof(wcex);
		wcex.hInstance = hInstance;
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = (WNDPROC)AboutBoxProc;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
		wcex.lpszClassName = L"AboutBox";

		if(!RegisterClassEx(&wcex))
			return 0;
	}

	// Create Window
	HWND hDlg = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, wcex.lpszClassName, lpcszTitle, WS_VISIBLE | WS_POPUP | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 353, 270, hParent, 0, hInstance, 0);

	if(!hDlg)
		return 0;

	// Get System Font
	LOGFONT lf = {0};
	GetObject((HFONT)SendMessage(hParent, WM_GETFONT, 0, 0), sizeof(lf), &lf);

	// Create Normal Font
	HFONT hFont = CreateFontIndirect(&lf);

	// Create Bold Font
	lf.lfWeight = FW_BOLD;
	lf.lfHeight = 14;
	HFONT hTitle = CreateFontIndirect(&lf);

	HWND hWnd = CreateWindowEx(0, WC_STATIC, 0, WS_VISIBLE | WS_CHILD | SS_ICON, 13, 13, 32, 32, hDlg, 0, hInstance, 0);
	SendMessage(hWnd, STM_SETICON, (WPARAM)LoadIcon(hInstance, lpszIcon), 0);
	
	hWnd = CreateWindowEx(0, WC_STATIC, lpszAppName, WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT | SS_CENTERIMAGE, 56, 13, 280, 32, hDlg, 0, hInstance, 0);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hTitle, 0);

	hWnd = CreateWindowEx(0, WC_LINK, lpcszCopyright, WS_VISIBLE | WS_CHILD | WS_GROUP | SS_LEFT, 56, 50, 280, 140, hDlg, 0, hInstance, 0);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, 0);

	hWnd = CreateWindowEx(0, WC_BUTTON, L"OK", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 270, GetWindowDimension(hDlg, HEIGHT, TRUE) - 33, 70, 23, hDlg, 0, hInstance, 0);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, 0);
	SetWindowLong(hWnd, GWL_ID, 100);

	while(GetMessage(&msg, 0, 0, 0))
	{
		if(!IsDialogMessage(hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DeleteObject(hFont);
	DeleteObject(hTitle);

	return msg.wParam;
}