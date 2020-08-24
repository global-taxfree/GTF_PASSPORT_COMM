
#include "StdAfx.h"
#include "Comm.h"
#include "CommCtrl.h"
#include <direct.h>
#include <time.h>

// 장비와의 통신 프로토콜에 따라 수정 필요
#define COMM_HEADER	0x03
//#define COMM_OKPOS_HEADER	0x4d //OKPOS 여권스캐너 체크 헤더 추가
#define COMM_OKPOS_HEADER	0x56 //OKPOS 여권스캐너 체크 헤더 추가


void GetCurDtTm_comm(char *targetbuf, int type);
int PrintLog_comm(const char *fmt, ...);

CCommCtrl gCom;
static BOOL gCommAckA = FALSE;

//
// Local Function
//

void HexDump(unsigned char *pDcs, int len)
{
    char        pDisp[4096], tbuf[9], asc;
    int         line, col, dummy;

    printf("\nDump Data> total length = %d\n", len);
    printf("%s", "|    | 0  1  2  3  4  5  6  7  | 8  9  A  B  C  D  E  F  | |0123456789ABCDEF|\n");
    printf("%s", "|====|=========================|=========================| |================|\n");

    for ( line = 0; line <= (len/16); line++) {
        memset( pDisp, 0x00, sizeof(pDisp));
        sprintf( pDisp, _T("|%04d| "), line*16);   /* offset number. */

        for ( col = 0; (col < 8) && ((line*16+col) < len); col++) {
            sprintf(&tbuf[0], _T("%02x "), (unsigned char) *(pDcs+((line*16)+col)));
            strncat(pDisp, &tbuf[0], 3);
        }

        if (col < 8) {
            for (dummy=0; dummy < (8 - col); dummy++) {
                sprintf( &tbuf[0], "   ");
                strncat( pDisp, &tbuf[0], 3);
            }
        }

        sprintf( &tbuf[0], "| ");
        strncat( pDisp, &tbuf[0], 2);

        for (col = 8; (col <16) && ((line*16+col) < len); col++) {
            sprintf(&tbuf[0], "%02x ", (unsigned char) *(pDcs+((line*16)+col)));
            strncat( pDisp, &tbuf[0], 3);
        }

        if ( col < 16 ) {
            for (dummy = 0; dummy < (16 - col); dummy++) {
                sprintf( &tbuf[0], "   ");
                strncat( pDisp, &tbuf[0], 3);
            }
        }

        sprintf( &tbuf[0], "| |");
        strncat( pDisp, &tbuf[0], 4);   /* separator */

        for (col = 0; (col < 16) && ((line*16+col) < len); col++) {
            asc = *(pDcs+((line*16)+col));
            //sprintf( &tbuf[0], "%c", (isprint(asc) ? asc : '.'));
			sprintf( &tbuf[0], "%c", (0x21 <= asc && asc <= 0x80) ? asc : '.');
            strncat( pDisp, &tbuf[0], 1);
        }

        if (col < 16) {
            for (dummy = 0; dummy < (16 - col); dummy++) {
                sprintf( &tbuf[0], " ");
                strncat( pDisp, &tbuf[0], 1);
            }
        }

        sprintf( &tbuf[0], "|");
        strncat( pDisp, &tbuf[0], 2);   /* separator */

        printf("%s\n", pDisp);
    }

    printf("%s", "|====|=========================|=========================| |================|\n");
}

void RcvComm()
{
	PrintLog_comm(" RcvComm >> start \n");
	if(gCom.isOpen()) {
		PrintLog_comm(" RcvComm >> isOpen \n");
		BYTE pBuf[8] = {0};
		INT  iRcv = gCom.RecievePacket(3, pBuf);
		// 장비와의 규약에 따라 인식 코드 전송
		if(iRcv>0) {
			PrintLog_comm(" RcvComm >> iRcv > 0 : %d \n", iRcv );
			//HexDump(pBuf, 3);

			if(pBuf[0]==COMM_HEADER && pBuf[1]=='M' && pBuf[2]=='V')
				gCommAckA = TRUE;

			if(pBuf[0]==0x02 && pBuf[1]==0x00 && pBuf[2]==0x02){
				//printf("========> check!\n");
				gCommAckA = TRUE;
			}
		}
	}
}

BOOL CheckPort(HWND hWnd, INT iPort, INT iBaudRate )
{
	PrintLog_comm(" CheckPort >> start \n");
	if(gCom.OpenPort(hWnd, iPort, iBaudRate, 0, 0)) {
		PrintLog_comm(" CheckPort >> open \n");

		HANDLE hWaitEvt = CreateEvent(NULL, TRUE, FALSE, _T("COMPORTRCV\0"));
		if(hWaitEvt)
			WaitForSingleObject(hWaitEvt, 50);

		gCommAckA = FALSE;

		PrintLog_comm(" CheckPort >> send COMM_HEADER \n");

		char bCommData[8];
		memset(bCommData, 0, 8);
		bCommData[0] = COMM_HEADER;
		bCommData[1] = 'M';
		bCommData[2] = 'V';
		gCom.SendData(bCommData, 3);

		if(hWaitEvt)
			WaitForSingleObject(hWaitEvt, 1000);
		PrintLog_comm(" CheckPort >> send RcvComm start \n");
		RcvComm();
		PrintLog_comm(" CheckPort >> send RcvComm end \n");

		if(gCommAckA) {
			PrintLog_comm(" CheckPort >> ok and close port\n");
			//gCom.ClosePort();//2018.02.27 성공 시 closeport 삭제
			return TRUE;
		}		
		PrintLog_comm(" CheckPort >> not ok port\n");
		gCom.ClosePort();

		if(hWaitEvt)
			CloseHandle(hWaitEvt);
	}	

	return FALSE;
}
void RcvComm_OKPOS()
{
	PrintLog_comm(" RcvComm_OKPOS >> start \n");
	if(gCom.isOpen()) {
		PrintLog_comm(" RcvComm_OKPOS >> OPEN OK \n");
		//AfxMessageBox("RcvComm_OKPOS OPEN OK");
		BYTE pBuf[8] = {0};
		INT  iRcv = gCom.RecievePacket(1, pBuf);

		CString strTemp ;
		strTemp.Format("Recv Len:%d", iRcv);

		//AfxMessageBox(strTemp);

		PrintLog_comm(" RcvComm_OKPOS >> recv len %d \n", iRcv );
		// 장비와의 규약에 따라 인식 코드 전송
		if(iRcv>0) {
			
			if(pBuf[0]==COMM_OKPOS_HEADER )
			{
				PrintLog_comm(" RcvComm_OKPOS >> recv COMM_OKPOS_HEADER ok [%x] \n", pBuf[0] );
				gCommAckA = TRUE;
			}else
			{
				PrintLog_comm(" RcvComm_OKPOS >> COMM_OKPOS_HEADER no recv [%x] \n", pBuf[0] );
			}
		}else
		{
			
		}

		//Buffer Clear
		::memset( pBuf, 0, sizeof(pBuf) );

	}else
	{
		//AfxMessageBox("RcvComm_OKPOS OPEN FAIL");
	}
}

BOOL CheckPort_OKPOS(HWND hWnd, INT iPort, INT iBaudRate )
{
	PrintLog_comm(" CheckPort_OKPOS start \n");
	if(gCom.OpenPort(hWnd, iPort, iBaudRate, 0, 0)) {
		PrintLog_comm(" CheckPort_OKPOS open ok \n");
		HANDLE hWaitEvt = CreateEvent(NULL, TRUE, FALSE, _T("COMPORTRCV\0"));
		if(hWaitEvt)
			WaitForSingleObject(hWaitEvt, 50);

		gCommAckA = FALSE;

		char bCommData[8];
		memset(bCommData, 0, 8);
		bCommData[0] = COMM_OKPOS_HEADER;
		bCommData[1] = 0x00;
		bCommData[2] = 0x00;
		gCom.SendData(bCommData, 3);

		PrintLog_comm(" CheckPort_OKPOS open check send \n");
		//Buffer Clear
		::memset( bCommData, 0, sizeof(bCommData) );

		if(hWaitEvt)
			WaitForSingleObject(hWaitEvt, 1000);
		RcvComm_OKPOS();

		PrintLog_comm(" CheckPort_OKPOS open check recv \n");
		if(gCommAckA) {
			PrintLog_comm(" CheckPort_OKPOS open check ok and close \n");
			//gCom.ClosePort();//2018.02.27 성공 시 closeport 삭제
			//return TRUE;
		}else
		{
			PrintLog_comm(" CheckPort_OKPOS open check fail \n");
			//gCom.ClosePort_value(); //close 처리 예외 추가
			gCom.ClosePort();
		}
		
		//gCom.ClosePort(); Close 처리 제외
		//PrintLog_comm(" CheckPort_OKPOS ClosePort \n");
		//CloseHandle Block 처리
		if(hWaitEvt)
		{
			PrintLog_comm(" CheckPort_OKPOS ResetEvent before \n");
			ResetEvent(hWaitEvt);
			PrintLog_comm(" CheckPort_OKPOS ResetEvent after \n");
			CloseHandle(hWaitEvt);
		}
		if(gCommAckA)
			return TRUE;
	}	

	return FALSE;
}

// GTF 구형용
INT AutoDetect(HWND hWnd, INT iBaudRate )
{

	HCURSOR hCur    = LoadCursor(NULL, IDC_WAIT);
	HCURSOR hPrvCur = SetCursor(hCur);
	BOOL bFound=FALSE;
	INT iPort;

	for(iPort=1; iPort<20; iPort++)	{


		bFound = CheckPort(hWnd, iPort, iBaudRate );
		if(bFound) {
			break;
		}
	}

	int a= 0;

	if(!bFound)
		iPort =-1;

	SetCursor(hPrvCur);
	return iPort;
}
////////////////////////////// }

CCommCtrl::CCommCtrl()
{
    m_fSendCmd = FALSE;

    m_wRCmd = 0;
    m_wTCmd = 0;
    m_nPkinx= 0;
    m_nPklen= 0;
    m_nRcvstat = 0;
    m_nSendsize= -1;

    m_wStartChar = '@';
    
    m_nPkCount  = 0;
    m_nPkIn     = 0;
    m_nPkOut    = 0;
	m_Packet	= new PACKETINFO[MAX_PACKET];
	m_bRecvbuf	= new BYTE[MAX_BUFSIZE];
    m_bSendbuf	= new BYTE[MAX_BUFSIZE];
    m_bBuffer	= new BYTE[MAX_BUFSIZE];

	::memset( m_bRecvbuf, 0, sizeof(m_bRecvbuf) );
    ::memset( m_bSendbuf, 0, sizeof(m_bSendbuf) );
}

CCommCtrl::~CCommCtrl()
{
	if(m_Packet)
		delete m_Packet;
	if(m_bRecvbuf)
		delete m_bRecvbuf;
	if(m_bSendbuf)
		delete m_bSendbuf;
	if(m_bBuffer)
		delete m_bBuffer;
}

//  Resend 전송시 에러를 방지하기 위하여
BOOL CCommCtrl::Initialize( void )
{
    m_nSendsize = 0;
    m_nPkCount  = 0;
    m_nPkIn     = 0;
    m_nPkOut    = 0;
    return TRUE;
}

BOOL CCommCtrl::OpenPort( HWND hWnd, int nPort, int nBaudrate, int nStopbit, int nParity )
{
    Initialize();
    if ( !CComm::OpenPort( hWnd, nPort, nBaudrate, nStopbit, nParity ) )  return FALSE;
    return SetThread( CommThread );
}

BOOL CCommCtrl::GetPacket( PPACKETINFO pPacket )
{
    if( m_nPkCount==0 )   return false;

    CopyMemory( pPacket, m_Packet+m_nPkOut, sizeof(PACKETINFO) );
    m_nPkOut++;
    m_nPkCount--;
    return TRUE;
}

BOOL CCommCtrl::PutPacket( PPACKETINFO pPacket )
{
    if( m_nPkCount >= MAX_PACKET )   return false;

    CopyMemory( m_Packet+m_nPkIn, pPacket, sizeof(PACKETINFO) );
    m_nPkIn++;
    m_nPkCount++;
    return TRUE;
}

BOOL CCommCtrl::SetThread(UINT (*pThread)(CCommCtrl *pComm))
{
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

int CCommCtrl::RecievePacket( int nCount, LPBYTE pBuf )
{
    int     i = 0, cnt, count;

	count = 0;
    cnt = m_Queue.GetData( MAX_BUFSIZE, m_bBuffer );

	for(i=0; i<nCount&&i<cnt; i++) {
		pBuf[i] = m_bBuffer[i];
		count ++;
	}

	return count;


}

BOOL CCommCtrl::SendCommand( WORD wCmd, int nBodysize, LPBYTE pBody )
{
    BYTE    chksum = 0;
    int     i;
    
    m_bSendbuf[ 0 ] = (BYTE)m_wStartChar;
    m_bSendbuf[ 1 ] = (BYTE)(nBodysize >> 8);
    m_bSendbuf[ 2 ] = (BYTE)(nBodysize & 0xff);
    m_bSendbuf[ 3 ] = (BYTE)wCmd;
    m_bSendbuf[ 4 ] = 0;
    
    for( i=0 ; i<nBodysize ; i++ )
    {
        m_bSendbuf[ i+5 ] = *pBody;
        chksum += *pBody++;
    }

    m_bSendbuf[ 4 ] = chksum;
    SendData( m_bSendbuf, nBodysize+5 );

    m_wTCmd = wCmd;
    m_nSendsize = nBodysize + 5;

    return TRUE;
}

void CCommCtrl::ResendCommand( void )
{
    if( m_nSendsize!=-1 && m_nSendsize < 2048 && m_nSendsize )
        SendData( m_bSendbuf, m_nSendsize );
}

UINT CommThread( CCommCtrl *pComm )
{
    OVERLAPPED ol;
    DWORD dwEvent, dwRead;
    BOOL  bOk = TRUE;   // Return Value;

    // Event Overlapped struct 설정
    memset(&ol, 0, sizeof(ol));
    if (!(ol.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL)))  bOk = FALSE;
    if (!SetCommMask(pComm->GetHandle(), EV_RXCHAR | EV_CTS | EV_TXEMPTY)) bOk = FALSE;

    if( bOk==FALSE )
    {
        //CString str;        
        //str.Format ( _T("Error while creating Thread Com"),pComm);
        return FALSE;
    }

    // Thread Loop
    while ( pComm->isActiveThread() )
    {
        dwEvent = 0;
        WaitCommEvent(pComm->GetHandle(), &dwEvent, NULL);
        
        // Recieve char event
        if (dwEvent & EV_RXCHAR)
        {
            // Port에서 Data를 읽어 온다.
            while(1)
            {
                dwRead = pComm->RecvData();
                if(!dwRead)
					break;
                pComm->SendMsg( WM_COMM_PASSSCAN, CM_RXCHAR );
            }
        }
        else if (dwEvent & EV_TXEMPTY) {}
        else if (dwEvent & EV_CTS) pComm->SendMsg( WM_COMM_PASSSCAN, CM_CTSCHANGE );
    }
    pComm->m_bThread = FALSE;
    return TRUE;
}


void GetCurDtTm_comm(char *targetbuf, int type)
{
	time_t tmt;
	struct tm *calptr;

	time(&tmt);
	calptr = (struct tm *)localtime(&tmt);

	SYSTEMTIME st;
	GetLocalTime(&st);

	switch(type)
	{	
		case YYYYMMDD:
			strftime(targetbuf, 16, "%Y%m%d", calptr);
			break;
		case YYYY_MM_DD_hh_mm_ss:
			strftime(targetbuf, 32, "%Y/%m/%d %H:%M:%S", calptr);
			break;	
		case YYYYMMDDhhmmss:
			strftime(targetbuf, 32, "%Y%m%d%H%M%S", calptr);
			break;	
		case YYYY_MM_DD_hh_mm_ss_SSS:
			sprintf(targetbuf,"%04d/%02d/%02d %02d:%02d:%02d.%03d",st.wYear, st.wMonth,st.wDay, st.wHour, st.wMinute, st.wSecond , st.wMilliseconds);
			break;	
			
	}
	return;
}


int PrintLog_comm(const char *fmt, ...)
{
	//2017.03.20 로그출력 막음
	//return 0;

	va_list	ap;
	char curdttm[32];
	FILE *fp;
	char logfilename[100] = {0x00 , };
		
	memset(curdttm, 0x00, sizeof(curdttm)); 
	GetCurDtTm_comm(curdttm, YYYYMMDD);
	_mkdir("..\\gtf_log");

	//sprintf(logfilename, "c:\\SMART_POS\\log\\gtf%s.log", curdttm);
	sprintf(logfilename, "c:\\GTF_PASSPORT\\log\\gtf_pass_%s.log", curdttm);
	
	fp = fopen(logfilename, "a+");
	if(fp == NULL) return -1;
		
	memset(curdttm, 0x00, sizeof(curdttm)); GetCurDtTm_comm(curdttm, YYYY_MM_DD_hh_mm_ss_SSS);
	fprintf(fp, "[%s] ▷ ", curdttm);
		
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
		
	fflush(fp);
	fclose(fp);
	return TRUE;
}
