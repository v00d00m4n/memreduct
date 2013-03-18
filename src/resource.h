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

// Menu Id
#define IDM_MAIN		                        100
#define IDM_TRAY		                        101

// Main Dlg
#define IDC_INFO								100
#define IDC_REDUCT								101

// Cleaner Dlg
#define IDC_RESULT								100

// Settings Dlg
#define IDC_TAB									100

// Page #1 Dlg
#define IDC_CHECK_UPDATE_AT_STARTUP_CHK			100
#define IDC_LOAD_ON_STARTUP_CHK					101
#define IDC_COLOR_INDICATION_CHK				102
#define IDC_MEASUREMENT_UNIT_CB					103
#define IDC_LANGUAGE_CB							104
#define IDC_LANGUAGE_INFO						105

// Page #2 Dlg
#define IDC_WORKING_SET_CHK						107
#define IDC_SYSTEM_WORKING_SET_CHK				108
#define IDC_MODIFIED_PAGELIST_CHK				109
#define IDC_STANDBY_PAGELIST_CHK				110
												
#define IDC_ASK_BEFORE_CLEANING_CHK				111
#define IDC_AUTOREDUCT_CHK						112
#define IDC_AUTOREDUCT_TB						113
#define IDC_AUTOREDUCT_PERCENT					114

// Page #3 Dlg									
#define IDC_REFRESHRATE							115
#define IDC_YELLOW_LEVEL						116
#define IDC_RED_LEVEL							117
#define IDC_DOUBLECLICK_CB						118

// Page #4 Dlg
#define IDC_BALLOON_SHOW_CHK					119
#define IDC_BALLOON_AUTOREDUCT_CHK				120
#define IDC_BALLOON_YELLOWLEVEL_CHK				121
#define IDC_BALLOON_REDLEVEL_CHK				122
#define IDC_BALLOONINTERVAL						123
								
// Common Controls
#define IDC_OK									1000
#define IDC_CANCEL								1001
#define IDC_APPLY								1002

#define IDC_LABEL_1								2000
#define IDC_LABEL_2								2001
#define IDC_LABEL_3								2002

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
#define IDS_MEM_FILL							10009
#define IDS_MEM_TOTAL							10010

#define IDS_TRAY_HIDE	                        10011
#define IDS_TRAY_TOOLTIP                        10012

#define IDS_BALLOON_AUTOREDUCT	                10013
#define IDS_BALLOON_YELLOWLEVEL                 10014
#define IDS_BALLOON_REDLEVEL                    10015

#define IDS_UNIT_KB_FULL		                10016
#define IDS_UNIT_MB_FULL	                    10017

#define IDS_UNIT_KB		                        10018
#define IDS_UNIT_MB		                        10019

#define IDS_CLEANER_COL_1	                    10020
#define IDS_CLEANER_COL_2	                    10021
#define IDS_CLEANER_COL_3	                    10022
												
#define IDS_CLEANER_SELECTAREA	                10023
#define IDS_CLEANER_WARNING			            10024
												
#define IDS_PAGE_1								10025
#define IDS_PAGE_2								10026
#define IDS_PAGE_3								10027
#define IDS_PAGE_4								10028

#define IDS_DOUBLECLICK_1						10029
#define IDS_DOUBLECLICK_2						10030
#define IDS_DOUBLECLICK_3						10031
#define IDS_DOUBLECLICK_4						10032

#define IDS_ONTOP								10033

// Icons
#define IDI_MAIN	                            100

#endif
