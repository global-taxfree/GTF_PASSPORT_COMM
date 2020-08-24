
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#include "DawinCtrl.h"

// DAWIN 스캐너용
int   ( FAR PASCAL * pConnect )();
int   ( FAR PASCAL * pSetAutoMode )(HWND hWnd, UINT wm_user);
int   ( FAR PASCAL * pResetAutoMode )();
int   ( FAR PASCAL * pDoScan )();
int   ( FAR PASCAL * pDisconnect )();
char* ( FAR PASCAL *pGetNation)();
char* ( FAR PASCAL *pGetSurname)();
char* ( FAR PASCAL *pGetGivenname)();
char* ( FAR PASCAL *pGetPassnum)();
char* ( FAR PASCAL *pGetBirthdate)();
char* ( FAR PASCAL *pGetExpiredate)();
char* ( FAR PASCAL *pGetSex)();
//2017.07.19 추가
char* ( FAR PASCAL *pGetMRZ1)();
char* ( FAR PASCAL *pGetMRZ2)();
char* ( FAR PASCAL *pSetSavePath)(char *strPath);


BOOL m_bConnect = FALSE;
HINSTANCE m_hDawinDll;

CDawinCtrl gDawin;


CDawinCtrl::CDawinCtrl(void)
{
	m_bConnect = FALSE;
	m_hDawinDll = 0;
}


CDawinCtrl::~CDawinCtrl(void)
{

//	FreeLibrary(m_hDawinDll);
//	m_hDawinDll = 0;

	m_bConnect = FALSE;
}

BOOL CDawinCtrl::Initialize( void )
{

	return TRUE;
}


BOOL CDawinCtrl::Connect(void)
{
	// Connect되어 있는 상태에서는 다시 Connect 하지 않음
	if(m_bConnect == FALSE)
	{
		if(m_hDawinDll == 0) DawinFuncLoad();
		m_bConnect = pConnect();
	}

	return m_bConnect;
}

BOOL CDawinCtrl::DisConnect(void)
{

	// Clear() 함수에서 호출 (DAWIN 전용) --> 종료 시에만 사용
	BOOL bRet = pDisconnect();
	FreeLibrary(m_hDawinDll);
	m_bConnect = FALSE;
	m_hDawinDll = 0;

	return bRet;
}

BOOL CDawinCtrl::Test_DisConnect(void)
{

	BOOL bRet = pDisconnect();
	FreeLibrary(m_hDawinDll);

	return bRet;
}


BOOL CDawinCtrl::Scan(void)
{
	BOOL bRet;

	bRet = pDoScan();

	return bRet;
}

BOOL CDawinCtrl::GetData(char *refPassInfo)
{
	char PassportInfo[65+1];

	memset(PassportInfo,	0x00,	sizeof(PassportInfo));
	memset(PassportInfo,	0x20,	65);

	// 전체이름으로 변환
	CString strTemp, strTemp1, strPassNo;

	strTemp  = pGetSurname();
	strTemp1 = pGetGivenname();
	strTemp = strTemp + _T(" ") + strTemp1;
	// 여권번호 (NULL 빼기위한 방법)
	strPassNo = pGetPassnum();

	strncpy((char*)&PassportInfo[0],  (LPCSTR)strTemp,          strTemp.GetLength());	// 전체 이름
	strncpy((char*)&PassportInfo[40], (LPCSTR)strPassNo,	    strPassNo.GetLength());	// 여권번호
	strncpy((char*)&PassportInfo[49], (LPCSTR)pGetNation(),	    3);		// 국가코드
	strncpy((char*)&PassportInfo[52], (LPCSTR)pGetSex(),	    1);		// 성별
	strncpy((char*)&PassportInfo[53], (LPCSTR)pGetBirthdate(),	6);		// 생년월일
	strncpy((char*)&PassportInfo[59], (LPCSTR)pGetExpiredate(),	6);		// 여권 만료기한

	memcpy(refPassInfo,	PassportInfo,	strlen(PassportInfo));

	return 1;
}

BOOL CDawinCtrl::GetMRZ1(char *refPassInfo)
{
	BOOL bRet = TRUE;
	memcpy(refPassInfo,	pGetMRZ1(),	strlen(pGetMRZ1()));
	return bRet;
}

BOOL CDawinCtrl::GetMRZ2(char *refPassInfo)
{
	BOOL bRet = TRUE;
	memcpy(refPassInfo,	pGetMRZ2(),	strlen(pGetMRZ2()));
	return bRet;
}

BOOL CDawinCtrl::SetSavePath(char *strSavePath)
{
	BOOL bRet = TRUE;
	pSetSavePath(strSavePath);
	return bRet;
}

BOOL CDawinCtrl::DawinFuncLoad(void)
{
	m_hDawinDll = LoadLibrary(_T("Dwscan.dll"));
	if (m_hDawinDll)
	{
		(FARPROC &)pConnect		    = GetProcAddress(m_hDawinDll, "Connect");
		(FARPROC &)pSetAutoMode     = GetProcAddress(m_hDawinDll, "SetAutoMode");
		(FARPROC &)pResetAutoMode   = GetProcAddress(m_hDawinDll, "ResetAutoMode");
		(FARPROC &)pDoScan          = GetProcAddress(m_hDawinDll, "DoScan");
		(FARPROC &)pDisconnect	    = GetProcAddress(m_hDawinDll, "Disconnect");
		(FARPROC &)pGetNation       = GetProcAddress(m_hDawinDll, "GetNation");
		(FARPROC &)pGetSurname      = GetProcAddress(m_hDawinDll, "GetSurname");
		(FARPROC &)pGetGivenname    = GetProcAddress(m_hDawinDll, "GetGivenname");
		(FARPROC &)pGetPassnum      = GetProcAddress(m_hDawinDll, "GetPassnum");
		(FARPROC &)pGetBirthdate    = GetProcAddress(m_hDawinDll, "GetBirthdate");
		(FARPROC &)pGetExpiredate   = GetProcAddress(m_hDawinDll, "GetExpiredate");
		(FARPROC &)pGetSex          = GetProcAddress(m_hDawinDll, "GetSex");

		(FARPROC &)pGetMRZ1          = GetProcAddress(m_hDawinDll, "GetMRZ1");
		(FARPROC &)pGetMRZ2          = GetProcAddress(m_hDawinDll, "GetMRZ2");
		(FARPROC &)pSetSavePath      = GetProcAddress(m_hDawinDll, "SetSavePath");

		if( !(pConnect) || !(pDisconnect) || !(pSetAutoMode) || !(pResetAutoMode) || !(pDoScan) || !(pGetNation)
			|| !(pGetSurname) || !(pGetGivenname) || !(pGetPassnum) || !(pGetBirthdate) || !(pGetExpiredate) || !(pGetSex)
			|| !(pGetMRZ1) || !(pGetMRZ2) || !(pSetSavePath)
			)
		{
			AfxMessageBox("Function load Fail!!");
		}
	}
	else
	{
		AfxMessageBox("Failed to load dll file!!");
		return FALSE;
	}

	return TRUE;
}