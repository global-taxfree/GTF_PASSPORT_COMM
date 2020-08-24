/* ================================================================================ *
 * Comm.h                                                                           *
 * ================================================================================ *
 * -------------------------------------------------------------------------------- *
 * Serial Port Controll Class                                                       *
 * Serial Port를 통한 RS232 / RS422 / RS485 / Modem을 controll할 수 있도록 한다.    *
 * ================================================================================ */

#include "StdAfx.h"
#include "Comm.h"
#include <stdio.h>

//#define _COM_DUMP
#ifdef	_COM_DUMP
FILE* fp = NULL;
#endif

///////////////////////////////////////////////////////////////////////////////////////
//  CLASS : CQueue
CQueue::CQueue(void)
{
    m_iHead = 0;
    m_iTail = 0;
    m_nSize = 0;

    //m_pBuffer = (LPBYTE)GlobalAllocPtr( GHND, QUEUESIZE );
	m_pBuffer = (LPBYTE)calloc( GHND, QUEUESIZE );
    ZeroMemory( m_pBuffer, QUEUESIZE );
}

CQueue::~CQueue(void)
{
    //GlobalFreePtr( m_pBuffer );
	if(m_pBuffer)
		free(m_pBuffer);
}

BOOL CQueue::PutByte(BYTE ch)
{
    if (m_nSize >= QUEUESIZE) return FALSE;

    m_pBuffer[ m_iTail++ ] = ch;
    m_iTail %= QUEUESIZE;
    m_nSize++;

    return TRUE;
}

BOOL CQueue::GetByte(BYTE *pch)
{
    if (m_nSize==0)  return FALSE;
    *pch = m_pBuffer[m_iHead++];
    m_nSize--;
    m_iHead %= QUEUESIZE;
    return TRUE;
}

void CQueue::Clear(void)
{
    m_iHead = 0;
    m_iTail = 0;
    m_nSize = 0;

    ZeroMemory( m_pBuffer, QUEUESIZE );
}

BOOL CQueue::PutData( int nCount, LPBYTE lpDat )
{
    int     cnt;

    if ( lpDat==NULL )  return FALSE;
    if ( m_nSize>=QUEUESIZE )  return FALSE;

    if ( m_iTail+nCount < QUEUESIZE )
    {
        CopyMemory( m_pBuffer+m_iTail, lpDat, nCount );
        m_iTail += nCount;
        m_nSize += nCount;
    }
    else
    {
        cnt = QUEUESIZE - m_iTail;

        CopyMemory( m_pBuffer+m_iTail, lpDat, cnt );
        nCount -= cnt;
        m_nSize+= cnt;

        CopyMemory( m_pBuffer, lpDat, nCount );
        m_nSize+= nCount;
        m_iTail = nCount;
    }

    return TRUE;
}

int CQueue::GetData( int nCount, LPBYTE lpDat )
{
    int     cnt;
    
    if ( lpDat==NULL )  return 0;

    if ( nCount > m_nSize )   nCount = m_nSize;

    if ( m_iHead+nCount < QUEUESIZE )
    {
        CopyMemory( lpDat, m_pBuffer+m_iHead, nCount );
        m_nSize -= nCount;
        m_iHead += nCount;
    }
    else
    {
        cnt = QUEUESIZE - m_iHead;

        CopyMemory( lpDat, m_pBuffer+m_iHead, cnt );
        nCount -= cnt;
        m_nSize+= cnt;

        CopyMemory( lpDat, m_pBuffer, nCount );
        m_nSize+= nCount;
        m_iHead = nCount;

        nCount += cnt;
    }

    return ( nCount );
}

// int CQueue::SaveFile( CFile  cfFile )
// {
//     LPBYTE      pBuf;
//     int         size;
// 
//     pBuf = (LPBYTE)GlobalAllocPtr( GHND, m_nSize+10 );
//     if ( pBuf==NULL )   return -1;
// 
//     size = GetData( m_nSize+10, pBuf );
//     cfFile.Write( pBuf, size );
// 
//     return size;
// }

///////////////////////////////////////////////////////////////////////////////////////
//  CLASS : CComm

CComm::CComm()
{
    m_fOpen   = FALSE;
    m_nPort   = -1;
    m_bThread = FALSE;
    m_hPort = INVALID_HANDLE_VALUE;
	m_hThreadProc = INVALID_HANDLE_VALUE;
	m_hWnd  = (HWND)INVALID_HANDLE_VALUE;
}

CComm::~CComm()
{
    if (m_fOpen==TRUE) ClosePort();
}

void CComm::SetHwnd(HWND hWnd)
{
	m_hWnd  = hWnd;
}

BOOL CComm::OpenPort( HWND hWnd, int nPort, int nBaudrate, int nStopbit, int nParity )
{
    //CString	        strPort;
	TCHAR			strPort[256];
    COMMTIMEOUTS    cto;
    DCB             dcb;

    if (m_fOpen==TRUE) return FALSE;

    m_hWnd  = hWnd;
    m_nPort = nPort;
    m_nBaudrate = nBaudrate;
    ClosePort();

    // Overlaped structure 초기화 한다.
    // Read Event Object 생성
    m_olRead.Offset = 0;
    m_olRead.OffsetHigh = 0;
	m_olRead.hEvent = CreateEvent(NULL,TRUE, FALSE, NULL);
    if(!m_olRead.hEvent)
		return FALSE;
    
    // Write Event Object 생성
    m_olRead.Offset = 0;
    m_olRead.OffsetHigh = 0;
	m_olWrite.hEvent = CreateEvent(NULL,TRUE, FALSE, NULL);
    if(!m_olWrite.hEvent)
		return FALSE;

    // Open Port
	wsprintf(strPort, _T("\\\\.\\COM%d"), nPort);
    if ((m_hPort = CreateFile (strPort, GENERIC_READ | GENERIC_WRITE,
         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
         return FALSE;

    // Event 설정
    SetCommMask(m_hPort, EV_RXCHAR | EV_CTS | EV_TXEMPTY);

    // Serial Port Input/Output buffer size 설정
    SetupComm(m_hPort, 4096,4096);

    // In/Out buffer clear
    PurgeComm(m_hPort, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

    // Time out 설정
    cto.ReadIntervalTimeout = 5;	     // time out between ch. with ch to 0.1 Sec
    cto.ReadTotalTimeoutMultiplier = 10;
    cto.ReadTotalTimeoutConstant = 0;    // total string receive time out as 0.5 Sec
    cto.WriteTotalTimeoutMultiplier = 20;
    cto.WriteTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(m_hPort, &cto ))
    {
        ClosePort();
        return FALSE;  // Error Return
    }
	
    //------ SetupConnection
    dcb.DCBlength = sizeof (DCB);           // sizeof(DCB) 
    if (!::GetCommState(m_hPort, &dcb)) {
		return FALSE;                       // get error number; 
    }
   
    dcb.BaudRate= nBaudrate;                // current baud rate 
    dcb.fBinary = TRUE;	                    // binary mode, no EOF check 
    dcb.fParity = FALSE;                    // enable parity checking 
    dcb.fOutxCtsFlow = FALSE;               // CTS output flow control 
    dcb.fOutxDsrFlow = FALSE;               // DSR output flow control 
    dcb.fDtrControl = DTR_CONTROL_ENABLE;   // DTR flow control type 
    dcb.fDsrSensitivity = FALSE;            // DSR sensitivity 
    dcb.fTXContinueOnXoff = FALSE;          // XOFF continues Tx 
    dcb.fOutX = FALSE;                      // XON/XOFF out flow control 
    dcb.fInX = FALSE;                       // XON/XOFF in flow control 
    dcb.fErrorChar = FALSE;	                // enable error replacement 
    dcb.fNull = FALSE;                      // enable null stripping 
    dcb.fRtsControl = RTS_CONTROL_ENABLE;   // RTS flow control 
    dcb.fAbortOnError = FALSE;              // abort reads/writes on error 
    dcb.ByteSize = 8;			            // number of bits/byte, 4-8 
    dcb.Parity = (BYTE)nParity;				// 0-4=no,odd,even,mark,space 
    dcb.StopBits = (BYTE)nStopbit;          // 0,1,2 = 1, 1.5, 2 
    dcb.XonChar = 0x11;                     // Tx and Rx XON character 
    dcb.XoffChar = 0x13;                    // Tx and Rx XOFF character 

    if (!SetCommState(m_hPort,&dcb))
    {
        ClosePort();
        return FALSE;   		    // get error number; 
    }	

#ifdef _COM_DUMP
	if(!fp) {
		fp = fopen("c:\\comm.txt", "wt");
	}
#endif

    m_fOpen = TRUE;
    return TRUE;
}


BOOL CComm::SetThread(UINT (*pThread)(CComm *pComm))
{
    if ( !m_fOpen )  return FALSE;

    m_hThreadProc = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)pThread,this,0,&m_dwThreadID);
    if (m_hThreadProc==0) 
    {
        m_bActThread = FALSE;
        return FALSE;
    }
    m_bActThread = TRUE;
    m_bThread = TRUE;
    return TRUE;
}

void CComm::ClosePort(void)
{
    if (m_hPort == INVALID_HANDLE_VALUE)  return;
    if (m_fOpen == FALSE)  return;

    m_fOpen = FALSE;
    m_bActThread = FALSE;

    // Event mask clear
    SetCommMask(m_hPort, 0);
    
    // Buffer Clear
    PurgeComm(m_hPort, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

    // Clear RTS
	EscapeCommFunction(m_hPort, CLRRTS);

#ifdef _COM_DUMP
	if(fp) {
		fclose(fp);
		fp = NULL;
	}
#endif
    CloseHandle(m_hPort);
}



void CComm::ClosePort_value(void)
{
    if (m_hPort == INVALID_HANDLE_VALUE)  return;
    if (m_fOpen == FALSE)  return;

    m_fOpen = FALSE;
    m_bActThread = FALSE;

	/*
    // Event mask clear
    SetCommMask(m_hPort, 0);
    
    // Buffer Clear
    PurgeComm(m_hPort, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

    // Clear RTS
	EscapeCommFunction(m_hPort, CLRRTS);

#ifdef _COM_DUMP
	if(fp) {
		fclose(fp);
		fp = NULL;
	}
#endif
	*/
    CloseHandle(m_hPort);
	
}


DWORD CComm::SendData(const void *pBuf, UINT nCount)
{
    DWORD   dwWrite, dwError, dwErrorFlags;
    COMSTAT comstat;

#ifdef _COM_DUMP
	if(fp) {
		INT i;
		LPBYTE pd = (LPBYTE)pBuf;
		for(i=0; i<nCount; i++) {
			fprintf(fp, "%02X ", pd[i]);
		}
		fprintf(fp, "\n");
	}
#endif
    if (!WriteFile(m_hPort, pBuf, nCount, &dwWrite, &m_olWrite))
    {
        if (GetLastError()==ERROR_IO_PENDING)
        {
            while (!GetOverlappedResult(m_hPort,&m_olWrite,&dwWrite,TRUE))
            {
                dwError = GetLastError();
                if (dwError != ERROR_IO_INCOMPLETE)
                {
                    ClearCommError(m_hPort,&dwErrorFlags, &comstat);
                    break;
                }
            }
        }
        else
        {
            dwWrite = 0;
            ClearCommError(m_hPort,&dwErrorFlags, &comstat);
        }
    }
    return dwWrite;
}

void CComm::SetRTS( BOOL fStat )
{
    if ( fStat )
        ::EscapeCommFunction( m_hPort, CLRRTS );
    else
        ::EscapeCommFunction( m_hPort, SETRTS );
}

DWORD CComm::RecvData( void )
{
    DWORD dwRead, dwError, dwErrorFlags;
    COMSTAT comstat;
    BYTE    Buffer[2048];
    BOOL    fSuccess;

    ClearCommError(m_hPort, &dwErrorFlags, &comstat);
    dwRead = comstat.cbInQue;

    if (dwRead > 0)
    {
        if (!ReadFile(m_hPort, Buffer, 2048, &dwRead, &m_olRead))
        {
            if (::GetLastError()==ERROR_IO_PENDING)
            {
                while (!GetOverlappedResult(m_hPort, &m_olRead, &dwRead, TRUE))
                {
                    dwError = GetLastError();
                    if (dwError != ERROR_IO_INCOMPLETE)
                    {
                        ClearCommError(m_hPort, &dwErrorFlags, &comstat);
                        break;
                    }
                }
            }
            else
            {
                dwRead = 0;
                ClearCommError(m_hPort,&dwErrorFlags,&comstat);
            }
        }
#ifdef _DEBUGx
		INT i;
		for(i=0; i<(INT)dwRead; i++) {
			CHAR strDbg[32];
			
			sprintf(strDbg, "%02x ", (INT)Buffer[i]);
			OutputDebugString(strDbg);
		}
		if(dwRead>0)
		OutputDebugString("\n");
#endif
        fSuccess = TRUE;
        if ( !m_Queue.PutData(dwRead, Buffer) )
            fSuccess = FALSE;
    }
    return dwRead;
}

BOOL CComm::SendMsg( UINT Msg, UINT MsgState )
{
    return (::PostMessage(m_hWnd, Msg, (WPARAM)this, MsgState) );
}

