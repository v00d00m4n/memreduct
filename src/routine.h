/************************************
*  	Routine Functions
*	Copyright © 2013 Henry++
*
*	GNU General Public License v2
*	http://www.gnu.org/licenses/
*
*	http://www.henrypp.org/
*************************************/

// lastmod: 11/02/13

#ifndef __ROUTINE_H__
#define __ROUTINE_H__

#include <windows.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <lm.h>
#include <atlstr.h>
#include <strsafe.h>

// Color Shader
#ifndef COLOR_SHADE
#define COLOR_SHADE(clr, percent) RGB((BYTE)(GetRValue(clr) * percent / 100), (BYTE)(GetGValue(clr) * percent / 100), (BYTE)(GetBValue(clr) * percent / 100))
#endif // COLOR_SHADE

#define WIDTH 1
#define HEIGHT 2

HTREEITEM Tv_InsertItem(HWND hWnd, INT iCtrlId, CString lpszText, HTREEITEM hParent = 0, INT iImage = -1, INT iSelImage = -1, LPARAM lParam = -1);
HRESULT Tv_SetStyleEx(HWND hWnd, INT iCtrlId, DWORD dwExStyle, BOOL bExplorerStyle, INT iItemHeight = 0);

DWORD Lv_SetStyleEx(HWND hWnd, INT iCtrlId, DWORD dwExStyle, BOOL bExplorerStyle = TRUE, BOOL bGroupView = FALSE);
CString Lv_GetItemText(HWND hWnd, INT iCtrlId, INT cchTextMax, INT iItem, INT iSubItem);
INT Lv_InsertGroup(HWND hWnd, INT iCtrlId, CString lpszText, INT iGroupId, UINT uAlign = 0, UINT uState = 0);
INT Lv_InsertColumn(HWND hWnd, INT iCtrlId, CString lpszText, INT iWidth, INT iItem, INT iFmt);
INT Lv_InsertItem(HWND hWnd, INT iCtrlId, CString lpszText, INT iItem, INT iSubItem, INT iImage = -1, INT iGroupId = -1, LPARAM lParam = 0);

BOOL ShowEditBalloonTip(HWND hWnd, INT iCtrlId, LPCTSTR lpcszTitle, LPCTSTR lpcszText, INT iIcon);
BOOL IsAdmin();
BOOL FileExists(LPCTSTR lpcszPath);
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpcszPrivilege, BOOL bEnablePrivilege);
BOOL ValidWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion);

CString number_format(LONGLONG lNumber, LPCWSTR lpszAppend = 0, CONST WCHAR cSeparator = L',');

VOID SetAlwaysOnTop(HWND hWnd, BOOL bEnable);
VOID CenterDialog(HWND hWnd);
VOID ToggleVisible(HWND hWnd);
VOID CreateAutorunEntry(LPCWSTR lpszName, BOOL bRemove);

CString ls(HINSTANCE hInstance, UINT uID);

CString ClipboardGet();
BOOL ClipboardPut(CString buffer);

VOID ImageList_Add(HIMAGELIST hImgList, LPWSTR lpszIco);

BOOL UnixTimeToSystemTime(time_t t, SYSTEMTIME* pst);
time_t SystemTimeToUnixTime(SYSTEMTIME* pst);

INT GetWindowDimension(HWND hWnd, INT iVector, BOOL bClientOnly);
INT MessageBox(HWND hWnd, UINT uType, LPCWSTR lpcszCaption, LPCWSTR lpcszFormat, ...);
HWND SetDlgItemTooltip(HWND hWnd, INT iCtrlId, LPWSTR lpszText);
INT WmMutexWrapper(HWND hwndDlg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK AboutBoxProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT AboutBoxCreate(HWND hParent, LPWSTR lpszIcon, LPCWSTR lpcszTitle, LPCWSTR lpszAppName, LPCWSTR lpcszCopyright);

#endif // __ROUTINE_H__
