// GTF_PASSPORT_COMM.h : main header file for the GTF_PASSPORT_COMM DLL
//
#include <process.h>
#include <direct.h>
#include <time.h>
#include <vector>

#undef _USRDLL
#if !defined(AFX_GTF_PASSPORT_COMM_H__7291BA87_B480_46AF_9AD4_2BE04D6A13DC__INCLUDED_)
#define AFX_GTF_PASSPORT_COMM_H__7291BA87_B480_46AF_9AD4_2BE04D6A13DC__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#define YYYYMMDD                 1
#define YYYY_MM_DD_hh_mm_ss      2
#define YYYYMMDDhhmmss			 3
#define YYYY_MM_DD_hh_mm_ss_SSS	 4

/////////////////////////////////////////////////////////////////////////////
// CGTF_PASSPORT_COMMApp
// See GTF_PASSPORT_COMM.cpp for the implementation of this class
//

class CGTF_PASSPORT_COMMApp : public CWinApp
{
public:
	CGTF_PASSPORT_COMMApp();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGTF_PASSPORT_COMMApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CGTF_PASSPORT_COMMApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL InitInstance();

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GTF_PASSPORT_COMM_H__7291BA87_B480_46AF_9AD4_2BE04D6A13DC__INCLUDED_)
