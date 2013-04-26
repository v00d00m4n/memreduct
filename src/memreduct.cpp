/************************************
*  	Mem Reduct
*	Copyright © 2013 Henry++
*
*	GNU General Public License v2
*	http://www.gnu.org/licenses/
*
*	http://www.henrypp.org/
*************************************/

// Include
#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlobj.h>
#include <atlstr.h> // cstring
#include <process.h> // _beginthreadex

#include "memreduct.h"
#include "routine.h"
#include "resource.h"
#include "ini.h"

INI ini;
CONFIG cfg = {0};
CONST INT WM_MUTEX = RegisterWindowMessage(APP_NAME_SHORT);
TAB_PAGES tab_pages = {{IDS_PAGE_1, IDS_PAGE_2, IDS_PAGE_3, IDS_PAGE_4, IDS_PAGE_5, IDS_PAGE_6}, {IDD_PAGE_1, IDD_PAGE_2, IDD_PAGE_3, IDD_PAGE_4, IDD_PAGE_5, IDD_PAGE_6}, {0}, 0, {0}};
NOTIFYICONDATA nid = {0};

// Check for updates
UINT WINAPI CheckUpdates(LPVOID lpParam)
{
	BOOL bStatus = FALSE;
	HINTERNET hInternet = NULL, hConnect = NULL;

	// Disable Menu
	EnableMenuItem(GetMenu(cfg.hWnd), IDM_CHECK_UPDATES, MF_BYCOMMAND | MF_DISABLED);

	// Connect
	if((hInternet = InternetOpen(APP_USERAGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0)) && (hConnect = InternetOpenUrl(hInternet, APP_WEBSITE L"/update.php?product=" APP_NAME_SHORT, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0)))
	{
		// Get Status
		DWORD dwStatus = 0, dwStatusSize = sizeof(dwStatus);
		HttpQueryInfo(hConnect, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL);

		// Check Errors
		if(dwStatus == HTTP_STATUS_OK)
		{
			// Reading
			ULONG ulReaded = 0;
			CHAR szBufferA[MAX_PATH] = {0};

			if(InternetReadFile(hConnect, szBufferA, MAX_PATH, &ulReaded) && ulReaded)
			{
				// Convert to Unicode
				CA2W newver(szBufferA, CP_UTF8);

				// If CURVER < NEWVER
				if(VersionCompare(APP_VERSION, newver.m_psz) == 1)
				{
					if(MessageBox(cfg.hWnd, MB_YESNO | MB_ICONQUESTION, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_YES), newver) == IDYES)
						ShellExecute(cfg.hWnd, 0, APP_WEBSITE L"/?product=" APP_NAME_SHORT, NULL, NULL, SW_SHOWDEFAULT);
				}
				else
				{
					if(!lpParam)
						MessageBox(cfg.hWnd, MB_OK | MB_ICONINFORMATION, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_NO));
				}

				// Good Result
				bStatus = TRUE;
			}
		}
	}

	if(!bStatus && !lpParam)
		MessageBox(cfg.hWnd, MB_OK | MB_ICONSTOP, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_ERROR));

	// Restore Menu
	EnableMenuItem(GetMenu(cfg.hWnd), IDM_CHECK_UPDATES, MF_BYCOMMAND | MF_ENABLED);

	// Clear Memory
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return bStatus;
}

// Get memory usage
DWORD GetMemoryUsage(LPMEMORY_USAGE lpmu)
{
	MEMORYSTATUSEX msex = {0};
	msex.dwLength = sizeof(msex);

	if(GlobalMemoryStatusEx(&msex))
	{
		// Physical
		lpmu->dwPercentPhys = msex.dwMemoryLoad;

		lpmu->ullFilledPhys = (msex.ullTotalPhys - msex.ullAvailPhys) / cfg.uUnitDivider;
		lpmu->ullTotalPhys = msex.ullTotalPhys / cfg.uUnitDivider;

		// Pagefile
		lpmu->dwPercentPageFile = DWORD((msex.ullTotalPageFile - msex.ullAvailPageFile) / (msex.ullTotalPageFile / 100));

		lpmu->ullFilledPageFile = (msex.ullTotalPageFile - msex.ullAvailPageFile) / cfg.uUnitDivider;
		lpmu->ullTotalPageFile = msex.ullTotalPageFile / cfg.uUnitDivider;
	}

	SYSTEM_CACHE_INFORMATION sci = {0};
	if(NT_SUCCESS(NtQuerySystemInformation(SystemFileCacheInformation, &sci, sizeof(sci), 0)))
	{
		// System working set
		lpmu->dwPercentSystemWorkingSet = sci.CurrentSize / (sci.PeakSize / 100);
	
		lpmu->ullFilledSystemWorkingSet = sci.CurrentSize / cfg.uUnitDivider;
		lpmu->ullTotalSystemWorkingSet = sci.PeakSize / cfg.uUnitDivider;
	}

	// Return percents
	switch(ini.read(APP_NAME_SHORT, L"TrayMemoryRegion", 0))
	{
		case 1:
			return lpmu->dwPercentPageFile;

		case 2:
			return lpmu->dwPercentSystemWorkingSet;
	}

	return lpmu->dwPercentPhys;
}

// Show tray balloon tip
BOOL ShowBalloonTip(DWORD dwInfoFlags, LPCWSTR lpcszTitle, LPCWSTR lpcszMessage)
{
	// Check interval
	if(cfg.dwLastBalloon && (GetTickCount() - cfg.dwLastBalloon) < ((UINT)ini.read(APP_NAME_SHORT, L"BalloonInterval", 10)) * 1000)
		return FALSE;

	// Configure structure
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_RESPECT_QUIET_TIME | dwInfoFlags;

	// Set text
	StringCchCopy(nid.szInfo, _countof(nid.szInfo), lpcszMessage);
	StringCchCopy(nid.szInfoTitle, _countof(nid.szInfoTitle), lpcszTitle);

	// Show balloon
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	// Clear for Prevent reshow
	nid.szInfo[0] = 0;
	nid.szInfoTitle[0] = 0;

	// Keep last show-time
	cfg.dwLastBalloon = GetTickCount();

	return TRUE;
}

// Create HICON with memory usage info
HICON CreateMemIcon(DWORD dwData)
{
	CString buffer;
	RECT rc = {0, 0, 16, 16}; // icon rect

	BOOL TrayChangeBackground = ini.read(APP_NAME_SHORT, L"TrayChangeBackground", 1);
	BOOL TrayShowFree = ini.read(APP_NAME_SHORT, L"TrayShowFree", 0);

	// Initialize Color
	COLORREF clrTextColor = ini.read(APP_NAME_SHORT, L"TrayTextClr", COLOR_TRAY_TEXT);
	COLORREF clrIndicator = NULL;

	if(cfg.bColorIndicationTray)
	{
		if(dwData >= cfg.uRedLevel)
		{
			clrIndicator = ini.read(APP_NAME_SHORT, L"DangerClr", COLOR_RED);
		}
		else if(dwData >= cfg.uYellowLevel)
		{
			clrIndicator = ini.read(APP_NAME_SHORT, L"WarningClr", COLOR_YELLOW);;
		}
	}

	// Create Bitmap
    HDC hDC = GetDC(NULL);
    HDC hCompatibleDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
    HBITMAP hBitmapMask = CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
	ReleaseDC(NULL, hDC);

    HBITMAP hOldBitMap = (HBITMAP)SelectObject(hCompatibleDC, hBitmap);

	// Instead FillRect
	COLORREF clrOld = SetBkColor(hCompatibleDC, TrayChangeBackground && clrIndicator ? clrIndicator : ini.read(APP_NAME_SHORT, L"TrayBackgroundClr", COLOR_TRAY_BG));
	ExtTextOut(hCompatibleDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	SetBkColor(hCompatibleDC, clrOld);

	// Create Font
	HFONT hTrayFont = CreateFontIndirect(&cfg.lf);
	SelectObject(hCompatibleDC, hTrayFont);

	// Draw Text
	SetTextColor(hCompatibleDC, !TrayChangeBackground && clrIndicator ? clrIndicator : clrTextColor);
	SetBkMode(hCompatibleDC, TRANSPARENT);

	buffer.Format(L"%d\0", TrayShowFree ? 100 - dwData : dwData);
	DrawTextEx(hCompatibleDC, buffer.GetBuffer(), buffer.GetLength(), &rc,  DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP, NULL);

	// Draw Border
	if(ini.read(APP_NAME_SHORT, L"TrayShowBorder", 0))
	{
		HBRUSH hBrush = CreateSolidBrush(!TrayChangeBackground && clrIndicator ? clrIndicator : clrTextColor);
		HRGN hRgn = CreateRectRgnIndirect(&rc);
		FrameRgn(hCompatibleDC, hRgn, hBrush, 1, 1);

		DeleteObject(hBrush);
		DeleteObject(hRgn);
	}

	SelectObject(hDC, hOldBitMap);

	// Create Icon
	ICONINFO ii = {TRUE, 0, 0, hBitmapMask, hBitmap};
	HICON hIcon = CreateIconIndirect(&ii);

	// Free Resources
	DeleteObject(SelectObject(hCompatibleDC, hTrayFont));
	DeleteDC(hCompatibleDC);
	DeleteDC(hDC);
	DeleteObject(hBitmap);
	DeleteObject(hBitmapMask);

	return hIcon;
}

// Memory Reduction Wrapper
BOOL MemReduct(HWND hWnd, BOOL bSilent)
{
	CString buffer;
	INT iBuffer = IDYES;
	
	MEMORY_USAGE mu = {0};
	SYSTEM_MEMORY_LIST_COMMAND smlc;

	LRESULT lParam = 0;
	DWORD dwPercents[3] = {0};

	// If user has no rights
	if(!cfg.bAdminPrivilege)
	{
		if(cfg.bUnderUAC)
			ShowBalloonTip(NIIF_ERROR, APP_NAME, ls(cfg.hLocale, IDS_UAC_WARNING));

		return 0;
	}

	// Check settings
	if(!ini.read(APP_NAME_SHORT, L"CleanWorkingSet", 1) && !ini.read(APP_NAME_SHORT, L"CleanSystemWorkingSet", 1) && !ini.read(APP_NAME_SHORT, L"CleanModifiedPagelist", 0) && !ini.read(APP_NAME_SHORT, L"CleanStandbyPagelist", 0))
	{
		if(!bSilent)
			MessageBox(hWnd, ls(cfg.hLocale, IDS_REDUCT_SELECTREGION), APP_NAME, MB_OK | MB_ICONSTOP);

		return 0;
	}

	if(!bSilent && ini.read(APP_NAME_SHORT, L"AskBeforeCleaning", 1))
		iBuffer = MessageBox(hWnd, ls(cfg.hLocale, IDS_REDUCT_WARNING), APP_NAME, MB_YESNO | MB_ICONQUESTION);

	if(iBuffer != IDYES)
		return 0;

	// Show Difference: BEFORE
	if(hWnd)
	{
		GetMemoryUsage(&mu);

		dwPercents[0] = mu.dwPercentPhys;
		buffer.Format(L"%d%%\0", mu.dwPercentPhys);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 0, 1);

		dwPercents[1] = mu.dwPercentPageFile;
		buffer.Format(L"%d%%\0", mu.dwPercentPageFile);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 1, 1);

		dwPercents[2] = mu.dwPercentSystemWorkingSet; 
		buffer.Format(L"%d%%\0", mu.dwPercentSystemWorkingSet);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 2, 1);
	}

	// Working Set
	if(cfg.bSupportedOS && ini.read(APP_NAME_SHORT, L"CleanWorkingSet", 1))
	{
		smlc = MemoryEmptyWorkingSets;
		NtSetSystemInformation(SystemMemoryListInformation, &smlc, sizeof(smlc));
	}

	// System Working Set
	if(ini.read(APP_NAME_SHORT, L"CleanSystemWorkingSet", 1))
	{
		SYSTEM_CACHE_INFORMATION cache = {0};

		cache.MinimumWorkingSet = (ULONG)-1;
		cache.MaximumWorkingSet = (ULONG)-1;

		NtSetSystemInformation(SystemFileCacheInformation, &cache, sizeof(cache));
	}
						
	// Modified Pagelists
	if(cfg.bSupportedOS && ini.read(APP_NAME_SHORT, L"CleanModifiedPagelist", 0))
	{
		smlc = MemoryFlushModifiedList;
		NtSetSystemInformation(SystemMemoryListInformation, &smlc, sizeof(smlc));
	}

	// Standby Pagelists
	if(cfg.bSupportedOS && ini.read(APP_NAME_SHORT, L"CleanStandbyPagelist", 0))
	{
		// Standby List
		smlc = MemoryPurgeStandbyList;
		NtSetSystemInformation(SystemMemoryListInformation, &smlc, sizeof(smlc));

		// Standby Priority-0 List
		smlc = MemoryPurgeLowPriorityStandbyList;
		NtSetSystemInformation(SystemMemoryListInformation, &smlc, sizeof(smlc));
	}
		
	// Show Difference: AFTER
	if(hWnd)
	{
		GetMemoryUsage(&mu);

		lParam = MAKELPARAM(dwPercents[0], mu.dwPercentPhys);
		buffer.Format(L"%d%%\0", mu.dwPercentPhys);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 0, 2, -1, -1, lParam);
			
		lParam = MAKELPARAM(dwPercents[1], mu.dwPercentPageFile);
		buffer.Format(L"%d%%\0", mu.dwPercentPageFile);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 1, 2, -1, -1, lParam);

		lParam = MAKELPARAM(dwPercents[2], mu.dwPercentSystemWorkingSet);
		buffer.Format(L"%d%%\0", mu.dwPercentSystemWorkingSet);
		Lv_InsertItem(hWnd, IDC_RESULT, buffer, 2, 2, -1, -1, lParam);
	}
	
	return 1;
}

/*
INT_PTR CALLBACK CleanerDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rc = {0};

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			if(cfg.bUnderUAC)
			{
				MessageBox(hwndDlg, ls(cfg.hLocale, IDS_UAC_WARNING), APP_NAME, MB_OK | MB_ICONERROR);
				SendMessage(hwndDlg, WM_CLOSE, 0, 0);
			}

			// Centering By Parent
			CenterDialog(hwndDlg);

			// Use Bold Font for Label
			SendDlgItemMessage(hwndDlg, IDC_TITLE_1, WM_SETFONT, (WPARAM)cfg.hBold, 0);

			// Init Settings
			CheckDlgButton(hwndDlg, IDC_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanSystemWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_MODIFIED_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanModifiedPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_STANDBY_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanStandbyPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);

			// Configure Listview
			Lv_SetStyleEx(hwndDlg, IDC_RESULT, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER, TRUE);

			// Insert Columns
			GetClientRect(GetDlgItem(hwndDlg, IDC_RESULT), &rc);

			for(int i = 0; i < 3; i++)
				Lv_InsertColumn(hwndDlg, IDC_RESULT, L"", (rc.right / (i ? 4 : 2)), i, 0);

			// Insert Items (HEADER)
			for(int i = IDS_CLEANER_COL_1, j = 0; i < (IDS_CLEANER_COL_3 + 1); i++, j++)
				Lv_InsertItem(hwndDlg, IDC_RESULT, ls(cfg.hLocale, i), 0, j);

			// Insert Items (ITEMS)
			for(int i = IDS_MEM_PHYSICAL, j = 1; i < (IDS_MEM_SYSCACHE + 1); i++, j++)
			{
				Lv_InsertItem(hwndDlg, IDC_RESULT, ls(cfg.hLocale, i), j, 0);

				for(int k = 1; k < 3; k++)
					Lv_InsertItem(hwndDlg, IDC_RESULT, L"0%", j, k);
			}

			// Indicate NOT Supported Features
			if(!cfg.bSupportedOS)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_WORKING_SET_CHK), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_MODIFIED_PAGELIST_CHK), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STANDBY_PAGELIST_CHK), 0);
			}

			SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(0, BN_CLICKED), 0);

			break;
		}

		case WM_CLOSE:
		{
			// Save Settings
			ini.write(APP_NAME_SHORT, L"CleanWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanSystemWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanModifiedPagelist", (IsDlgButtonChecked(hwndDlg, IDC_MODIFIED_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanStandbyPagelist", (IsDlgButtonChecked(hwndDlg, IDC_STANDBY_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);

			EndDialog(hwndDlg, 0);

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
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

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
		{
			return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;

			switch(lpnmhdr->code)
			{
				case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					LONG dwResult = CDRF_DODEFAULT;

					switch(lplvcd->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:
						{
							dwResult = CDRF_NOTIFYITEMDRAW;
							break;
						}

						case CDDS_ITEMPREPAINT:
						{
							if(!lplvcd->nmcd.dwItemSpec)
							{
								SelectObject(lplvcd->nmcd.hdc, cfg.hBold);
								dwResult = CDRF_NEWFONT;
							}

							dwResult |= CDRF_NOTIFYSUBITEMDRAW;

							break;
						}

						case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
						{
							if(lplvcd->nmcd.dwItemSpec && lplvcd->iSubItem)
							{
								lplvcd->clrText = COLOR_TEXT;

								if(lplvcd->nmcd.lItemlParam)
								{
									if(lplvcd->iSubItem && (LOWORD(lplvcd->nmcd.lItemlParam) == HIWORD(lplvcd->nmcd.lItemlParam)))
										lplvcd->clrText = COLOR_TEXT;
									else if(lplvcd->iSubItem == 1)
										lplvcd->clrText = LOWORD(lplvcd->nmcd.lItemlParam) < HIWORD(lplvcd->nmcd.lItemlParam) ? COLOR_GREEN : COLOR_RED;
									else if(lplvcd->iSubItem == 2)
										lplvcd->clrText = LOWORD(lplvcd->nmcd.lItemlParam) > HIWORD(lplvcd->nmcd.lItemlParam) ? COLOR_GREEN : COLOR_RED;
								}

								dwResult = CDRF_NEWFONT;
							}

							break;
						}
					}

					SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, dwResult);
					return 1;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) != IDC_OK && LOWORD(wParam) != IDC_CANCEL)
			{
				BOOL bEnable = 0;

				for(int i = IDC_WORKING_SET_CHK; i < (IDC_STANDBY_PAGELIST_CHK + 1); i++)
				{
					if(IsDlgButtonChecked(hwndDlg, i) == BST_CHECKED && IsWindowEnabled(GetDlgItem(hwndDlg, i)))
					{
						bEnable = 1;
						break;
					}
				}

				EnableWindow(GetDlgItem(hwndDlg, IDC_OK), bEnable);
			}

			switch(LOWORD(wParam))
			{
				case IDC_OK:
				{
					// Save Settings
					ini.write(APP_NAME_SHORT, L"CleanWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanSystemWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanModifiedPagelist", (IsDlgButtonChecked(hwndDlg, IDC_MODIFIED_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanStandbyPagelist", (IsDlgButtonChecked(hwndDlg, IDC_STANDBY_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);

					// Start Reduction
					if(MemReduct(hwndDlg, 0))
						EnableWindow(GetDlgItem(hwndDlg, IDC_OK), 0);

					break;
				}

				case IDCANCEL: // process Esc key
				case IDC_CANCEL:
				{
					SendMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}
			}

			break;
		}
	}

	return 0;
}
*/

INT_PTR WINAPI PagesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CString buffer;
	INT iBuffer = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Set Position
			SetWindowPos(hwndDlg, 0, tab_pages.rc.left, tab_pages.rc.top, tab_pages.rc.right - tab_pages.rc.left, tab_pages.rc.bottom - tab_pages.rc.top, 0);

			// Activate Tab Texture
			EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

			// Indicate Labels
			for(int i = IDC_TITLE_1; GetDlgItem(hwndDlg, i); i++)
				SendDlgItemMessage(hwndDlg, i, WM_SETFONT, (WPARAM)cfg.hBold, 0);

			switch(SendDlgItemMessage(GetParent(hwndDlg), IDC_TAB, TCM_GETCURSEL, 0, 0))
			{
				// GENERAL
				case 0:
				{
					// General
					CheckDlgButton(hwndDlg, IDC_LOAD_ON_STARTUP_CHK, IsAutorunExists(APP_NAME) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_UPDATE_AT_STARTUP_CHK, ini.read(APP_NAME_SHORT, L"CheckUpdateAtStartup", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_ALWAYS_ON_TOP_CHK, ini.read(APP_NAME_SHORT, L"AlwaysOnTop", 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_SHOW_AS_KILOBYTE_CHK, ini.read(APP_NAME_SHORT, L"ShowAsKilobyte", 0) ? BST_CHECKED : BST_UNCHECKED);

					// Color Indication
					CheckDlgButton(hwndDlg, IDC_COLOR_INDICATION_TRAY_CHK, ini.read(APP_NAME_SHORT, L"ColorIndicationTray", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_COLOR_INDICATION_LISTVIEW_CHK, ini.read(APP_NAME_SHORT, L"ColorIndicationListview", 1) ? BST_CHECKED : BST_UNCHECKED);

					// Language
					SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_ADDSTRING, 0, (LPARAM)L"English");

					WIN32_FIND_DATA wfd = {0};
					buffer.Format(L"%s\\Languages\\*.dll", cfg.szCurrentDir);
					HANDLE hFind = FindFirstFile(buffer, &wfd);
					HINSTANCE hLanguage= NULL;

					if(hFind != INVALID_HANDLE_VALUE)
					{
						do {
							if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
							{
								PathRemoveExtension(wfd.cFileName);

								buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, wfd.cFileName);

								if((hLanguage = LoadLanguage(buffer, APP_VERSION)))
								{
									SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_ADDSTRING, 0, (LPARAM)wfd.cFileName);
									FreeLibrary(hLanguage);
								}
							}
						} while(FindNextFile(hFind, &wfd));

						FindClose(hFind);
					}

					if(SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_GETCOUNT, 0, 0) <= 1)
						EnableWindow(GetDlgItem(hwndDlg, IDC_LANGUAGE_CB), 0);

					if(SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_SELECTSTRING, 1, (LPARAM)ini.read(APP_NAME_SHORT, L"Language", MAX_PATH, 0).GetBuffer()) == CB_ERR)
						SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_SETCURSEL, 0, 0);

					SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDC_LANGUAGE_CB, CBN_SELENDOK), 0);

					break;
				}

				// MEMORY REDUCTION
				case 1:
				{
					// Cleaning region
					CheckDlgButton(hwndDlg, IDC_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanSystemWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_MODIFIED_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanModifiedPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_STANDBY_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanStandbyPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);

					// Cleaning
					CheckDlgButton(hwndDlg, IDC_ASK_BEFORE_CLEANING_CHK, ini.read(APP_NAME_SHORT, L"AskBeforeCleaning", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_AUTOREDUCT_CHK, ini.read(APP_NAME_SHORT, L"AutoReduct", 0) ? BST_CHECKED : BST_UNCHECKED);

					// Trackbar
					SendDlgItemMessage(hwndDlg, IDC_AUTOREDUCT_TB, TBM_SETTICFREQ, 5, 0);				 
					SendDlgItemMessage(hwndDlg, IDC_AUTOREDUCT_TB, TBM_SETRANGE, 1, MAKELPARAM(5, 100));
					SendDlgItemMessage(hwndDlg, IDC_AUTOREDUCT_TB, TBM_SETPOS, 1, ini.read(APP_NAME_SHORT, L"AutoReductPercents", 90));

					SendMessage(hwndDlg, WM_HSCROLL, MAKELPARAM(SB_THUMBPOSITION, ini.read(APP_NAME_SHORT, L"AutoReductPercents", 90)), 1);
					SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDC_AUTOREDUCT_CHK, 0), 0);

					// Indicate unsupported features
					if(!cfg.bSupportedOS)
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_WORKING_SET_CHK), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_MODIFIED_PAGELIST_CHK), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_STANDBY_PAGELIST_CHK), 0);
					}

					break;
				}
				
				// TRAY
				case 2:
				{
					UDACCEL uda_rr[1] = {{0, 100}};

					// Configure Up-Down #1
					SendDlgItemMessage(hwndDlg, IDC_REFRESHRATE, UDM_SETACCEL, 1, (LPARAM)&uda_rr);
					SendDlgItemMessage(hwndDlg, IDC_REFRESHRATE, UDM_SETRANGE32, 100, 60000);
					SendDlgItemMessage(hwndDlg, IDC_REFRESHRATE, UDM_SETPOS32, 0, ini.read(APP_NAME_SHORT, L"RefreshRate", 500));

					// Configure Up-Down #2
					SendDlgItemMessage(hwndDlg, IDC_WARNING_LEVEL, UDM_SETRANGE32, 1, 99);
					SendDlgItemMessage(hwndDlg, IDC_WARNING_LEVEL, UDM_SETPOS32, 0, ini.read(APP_NAME_SHORT, L"WarningLevel", 60));

					// Configure Up-Down #3
					SendDlgItemMessage(hwndDlg, IDC_DANGER_LEVEL, UDM_SETRANGE32, 1, 99);
					SendDlgItemMessage(hwndDlg, IDC_DANGER_LEVEL, UDM_SETPOS32, 0, ini.read(APP_NAME_SHORT, L"DangerLevel", 90));
					
					// Tray double-click
					for(int i = IDS_DOUBLECLICK_1; i < (IDS_DOUBLECLICK_4 + 1); i++)
						SendDlgItemMessage(hwndDlg, IDC_DOUBLECLICK_CB, CB_ADDSTRING, 0, (LPARAM)ls(cfg.hLocale, i).GetBuffer());

					if(SendDlgItemMessage(hwndDlg, IDC_DOUBLECLICK_CB, CB_SETCURSEL, ini.read(APP_NAME_SHORT, L"OnDoubleClick", 0), 0) == CB_ERR)
						SendDlgItemMessage(hwndDlg, IDC_DOUBLECLICK_CB, CB_SETCURSEL, 0, 0);

					// Tray displayed memory region
					for(int i = IDS_MEM_PHYSICAL; i < (IDS_MEM_SYSCACHE + 1); i++)
						SendDlgItemMessage(hwndDlg, IDC_TRAYMEMORYREGION_CB, CB_ADDSTRING, 0, (LPARAM)ls(cfg.hLocale, i).GetBuffer());

					if(SendDlgItemMessage(hwndDlg, IDC_TRAYMEMORYREGION_CB, CB_SETCURSEL, ini.read(APP_NAME_SHORT, L"TrayMemoryRegion", 0), 0) == CB_ERR)
						SendDlgItemMessage(hwndDlg, IDC_TRAYMEMORYREGION_CB, CB_SETCURSEL, 0, 0);

					// Show "Free"
					CheckDlgButton(hwndDlg, IDC_TRAYSHOWFREE_CHK, ini.read(APP_NAME_SHORT, L"TrayShowFree", 0) ? BST_CHECKED : BST_UNCHECKED);
					SetDlgItemTooltip(hwndDlg, IDC_TRAYSHOWFREE_CHK, ls(cfg.hLocale, IDS_TRAY_SHOW_FREE));

					break;
				}

				// APPEARANCE
				case 3:
				{
					// Icon
					CheckDlgButton(hwndDlg, IDC_TRAY_SHOW_BORDER_CHK, ini.read(APP_NAME_SHORT, L"TrayShowBorder", 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TRAY_CHANGE_BACKGROUND_CHK, ini.read(APP_NAME_SHORT, L"TrayChangeBackground", 1) ? BST_CHECKED : BST_UNCHECKED);

					SetDlgItemText(hwndDlg, IDC_TRAY_FONT_BTN, cfg.lf.lfFaceName);

					break;
				}

				// BALLOON
				case 4:
				{
					// General
					CheckDlgButton(hwndDlg, IDC_BALLOON_SHOW_CHK, ini.read(APP_NAME_SHORT, L"BalloonShow", 1) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton(hwndDlg, IDC_BALLOON_AUTOREDUCT_CHK, ini.read(APP_NAME_SHORT, L"BalloonAutoReduct", 1) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_BALLOON_WARNING_CHK, ini.read(APP_NAME_SHORT, L"BalloonWarningLevel", 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_BALLOON_DANGER_CHK, ini.read(APP_NAME_SHORT, L"BalloonDangerLevel", 1) ? BST_CHECKED : BST_UNCHECKED);

					// Options
					SendDlgItemMessage(hwndDlg, IDC_BALLOONINTERVAL, UDM_SETRANGE32, 5, 1000);
					SendDlgItemMessage(hwndDlg, IDC_BALLOONINTERVAL, UDM_SETPOS32, 0, ini.read(APP_NAME_SHORT, L"BalloonInterval", 10));

					SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDC_BALLOON_SHOW_CHK, 0), 0);

					break;
				}

				// STATISTIC
				case 5:
				{
					// Styling
					Lv_SetStyleEx(hwndDlg, IDC_STATISTIC, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER, TRUE, TRUE);

					// Columns
					Lv_InsertColumn(hwndDlg, IDC_STATISTIC, L"", 214, 0, 0);
					Lv_InsertColumn(hwndDlg, IDC_STATISTIC, L"", 214, 1, 0);

					Lv_InsertGroup(hwndDlg, IDC_STATISTIC, L"Счётчик", 0);
					Lv_InsertGroup(hwndDlg, IDC_STATISTIC, L"Результаты", 1);

					Lv_InsertItem(hwndDlg, IDC_STATISTIC, L"Всего очисток", 0, 0, -1, 0);
					Lv_InsertItem(hwndDlg, IDC_STATISTIC, L"Последняя очистка", 1, 0, -1, 0);

					Lv_InsertItem(hwndDlg, IDC_STATISTIC, L"Всего очищено", 2, 0, -1, 1);
					Lv_InsertItem(hwndDlg, IDC_STATISTIC, L"Последний результат", 3, 0, -1, 1);

					break;
				}
			}

			break;
		}

		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK nmlink = (PNMLINK)lParam;

					if(nmlink->item.szUrl[0])
					{
						ShellExecute(hwndDlg, 0, nmlink->item.szUrl, 0, 0, SW_SHOW);
					}
					else if(nmlink->item.szID[0])
					{
						CHOOSECOLOR cc = {0};
						COLORREF cr[16] = {COLOR_TRAY_TEXT, COLOR_TRAY_BG, COLOR_TEXT, COLOR_GREEN, COLOR_YELLOW, COLOR_RED};

						cc.lStructSize = sizeof(cc);
						cc.Flags = CC_RGBINIT | CC_FULLOPEN;
						cc.hwndOwner = hwndDlg;
						cc.lpCustColors = cr;
						cc.rgbResult = ini.read(APP_NAME_SHORT, nmlink->item.szID, 0);

						if(ChooseColor(&cc))
							ini.write(APP_NAME_SHORT, nmlink->item.szID, cc.rgbResult);
					}

					break;
				}

				case UDN_DELTAPOS:
				{
					EnableWindow(GetDlgItem(GetParent(hwndDlg), IDC_APPLY), 1);
					break;
				}
			}

			break;
		}

		case WM_HSCROLL:
		case WM_VSCROLL:
		{
			if(GetDlgItem(hwndDlg, IDC_AUTOREDUCT_TB) && LOWORD(wParam) == SB_THUMBPOSITION)
			{
				buffer.Format(L"%d%%", HIWORD(wParam));
				SetDlgItemText(hwndDlg, IDC_AUTOREDUCT_PERCENT, buffer);

				if(!lParam)
					EnableWindow(GetDlgItem(GetParent(hwndDlg), IDC_APPLY), 1);
			}

			break;
		}

		case WM_COMMAND:
		{
			if(lParam && (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELENDOK))
				EnableWindow(GetDlgItem(GetParent(hwndDlg), IDC_APPLY), 1);

			if(HIWORD(wParam) == CBN_SELENDOK && LOWORD(wParam) == IDC_LANGUAGE_CB)
			{
				HINSTANCE hModule = NULL;
				WCHAR szBuffer[MAX_PATH] = {0};

				iBuffer = (SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_GETCURSEL, 0, 0) > 0);

				if(iBuffer)
				{
					GetDlgItemText(hwndDlg, IDC_LANGUAGE_CB, szBuffer, MAX_PATH);
					buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, szBuffer);

					hModule = LoadLanguage(buffer);
				}

				SetDlgItemText(hwndDlg, IDC_LANGUAGE_INFO, (iBuffer && !hModule) ? L"unknown" : ls(hModule, IDS_TRANSLATION_INFO));

				if(hModule)
					FreeLibrary(hModule);
			}

			switch(LOWORD(wParam))
			{
				case IDC_TRAY_FONT_BTN:
				{
					CHOOSEFONT cf = {0};

					cf.lStructSize = sizeof(cf);
					cf.hwndOwner = hwndDlg;
					cf.Flags = CF_FORCEFONTEXIST | CF_NOSCRIPTSEL | CF_INITTOLOGFONTSTRUCT;
					cf.lpLogFont = &cfg.lf;

					if(ChooseFont(&cf))
					{
						SetDlgItemText(hwndDlg, IDC_TRAY_FONT_BTN, cfg.lf.lfFaceName);

						ini.write(APP_NAME_SHORT, L"FontFace", cfg.lf.lfFaceName);
						ini.write(APP_NAME_SHORT, L"FontHeight", cfg.lf.lfHeight);
					}

					break;
				}

				case IDC_AUTOREDUCT_CHK:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOREDUCT_TB), (IsDlgButtonChecked(hwndDlg, IDC_AUTOREDUCT_CHK) == BST_CHECKED));
					break;
				}

				case IDC_BALLOON_SHOW_CHK:
				{
					iBuffer = (IsDlgButtonChecked(hwndDlg, IDC_BALLOON_SHOW_CHK) == BST_CHECKED);

					EnableWindow(GetDlgItem(hwndDlg, IDC_BALLOON_AUTOREDUCT_CHK), iBuffer);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BALLOON_WARNING_CHK), iBuffer);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BALLOON_DANGER_CHK), iBuffer);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BALLOONINTERVAL), iBuffer);
					EnableWindow((HWND)SendDlgItemMessage(hwndDlg, IDC_BALLOONINTERVAL, UDM_GETBUDDY, 0, 0), iBuffer);

					break;
				}
			}

			break;
		}
	}

	return 0;
}

INT_PTR CALLBACK SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CString buffer;
	INT iBuffer = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Centering By Parent
			CenterDialog(hwndDlg);

			// Insert Tab Pages
			TCITEM tci = {0};

			for(int i = 0; i < PAGE_COUNT; i++)
			{
				buffer = ls(cfg.hLocale, tab_pages.iTitle[i]);
				
				tci.mask = TCIF_TEXT;
				tci.pszText = buffer.GetBuffer();
				tci.cchTextMax = buffer.GetLength();

				SendDlgItemMessage(hwndDlg, IDC_TAB, TCM_INSERTITEM, i, (LPARAM)&tci);
			}

			// Calculate Tab Rect
			if(IsRectEmpty(&tab_pages.rc))
			{
				RECT rc = {0};
				GetWindowRect(GetDlgItem(hwndDlg, IDC_TAB), &rc);
				MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);

				GetClientRect(GetDlgItem(hwndDlg, IDC_TAB), &tab_pages.rc);
				TabCtrl_AdjustRect(GetDlgItem(hwndDlg, IDC_TAB), 0, &tab_pages.rc);
			
				tab_pages.rc.top += rc.top + 1;	tab_pages.rc.left += rc.left - 1; tab_pages.rc.right += rc.left - 1; tab_pages.rc.bottom += rc.top - 1;
			}

			// Reset Pages Handles
			for(int i = 0; i < PAGE_COUNT; i++)
				tab_pages.hWnd[i] = NULL;

			// Set Last Used Tab to Current
			SendDlgItemMessage(hwndDlg, IDC_TAB, TCM_SETCURSEL, ini.read(APP_NAME_SHORT, L"LastTab", 0), 0);
			
			// Initialize Page
			NMHDR hdr = {0};
			hdr.code = TCN_SELCHANGE;
			hdr.idFrom = IDC_TAB;

			SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM)&hdr);

			// Disable "Apply" Button
			EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), 0);

			break;
		}

		case WM_CLOSE:
		{
			ini.write(APP_NAME_SHORT, L"LastTab", SendDlgItemMessage(hwndDlg, IDC_TAB, TCM_GETCURSEL, 0, 0));
			EndDialog(hwndDlg, 0);

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;

			switch(lpnmhdr->code)
			{
				case TCN_SELCHANGE:
				{
					if(lpnmhdr->idFrom == IDC_TAB)
					{
						iBuffer = SendDlgItemMessage(hwndDlg, lpnmhdr->idFrom, TCM_GETCURSEL, 0, 0);
						ShowWindow(tab_pages.hCurrent, SW_HIDE);

						if(tab_pages.hWnd[iBuffer])
						{
							// Show Window
							ShowWindow(tab_pages.hWnd[iBuffer], SW_SHOW);
							tab_pages.hCurrent = tab_pages.hWnd[iBuffer];
						}
						else
						{
							// Create Window
							tab_pages.hCurrent = CreateDialog(cfg.hLocale, MAKEINTRESOURCE(tab_pages.iDialog[iBuffer]), hwndDlg, PagesDlgProc);
							tab_pages.hWnd[iBuffer] = tab_pages.hCurrent;
						}
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_OK:
				case IDC_APPLY:
				{
					// GENERAL
					if(tab_pages.hWnd[0])
					{
						// General
						CreateAutorunEntry(APP_NAME, (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_LOAD_ON_STARTUP_CHK) == BST_CHECKED));

						ini.write(APP_NAME_SHORT, L"CheckUpdateAtStartup", (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_CHECK_UPDATE_AT_STARTUP_CHK) == BST_CHECKED) ? 1 : 0);

						iBuffer = (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_ALWAYS_ON_TOP_CHK) == BST_CHECKED) ? 1 : 0;
						ini.write(APP_NAME_SHORT, L"AlwaysOnTop", iBuffer);
						SetWindowPos(cfg.hWnd, (iBuffer ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

						// Color Indication
						cfg.bColorIndicationTray = (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_COLOR_INDICATION_TRAY_CHK) == BST_CHECKED);
						ini.write(APP_NAME_SHORT, L"ColorIndicationTray", cfg.bColorIndicationTray);

						cfg.bColorIndicationListView = (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_COLOR_INDICATION_LISTVIEW_CHK) == BST_CHECKED);
						ini.write(APP_NAME_SHORT, L"ColorIndicationListview", cfg.bColorIndicationListView);

						// Language
						iBuffer = SendDlgItemMessage(tab_pages.hWnd[0], IDC_LANGUAGE_CB, CB_GETCURSEL, 0, 0);
						
						if(cfg.hLocale)
							FreeLibrary(cfg.hLocale);

						if(iBuffer <= 0)
						{
							ini.write(APP_NAME_SHORT, L"Language", (DWORD)0);
							cfg.hLocale = 0;
						}
						else
						{
							WCHAR szBuffer[MAX_PATH] = {0};

							GetDlgItemText(tab_pages.hWnd[0], IDC_LANGUAGE_CB, szBuffer, MAX_PATH);
							ini.write(APP_NAME_SHORT, L"Language", szBuffer);

							buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, szBuffer);
							cfg.hLocale = LoadLanguage(buffer);
						}

						// Size Unit
						iBuffer = (IsDlgButtonChecked(tab_pages.hWnd[0], IDC_SHOW_AS_KILOBYTE_CHK) == BST_CHECKED);
						ini.write(APP_NAME_SHORT, L"ShowAsKilobyte", iBuffer);
						StringCchCopy(cfg.szUnit, _countof(cfg.szUnit), ls(cfg.hLocale, iBuffer ? IDS_UNIT_KB : IDS_UNIT_MB));
						cfg.uUnitDivider = iBuffer ? 1024 : 1048576;
					}

					// MEMORY REDUCTION
					if(tab_pages.hWnd[1])
					{
						// Reduction region
						ini.write(APP_NAME_SHORT, L"CleanWorkingSet", (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
						ini.write(APP_NAME_SHORT, L"CleanSystemWorkingSet", (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_SYSTEM_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
						ini.write(APP_NAME_SHORT, L"CleanModifiedPagelist", (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_MODIFIED_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);
						ini.write(APP_NAME_SHORT, L"CleanStandbyPagelist", (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_STANDBY_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);

						// Reduction options
						ini.write(APP_NAME_SHORT, L"AskBeforeCleaning", (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_ASK_BEFORE_CLEANING_CHK) == BST_CHECKED) ? 1 : 0);

						cfg.bAutoReduct = (IsDlgButtonChecked(tab_pages.hWnd[1], IDC_AUTOREDUCT_CHK) == BST_CHECKED) ? 1 : 0;
						ini.write(APP_NAME_SHORT, L"AutoReduct", cfg.bAutoReduct);

						cfg.uAutoReductPercents = SendDlgItemMessage(tab_pages.hWnd[1], IDC_AUTOREDUCT_TB, TBM_GETPOS, 0, 0);
						ini.write(APP_NAME_SHORT, L"AutoReductPercents", cfg.uAutoReductPercents);
					}
					
					// TRAY
					if(tab_pages.hWnd[2])
					{
						// Refresh Rate
						iBuffer = SendDlgItemMessage(tab_pages.hWnd[2], IDC_REFRESHRATE, UDM_GETPOS32, 0, 0);

						ini.write(APP_NAME_SHORT, L"RefreshRate", iBuffer);
						KillTimer(GetParent(hwndDlg), UID);
						SetTimer(GetParent(hwndDlg), UID, iBuffer, 0);
						
						// "Warning" Level
						cfg.uYellowLevel = SendDlgItemMessage(tab_pages.hWnd[2], IDC_WARNING_LEVEL, UDM_GETPOS32, 0, 0);
						ini.write(APP_NAME_SHORT, L"WarningLevel", cfg.uYellowLevel);

						// "Danger" Level
						cfg.uRedLevel = SendDlgItemMessage(tab_pages.hWnd[2], IDC_DANGER_LEVEL, UDM_GETPOS32, 0, 0);
						ini.write(APP_NAME_SHORT, L"DangerLevel", cfg.uRedLevel);

						// Tray double-click
						ini.write(APP_NAME_SHORT, L"OnDoubleClick", SendDlgItemMessage(tab_pages.hWnd[2], IDC_DOUBLECLICK_CB, CB_GETCURSEL, 0, 0));

						// Tray displayed memory region
						ini.write(APP_NAME_SHORT, L"TrayMemoryRegion", SendDlgItemMessage(tab_pages.hWnd[2], IDC_TRAYMEMORYREGION_CB, CB_GETCURSEL, 0, 0));

						// Show "Free"
						ini.write(APP_NAME_SHORT, L"TrayShowFree", (IsDlgButtonChecked(tab_pages.hWnd[2], IDC_TRAYSHOWFREE_CHK) == BST_CHECKED) ? 1 : 0);
						
					}

					// APPEARANCE
					if(tab_pages.hWnd[3])
					{
						ini.write(APP_NAME_SHORT, L"TrayShowBorder", (IsDlgButtonChecked(tab_pages.hWnd[3], IDC_TRAY_SHOW_BORDER_CHK) == BST_CHECKED));
						ini.write(APP_NAME_SHORT, L"TrayChangeBackground", (IsDlgButtonChecked(tab_pages.hWnd[3], IDC_TRAY_CHANGE_BACKGROUND_CHK) == BST_CHECKED));
					}

					// BALLOON
					if(tab_pages.hWnd[4])
					{
						// Balloon's Chk
						cfg.bBalloonShow = (IsDlgButtonChecked(tab_pages.hWnd[4], IDC_BALLOON_SHOW_CHK) == BST_CHECKED);
						ini.write(APP_NAME_SHORT, L"BalloonShow", cfg.bBalloonShow);

						ini.write(APP_NAME_SHORT, L"BalloonAutoReduct", (IsDlgButtonChecked(tab_pages.hWnd[4], IDC_BALLOON_AUTOREDUCT_CHK) == BST_CHECKED));
						ini.write(APP_NAME_SHORT, L"BalloonWarningLevel", (IsDlgButtonChecked(tab_pages.hWnd[4], IDC_BALLOON_WARNING_CHK) == BST_CHECKED));
						ini.write(APP_NAME_SHORT, L"BalloonDangerLevel", (IsDlgButtonChecked(tab_pages.hWnd[4], IDC_BALLOON_DANGER_CHK) == BST_CHECKED));
	
						// Balloon Options
						ini.write(APP_NAME_SHORT, L"BalloonInterval", SendDlgItemMessage(tab_pages.hWnd[4], IDC_BALLOONINTERVAL, UDM_GETPOS32, 0, 0));
					}

					if(LOWORD(wParam) == IDC_APPLY)
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), 0);
						break;
					}

					// without break
				}

				case IDCANCEL: // process Esc key
				case IDC_CANCEL:
				{
					SendMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}
			}

			break;
		}
	}

	return 0;
}

VOID Switch(HWND hWnd)
{
	ShowWindow(GetDlgItem(hWnd, IDC_REGION), cfg.bSwitch ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_RESULT), cfg.bSwitch ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_REDUCT), cfg.bSwitch ? SW_SHOW : SW_HIDE);

	ShowWindow(GetDlgItem(hWnd, IDC_MONITOR), cfg.bSwitch ? SW_HIDE : SW_SHOW);

	SetDlgItemText(hWnd, IDC_SWITCH, cfg.bSwitch ? L"Memory Monitor" : L"Memory Reduction");

	cfg.bSwitch = !cfg.bSwitch;
}

LRESULT CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CString buffer;
	INT iBuffer = 0;
	RECT rc = {0};

	if(uMsg == WM_MUTEX)
		return WmMutexWrapper(hwndDlg, wParam, lParam);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Check Mutex
			CreateMutex(NULL, TRUE, APP_NAME_SHORT);

			if(wcsstr(GetCommandLine(), L"/restart"))
			{
				PostMessage(HWND_BROADCAST, WM_MUTEX, GetCurrentProcessId(), 0);
				ToggleVisible(hwndDlg, TRUE);
			}
			else if(GetLastError() == ERROR_ALREADY_EXISTS)
			{
				PostMessage(HWND_BROADCAST, WM_MUTEX, GetCurrentProcessId(), 1);
				SendMessage(hwndDlg, WM_CLOSE, 0, 0);

				return 0;
			}

			// Set Window Title
			SetWindowText(hwndDlg, APP_NAME L" " APP_VERSION L" / TEST - DO NOT USE");

			// Set Icons
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, 0));
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 16, 16, 0));

			// Modify System Menu
			HMENU hMenu = GetSystemMenu(hwndDlg, 0);
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_ABOUT, ls(cfg.hLocale, IDS_ABOUT));

			// Load Settings
			LOGFONT lf = {0};
			GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;

			cfg.hBold = CreateFontIndirect(&lf);
			cfg.hWnd = hwndDlg;

			cfg.bSupportedOS = ValidWindowsVersion(6, 0); // if vista (6.0) and later
			cfg.bAdminPrivilege = IsAdmin(); // if user has admin rights
			cfg.bUnderUAC = IsUnderUAC(); // if running under UAC

			cfg.bColorIndicationTray = ini.read(APP_NAME_SHORT, L"ColorIndicationTray", 1);
			cfg.bColorIndicationListView = ini.read(APP_NAME_SHORT, L"ColorIndicationListView", 1);
			cfg.uYellowLevel = ini.read(APP_NAME_SHORT, L"WarningLevel", 60);
			cfg.uRedLevel = ini.read(APP_NAME_SHORT, L"DangerLevel", 90);

			cfg.bAutoReduct = ini.read(APP_NAME_SHORT, L"AutoReduct", 0);
			cfg.uAutoReductPercents = ini.read(APP_NAME_SHORT, L"AutoReductPercents", 90);

			cfg.bBalloonShow = ini.read(APP_NAME_SHORT, L"BalloonShow", 1);

			iBuffer = ini.read(APP_NAME_SHORT, L"ShowAsKilobyte", 0);
			cfg.uUnitDivider = iBuffer ? 1024 : 1048576;
			StringCchCopy(cfg.szUnit, _countof(cfg.szUnit), ls(cfg.hLocale, iBuffer ? IDS_UNIT_KB : IDS_UNIT_MB));

			StringCchCopy(cfg.lf.lfFaceName, _countof(cfg.lf.lfFaceName), ini.read(APP_NAME_SHORT, L"FontFace", MAX_PATH, L"Tahoma"));
			cfg.lf.lfHeight = ini.read(APP_NAME_SHORT, L"FontHeight", -11);

			// Always on top
			SetWindowPos(hwndDlg, (ini.read(APP_NAME_SHORT, L"AlwaysOnTop", 0) ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			// Privileges enabler
			if(cfg.bAdminPrivilege)
			{
				HANDLE hToken = NULL;

				if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
				{
					SetPrivilege(hToken, SE_INCREASE_QUOTA_NAME, TRUE);
					SetPrivilege(hToken, SE_PROF_SINGLE_PROCESS_NAME, TRUE);
				}

				if(hToken)
					CloseHandle(hToken);
			}

			// Styling
			Lv_SetStyleEx(hwndDlg, IDC_MONITOR, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER, TRUE, TRUE);
			Lv_SetStyleEx(hwndDlg, IDC_REGION, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES, TRUE, TRUE);
			Lv_SetStyleEx(hwndDlg, IDC_RESULT, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER, TRUE, TRUE);

			// Columns (Monitor)
			Lv_InsertColumn(hwndDlg, IDC_MONITOR, L"", GetWindowDimension(GetDlgItem(hwndDlg, IDC_MONITOR), WIDTH, TRUE) / 2, 1, LVCFMT_RIGHT);
			Lv_InsertColumn(hwndDlg, IDC_MONITOR, L"", GetWindowDimension(GetDlgItem(hwndDlg, IDC_MONITOR), WIDTH, TRUE) / 2, 2, LVCFMT_LEFT);

			// Columns (Region)
			Lv_InsertColumn(hwndDlg, IDC_REGION, L"", GetWindowDimension(GetDlgItem(hwndDlg, IDC_REGION), WIDTH, TRUE), 0, LVCFMT_LEFT);

			// Columns (Reduction)
			for(int i = 0; i < 3; i++)
				Lv_InsertColumn(hwndDlg, IDC_RESULT, L"", i ? 80 : 161, i, i ? LVCFMT_RIGHT : LVCFMT_LEFT);

			// Groups (Monitor)
			for(int i = IDS_MEM_PHYSICAL, j = 0; i < (IDS_MEM_SYSCACHE + 1); i++, j++)
				Lv_InsertGroup(hwndDlg, IDC_MONITOR, ls(cfg.hLocale, i), j);
			
			// Groups (Region)
			Lv_InsertGroup(hwndDlg, IDC_REGION, ls(cfg.hLocale, IDS_REDUCT_REGION), 0);

			// Groups (Reduction)
			Lv_InsertGroup(hwndDlg, IDC_RESULT, ls(cfg.hLocale, IDS_REDUCT_RESULT), 0);

			// Items (Monitor)
			for(int i = 0, j = 0; i < 3; i++)
			{
				for(int k = IDS_MEM_USED; k < (IDS_MEM_TOTAL + 1); k++)
					Lv_InsertItem(hwndDlg, IDC_MONITOR, ls(cfg.hLocale, k), j++, 0, -1, i);
			}

			// Items (Region)
			for(int i = IDS_REDUCT_REGION_1, j = 0; i < (IDS_REDUCT_REGION_4 + 1); i++, j++)
				Lv_InsertItem(hwndDlg, IDC_REGION, ls(cfg.hLocale, i), j, 0, -1, 0);

			// Items (Reduction)
			for(int i = IDS_MEM_PHYSICAL, j = 0; i < (IDS_MEM_SYSCACHE + 1); i++, j++)
			{
				Lv_InsertItem(hwndDlg, IDC_RESULT, ls(cfg.hLocale, i), j, 0, -1, 0);

				for(int k = 0; k < 2; k++)
					Lv_InsertItem(hwndDlg, IDC_RESULT, L"0%", j, k + 1);
			}

			// Init settings
			CheckDlgButton(hwndDlg, IDC_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK, ini.read(APP_NAME_SHORT, L"CleanSystemWorkingSet", 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_MODIFIED_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanModifiedPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_STANDBY_PAGELIST_CHK, ini.read(APP_NAME_SHORT, L"CleanStandbyPagelist", 0) ? BST_CHECKED : BST_UNCHECKED);

			// Indicate NOT Supported Features
			if(!cfg.bSupportedOS)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_WORKING_SET_CHK), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_MODIFIED_PAGELIST_CHK), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STANDBY_PAGELIST_CHK), 0);
			}

			// Privilege Indicator
			if(cfg.bUnderUAC)
				SendDlgItemMessage(hwndDlg, IDC_SWITCH, BCM_SETSHIELD, 0, TRUE); // Windows Vista (and above)

			if(!cfg.bAdminPrivilege && !cfg.bSupportedOS)
				EnableWindow(GetDlgItem(hwndDlg, IDC_REDUCT), FALSE); // Windows XP

			// Tray Icon
			MEMORY_USAGE mu = {0};

			nid.cbSize = cfg.bSupportedOS ? sizeof(nid) : NOTIFYICONDATA_V3_SIZE;
			nid.hWnd = hwndDlg;
			nid.uID = UID;
			nid.uFlags = NIF_MESSAGE | NIF_ICON;
			nid.uCallbackMessage = WM_TRAYICON;
			nid.hIcon = CreateMemIcon(GetMemoryUsage(&mu));

			Shell_NotifyIcon(NIM_ADD, &nid);

			// Timers
			SetTimer(hwndDlg, UID, ini.read(APP_NAME_SHORT, L"RefreshRate", 500), NULL);
			SetTimer(hwndDlg, UID + 1, 500, NULL);

			Switch(hwndDlg);

			// Check Updates
			if(ini.read(APP_NAME_SHORT, L"CheckUpdateAtStartup", 1))
				_beginthreadex(NULL, 0, &CheckUpdates, (LPVOID)1, 0, NULL);

			break;
		}

		case WM_CLOSE:
		{
			// Save settings
			ini.write(APP_NAME_SHORT, L"CleanWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanSystemWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanModifiedPagelist", (IsDlgButtonChecked(hwndDlg, IDC_MODIFIED_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);
			ini.write(APP_NAME_SHORT, L"CleanStandbyPagelist", (IsDlgButtonChecked(hwndDlg, IDC_STANDBY_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);

			// Destroy Timers
			KillTimer(hwndDlg, UID);
			KillTimer(hwndDlg, UID + 1);

			// Destroy Resources
			if(cfg.hBold)
				DeleteObject(cfg.hBold);

			if(nid.hIcon)
				DestroyIcon(nid.hIcon);

			if(cfg.hLocale)
				FreeLibrary(cfg.hLocale);

			// Destroy Tray
			if(nid.uID)
				Shell_NotifyIcon(NIM_DELETE, &nid);

			DestroyWindow(hwndDlg);
			PostQuitMessage(0);

			break;
		}

		case WM_DRAWITEM:
		{
			COLORREF clrBg = COLOR_TRAY_BG, cltText = COLOR_TRAY_TEXT;

			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

			//if(lpdis->CtlID == ODT_BUTTON)
			//{
				if(lpdis->itemState & ODS_DISABLED)
				{
					clrBg = RGB(234, 234, 234);
					cltText = RGB(190, 190, 190);
				}
				else if(lpdis->itemState & ODS_SELECTED)
				{
					clrBg = RGB(204, 204, 204);
				}

				COLORREF clrOld = SetBkColor(lpdis->hDC, clrBg);
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);
				SetBkColor(lpdis->hDC, clrOld);


				SetBkMode(lpdis->hDC, TRANSPARENT);
				SetTextColor(lpdis->hDC, cltText);
				GetWindowText(lpdis->hwndItem, buffer.GetBuffer(MAX_PATH), MAX_PATH);
				buffer.ReleaseBuffer();

				DrawTextEx(lpdis->hDC, buffer.GetBuffer(), buffer.GetLength(), &lpdis->rcItem,  DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP, NULL);

				return TRUE;
			//}

			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
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
		
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
		{
			return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;

			switch(lpnmhdr->code)
			{
				case LVN_ITEMCHANGED:
				{
					LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;

					if(lpnmlv->hdr.idFrom == IDC_REGION)
					{
						if(!lpnmlv->uNewState && !lpnmlv->uOldState)
							break;

						// Old checkbox state
						BOOL bPrevState = ((lpnmlv->uOldState & LVIS_STATEIMAGEMASK) >> 12) - 1;

						if(bPrevState < 0) 
							bPrevState = 0;

						// New checkbox state
						BOOL bChecked = ((lpnmlv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1;
						
						if(bChecked < 0)
							bChecked = 0; 

						// No changes in checkbox
						if(bPrevState == bChecked)
							break;

						MessageBox(0,MB_TOPMOST, 0, L"%d %d", lpnmlv->uNewState, lpnmlv->iItem);
					}

					break;
				}

				case NM_CUSTOMDRAW:
				{
					LONG dwResult = CDRF_DODEFAULT;
					LPNMLVCUSTOMDRAW lpnmlv = (LPNMLVCUSTOMDRAW)lParam;

					switch(lpnmlv->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:
						{
							dwResult = CDRF_NOTIFYITEMDRAW;
							break;
						}

						case CDDS_ITEMPREPAINT:
						{
							dwResult = CDRF_NOTIFYSUBITEMDRAW;
							break;
						}

						case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
						{
							if(lpnmlv->nmcd.hdr.idFrom == IDC_MONITOR && lpnmlv->iSubItem == 1)
							{
								if(cfg.bColorIndicationListView && (UINT)lpnmlv->nmcd.lItemlParam >= cfg.uRedLevel)
									lpnmlv->clrText = ini.read(APP_NAME_SHORT, L"DangerClr", COLOR_RED);

								else if(cfg.bColorIndicationListView && (UINT)lpnmlv->nmcd.lItemlParam >= cfg.uYellowLevel)
									lpnmlv->clrText = ini.read(APP_NAME_SHORT, L"WarningClr", COLOR_YELLOW);

								else
									lpnmlv->clrText = ini.read(APP_NAME_SHORT, L"ListViewTextClr", COLOR_TEXT);

								dwResult = CDRF_NEWFONT;
							}
							else if(lpnmlv->nmcd.hdr.idFrom == IDC_RESULT && lpnmlv->iSubItem > 0)
							{
								lpnmlv->clrText = ini.read(APP_NAME_SHORT, L"ListViewTextClr", COLOR_TEXT);

								if(lpnmlv->nmcd.lItemlParam)
								{
									if(lpnmlv->iSubItem && (LOWORD(lpnmlv->nmcd.lItemlParam) != HIWORD(lpnmlv->nmcd.lItemlParam)))
									{
										if(lpnmlv->iSubItem == 1)
											lpnmlv->clrText = LOWORD(lpnmlv->nmcd.lItemlParam) < HIWORD(lpnmlv->nmcd.lItemlParam) ? COLOR_GREEN : ini.read(APP_NAME_SHORT, L"DangerClr", COLOR_RED);

										else if(lpnmlv->iSubItem == 2)
											lpnmlv->clrText = LOWORD(lpnmlv->nmcd.lItemlParam) > HIWORD(lpnmlv->nmcd.lItemlParam) ? COLOR_GREEN : ini.read(APP_NAME_SHORT, L"DangerClr", COLOR_RED);
									}
								}




								dwResult = CDRF_NEWFONT;
							}

							break;
						}
					}

					SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, dwResult);
					return 1;
				}
			}

			break;
		}

		case WM_SIZE:
		{
			if(wParam == SIZE_MINIMIZED)
				ToggleVisible(hwndDlg);

			break;
		}

		case WM_SYSCOMMAND:
		{
			if(wParam == IDM_ABOUT)
			{
				SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDM_ABOUT, 0), 0);
			}
			else if(wParam == SC_CLOSE)
			{
				ToggleVisible(hwndDlg);
				return 1;
			}

			break;
		}

		case WM_TIMER:
		{
			MEMORY_USAGE mu = {0};
			iBuffer = GetMemoryUsage(&mu);

			switch(wParam)
			{		
				case UID:
				{
					// Destroy Previous Icon
					if(nid.hIcon)
						DestroyIcon(nid.hIcon);

					// Refresh Tray Info
					nid.uFlags = NIF_ICON | NIF_TIP;
					StringCchPrintf(nid.szTip, _countof(nid.szTip), ls(cfg.hLocale, IDS_TRAY_TOOLTIP), number_format(mu.ullFilledPhys, cfg.szUnit), number_format(mu.ullTotalPhys, cfg.szUnit), number_format(mu.ullFilledPageFile, cfg.szUnit), number_format(mu.ullTotalPageFile, cfg.szUnit), number_format(mu.ullFilledSystemWorkingSet, cfg.szUnit), number_format(mu.ullTotalSystemWorkingSet, cfg.szUnit));
					nid.hIcon = CreateMemIcon(iBuffer);
				
					Shell_NotifyIcon(NIM_MODIFY, &nid);

					break;
				}

				case UID + 1:
				{
					// Auto-Reduct Option
					if(cfg.bAdminPrivilege && cfg.bAutoReduct && mu.dwPercentPhys >= cfg.uAutoReductPercents)
					{
						MemReduct(0, 1);

						if(cfg.bBalloonShow && ini.read(APP_NAME_SHORT, L"BalloonAutoReduct", 1))
						{
							GetMemoryUsage(&mu);

							buffer.Format(ls(cfg.hLocale, IDS_BALLOON_AUTOREDUCT), cfg.uAutoReductPercents, mu.dwPercentPhys);
							ShowBalloonTip(NIIF_INFO, APP_NAME, buffer);
						}
					}

					// Show Balloon Tips
					if(cfg.bBalloonShow)
					{
						if(mu.dwPercentPhys >= cfg.uRedLevel && ini.read(APP_NAME_SHORT, L"BalloonDangerLevel", 1))
							ShowBalloonTip(NIIF_ERROR, APP_NAME, ls(cfg.hLocale, IDS_BALLOON_REDLEVEL));
						else if(mu.dwPercentPhys >= cfg.uYellowLevel && ini.read(APP_NAME_SHORT, L"BalloonWarningLevel", 0))
							ShowBalloonTip(NIIF_WARNING, APP_NAME, ls(cfg.hLocale, IDS_BALLOON_YELLOWLEVEL));
					}

					if(IsWindowVisible(hwndDlg))
					{
						// Physical Memory
						buffer.Format(L"%d%%", mu.dwPercentPhys);

						Lv_InsertItem(hwndDlg, IDC_MONITOR, buffer, 0, 1, -1, -1, mu.dwPercentPhys);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullFilledPhys, cfg.szUnit), 1, 1, -1, -1, mu.dwPercentPhys);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullTotalPhys, cfg.szUnit), 2, 1, -1, -1, mu.dwPercentPhys);

						// Page File
						buffer.Format(L"%d%%", mu.dwPercentPageFile);

						Lv_InsertItem(hwndDlg, IDC_MONITOR, buffer, 3, 1, -1, -1, mu.dwPercentPageFile);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullFilledPageFile, cfg.szUnit), 4, 1, -1, -1, mu.dwPercentPageFile);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullTotalPageFile, cfg.szUnit), 5, 1, -1, -1, mu.dwPercentPageFile);

						// System Cache
						buffer.Format(L"%d%%", mu.dwPercentSystemWorkingSet);

						Lv_InsertItem(hwndDlg, IDC_MONITOR, buffer, 6, 1, -1, -1, mu.dwPercentSystemWorkingSet);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullFilledSystemWorkingSet, cfg.szUnit), 7, 1, -1, -1, mu.dwPercentSystemWorkingSet);
						Lv_InsertItem(hwndDlg, IDC_MONITOR, number_format(mu.ullTotalSystemWorkingSet, cfg.szUnit), 8, 1, -1, -1, mu.dwPercentSystemWorkingSet);
					}

					break;
				}
			}

			break;
		}

		case WM_TRAYICON:
		{
			switch(LOWORD(lParam))
			{
				case WM_LBUTTONDBLCLK:
				{
					switch(ini.read(APP_NAME_SHORT, L"OnDoubleClick", 0))
					{
						case 1:
						{
							ShellExecute(hwndDlg, 0, L"taskmgr.exe", NULL, NULL, 0);
							break;
						}
							
						case 2:
						{
							SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDM_TRAY_REDUCT, 0), 0);
							break;
						}

						case 3:
						{
							MemReduct(hwndDlg, 0);
							break;
						}

						default:
						{
							SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDM_TRAY_SHOW, 0), 0);
							break;
						}
					}

					break;
				}

				case WM_RBUTTONUP:
				case WM_CONTEXTMENU:
				{
					// Load Menu
					HMENU hMenu = LoadMenu(cfg.hLocale, MAKEINTRESOURCE(IDM_TRAY)), hSubMenu = GetSubMenu(hMenu, 0);

					// Set Default Menu Item
					if(ini.read(APP_NAME_SHORT, L"OnDoubleClick", 0) == 0)
						SetMenuDefaultItem(hSubMenu, IDM_TRAY_SHOW, 0);

					// Indicate (SHOW/HIDE) Action
					if(IsWindowVisible(hwndDlg))
					{
						MENUITEMINFO mii = {0};

						mii.cbSize = sizeof(mii);
						mii.fMask = MIIM_STRING;

						buffer = ls(cfg.hLocale, IDS_TRAY_HIDE);
						mii.dwTypeData = buffer.GetBuffer();
						mii.cch = buffer.GetLength();

						SetMenuItemInfo(hSubMenu, IDM_TRAY_SHOW, FALSE, &mii);
					}

					if(cfg.bUnderUAC)
						SetMenuItemShield(hSubMenu, IDM_TRAY_REDUCT, FALSE);

					// Prevent Re-opening Dialogs
					if(cfg.bReductDlg)
						EnableMenuItem(hSubMenu, IDM_TRAY_REDUCT, MF_BYCOMMAND | MF_DISABLED);
					
					if(cfg.bSettingsDlg)
						EnableMenuItem(hSubMenu, IDM_TRAY_SETTINGS, MF_BYCOMMAND | MF_DISABLED);

					// Switch Window to Foreground
					SetForegroundWindow(hwndDlg);

					// Get Cursor Position
					POINT pt = {0};
					GetCursorPos(&pt);

					// Show Menu
					TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON | TPM_NOANIMATION, pt.x, pt.y, hwndDlg, NULL);

					// Destroy Menu
					DestroyMenu(hMenu);
					DestroyMenu(hSubMenu);

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) != IDC_SWITCH)
			{
				BOOL bEnable = 0;

				for(int i = IDC_WORKING_SET_CHK; i < (IDC_STANDBY_PAGELIST_CHK + 1); i++)
				{
					if(IsDlgButtonChecked(hwndDlg, i) == BST_CHECKED && IsWindowEnabled(GetDlgItem(hwndDlg, i)))
					{
						bEnable = 1;
						break;
					}
				}

				EnableWindow(GetDlgItem(hwndDlg, IDC_REDUCT), bEnable);
			}

			switch(LOWORD(wParam))
			{
				case IDM_TRAY_EXIT:
				case IDM_EXIT:
				{
					SendMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}

				case IDCANCEL: // process Esc key
				case IDM_TRAY_SHOW:
				{
					ToggleVisible(hwndDlg);
					break;
				}

				case IDM_TRAY_REDUCT:
				{
					cfg.bSwitch = 1;
					ToggleVisible(hwndDlg, TRUE);

					// without break
				}

				case IDC_SWITCH:
				{
					if(cfg.bUnderUAC)
					{
						GetModuleFileName(NULL, buffer.GetBuffer(MAX_PATH), MAX_PATH);
						buffer.ReleaseBuffer();

						if(RunElevated(hwndDlg, buffer, L"/restart"))
						{
							PostMessage(hwndDlg, WM_CLOSE, 0, 0);
							return 0;
						}
					}

					Switch(hwndDlg);
					break;
				}

				case IDC_REDUCT:
				{
					// Save settings
					ini.write(APP_NAME_SHORT, L"CleanWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanSystemWorkingSet", (IsDlgButtonChecked(hwndDlg, IDC_SYSTEM_WORKING_SET_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanModifiedPagelist", (IsDlgButtonChecked(hwndDlg, IDC_MODIFIED_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);
					ini.write(APP_NAME_SHORT, L"CleanStandbyPagelist", (IsDlgButtonChecked(hwndDlg, IDC_STANDBY_PAGELIST_CHK) == BST_CHECKED) ? 1 : 0);

					// Start reduction
					MemReduct(hwndDlg, 0);

					break;
				}
				
				case IDM_TRAY_SETTINGS:
				case IDM_SETTINGS:
				{
					if(!cfg.bSettingsDlg)
					{
						cfg.bSettingsDlg = 1;
						DialogBox(cfg.hLocale, MAKEINTRESOURCE(IDD_SETTINGS), hwndDlg, SettingsDlgProc);
						cfg.bSettingsDlg = 0;
					}

					break;
				}
				
				case IDM_TRAY_WEBSITE:
				case IDM_WEBSITE:
				{
					ShellExecute(hwndDlg, 0, APP_WEBSITE, NULL, NULL, 0);
					break;
				}

				case IDM_CHECK_UPDATES:
				{
					_beginthreadex(NULL, 0, &CheckUpdates, 0, 0, NULL);
					break;
				}

				case IDM_TRAY_ABOUT:
				case IDM_ABOUT:
				{
					buffer.Format(ls(cfg.hLocale, IDS_COPYRIGHT), APP_WEBSITE, APP_HOST);
					AboutBoxCreate(hwndDlg, MAKEINTRESOURCE(IDI_MAIN), ls(cfg.hLocale, IDS_ABOUT), APP_NAME L" " APP_VERSION, L"Copyright © 2013 Henry++\r\nAll Rights Reversed\r\n\r\n" + buffer);

					break;
				}
			}

			break;
		}
	}

	return 0;
}

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	CString buffer;

	MSG msg = {0};
	INITCOMMONCONTROLSEX icex = {0};

	// Set current dir
	GetModuleFileName(NULL, buffer.GetBuffer(MAX_PATH), MAX_PATH);
	buffer.ReleaseBuffer();

	PathRemoveFileSpec(buffer.GetBuffer(MAX_PATH));
	buffer.ReleaseBuffer();

	cfg.szCurrentDir = buffer;

	// Generate config path
	if(!FileExists((buffer = cfg.szCurrentDir + L"\\" APP_NAME_SHORT + L".cfg")))
	{
		ExpandEnvironmentStrings(L"%APPDATA%\\" APP_AUTHOR L"\\" APP_NAME, buffer.GetBuffer(MAX_PATH), MAX_PATH);
		buffer.ReleaseBuffer();

		SHCreateDirectoryEx(NULL, buffer, NULL);
		buffer.Append(+ L"\\" APP_NAME_SHORT L".cfg");
	}

	// Set config path
	ini.set_path(buffer);

	// Load language
	buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, ini.read(APP_NAME_SHORT, L"Language", MAX_PATH, 0));

	if(FileExists(buffer))
		cfg.hLocale = LoadLanguage(buffer, APP_VERSION);

	// Initialize and create window
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;

	if(!InitCommonControlsEx(&icex) || !CreateDialog(cfg.hLocale, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DlgProc))
		return 0;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsDialogMessage(cfg.hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	return msg.wParam;
}