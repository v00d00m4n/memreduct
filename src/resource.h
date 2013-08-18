#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Dialogs
#define IDD_MAIN	                            100
#define IDD_REDUCT	                            101
#define IDD_SETTINGS                            102
#define IDD_PAGE_1								103
#define IDD_PAGE_2								104
#define IDD_PAGE_3								105
#define IDD_PAGE_4								106
#define IDD_PAGE_5								107
#define IDD_PAGE_6								108

// Menu Id
#define IDM_MAIN		                        100
#define IDM_TRAY		                        101
#define IDM_REDUCT		                        102

// Main Dlg
#define IDC_MONITOR								100
#define IDC_REDUCT								101

// Memory Reduction Dlg
#define IDC_RESULT								100
#define IDC_SETTINGS							101

// Settings Dlg
#define IDC_TAB									100

// Page #1 Dlg
#define IDC_CHECK_UPDATE_AT_STARTUP_CHK			100
#define IDC_LOAD_ON_STARTUP_CHK					101
#define IDC_ALWAYS_ON_TOP_CHK					102
#define IDC_SHOW_AS_KILOBYTE_CHK				103
#define IDC_COLOR_INDICATION_TRAY_CHK			104
#define IDC_COLOR_INDICATION_LISTVIEW_CHK		105
#define IDC_LANGUAGE_CB							106
#define IDC_LANGUAGE_INFO						107

// Page #2 Dlg
#define IDC_WORKING_SET_CHK						108
#define IDC_SYSTEM_WORKING_SET_CHK				109
#define IDC_MODIFIED_PAGELIST_CHK				110
#define IDC_STANDBY_PAGELIST_CHK				111
												
#define IDC_ASK_BEFORE_CLEANING_CHK				112
#define IDC_AUTOREDUCT_CHK						113
#define IDC_AUTOREDUCT_TB						114
#define IDC_AUTOREDUCT_PERCENT					115

// Page #3 Dlg									
#define IDC_REFRESHRATE							116
#define IDC_WARNING_LEVEL						117
#define IDC_DANGER_LEVEL						118
#define IDC_DOUBLECLICK_CB						119
#define IDC_TRAYMEMORYREGION_CB					120
#define IDC_TRAYSHOWFREE_CHK					121

// Page #4 Dlg
#define IDC_TRAY_SHOW_BORDER_CHK				122
#define IDC_TRAY_CHANGE_BACKGROUND_CHK			123
#define IDC_TRAY_FONT_BTN						124
#define IDC_TRAY_TEXT_CLR_BTN					125
#define IDC_TRAY_BG_CLR_BTN						126
#define IDC_LISTVIEW_TEXT_CLR_BTN				127
#define IDC_LEVEL_NORMAL_CLR_BTN				128
#define IDC_LEVEL_WARNING_CLR_BTN				129
#define IDC_LEVEL_DANGER_CLR_BTN				130

// Page #5 Dlg
#define IDC_BALLOON_SHOW_CHK					131
#define IDC_BALLOON_AUTOREDUCT_CHK				132
#define IDC_BALLOON_WARNING_CHK					133
#define IDC_BALLOON_DANGER_CHK					134
#define IDC_BALLOONINTERVAL						135

// Common Controls
#define IDC_OK									1000
#define IDC_CANCEL								1001
#define IDC_APPLY								1002

#define IDC_TITLE_1								2000
#define IDC_TITLE_2								2001
#define IDC_TITLE_3								2002

// Main Menu
#define IDM_SETTINGS                            40000
#define IDM_EXIT                                40001
#define IDM_WEBSITE                             40002
#define IDM_CHECK_UPDATES                       40003
#define IDM_ABOUT                               40004
#define IDM_ONTOP                               40005

// Tray Menu
#define IDM_TRAY_SHOW							1000
#define IDM_TRAY_REDUCT							1001
#define IDM_TRAY_SETTINGS                       1002
#define IDM_TRAY_WEBSITE						1003
#define IDM_TRAY_ABOUT							1004
#define IDM_TRAY_EXIT							1005

// Strings
#define IDS_TRANSLATION_INFO                    1000

#define IDS_UPDATE_NO                           10000
#define IDS_UPDATE_YES                          10001
#define IDS_UPDATE_ERROR                        10002

#define IDS_ABOUT								10003
#define IDS_COPYRIGHT							10004

#define IDS_MEM_PHYSICAL						10005
#define IDS_MEM_PAGEFILE						10006
#define IDS_MEM_SYSCACHE						10007

#define IDS_MEM_USAGE							10008
#define IDS_MEM_FREE							10009
#define IDS_MEM_TOTAL							10010

#define IDS_TRAY_HIDE	                        10011
#define IDS_TRAY_TOOLTIP                        10012

#define IDS_BALLOON_AUTOREDUCT	                10013
#define IDS_BALLOON_YELLOWLEVEL                 10014
#define IDS_BALLOON_REDLEVEL                    10015

#define IDS_UNIT_KB		                        10016
#define IDS_UNIT_MB		                        10017
							
#define IDS_REDUCT_SELECTREGION					10021
#define IDS_REDUCT_WARNING			            10022

#define IDS_REDUCT_REGION_1			            10023
#define IDS_REDUCT_REGION_2			            10024
#define IDS_REDUCT_REGION_3			            10025
#define IDS_REDUCT_REGION_4			            10026
			
#define IDS_PAGE_1								10027
#define IDS_PAGE_2								10028
#define IDS_PAGE_3								10029
#define IDS_PAGE_4								10030
#define IDS_PAGE_5								10031

#define IDS_DOUBLECLICK_1						10033
#define IDS_DOUBLECLICK_2						10034
#define IDS_DOUBLECLICK_3						10035
#define IDS_DOUBLECLICK_4						10036

#define IDS_UAC_WARNING							10037

// Icons
#define IDI_MAIN	                            100

#endif
