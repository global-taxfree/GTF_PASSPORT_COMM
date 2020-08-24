

#if !defined( _CCOMMCTRL_H_ )
#define  _CCOMMCTRL_H_

#if !defined( _COMM_H_ )
#include "Comm.h"
#endif  // _COMM_H_


// Compile용 Option flag 정의
#define FLG_ENABLEID            0

#define WM_COMM_PASSSCAN	WM_USER+7999


const int   MAX_BUFSIZE         = 2000;
const int   MAX_PACKET          = 256;

/* Error Code */
#define ERR_NONE                            0
#define ERR_NOTENOUGHMEMORY                 1
#define ERR_CHECKSUM                        2
#define ERR_OVERBUFFER                      3
#define ERR_UNDEFINED                       -1

#define YYYYMMDD                 1
#define YYYY_MM_DD_hh_mm_ss      2
#define YYYYMMDDhhmmss			 3
#define YYYY_MM_DD_hh_mm_ss_SSS	 4

typedef struct _PacketInfo
{
    WORD    m_wSize;
    WORD    m_wCommand;
    WORD    m_wRecvID;
    WORD    m_wSendID;
    int     m_nIndex;
    BYTE    m_bBody[4084];
}   PACKETINFO, *PPACKETINFO;

class CCommCtrl : public CComm
{
protected:
    int         m_nError;

    int         m_nMyid;

    BOOL        m_fSendCmd;
    WORD        m_wStartChar;

    //  Resend 과련 정보
    int         m_nSendsize;

    // Command Patch 관련변수
    WORD        m_wRCmd;
    WORD        m_wTCmd;
    WORD        m_wRecvID;
    WORD        m_wSendID;
    int         m_nPklen;
    int         m_nPkinx;
    int         m_nRcvstat;             // Command Packet State
    BYTE        m_bChksum1;
    BYTE        m_bChksum2;

	BYTE        *m_bRecvbuf;
    BYTE        *m_bSendbuf;
    BYTE        *m_bBuffer;
	
    int         m_nPkCount;
    int         m_nPkIn;
    int         m_nPkOut;
    PACKETINFO  *m_Packet;

public:
    CCommCtrl();
    virtual ~CCommCtrl();

    virtual BOOL Initialize( void );
    virtual BOOL OpenPort( HWND hWnd, int nPort, int nBaudrate, int nStopbit=0, int nParity=0 );

    virtual void SetMyid( int id )  {   m_nMyid = id;   }
    virtual int  GetMyid( void )    {   return m_nMyid; }

    virtual int  RecievePacket( int nCount=0, LPBYTE pBuf=NULL );
    virtual BOOL GetPacket( PPACKETINFO pPacket );
    virtual BOOL PutPacket( PPACKETINFO pPacket );
    virtual BOOL GetPacketCount( void )     {   return  m_nPkCount;   }
    virtual BOOL SendCommand( WORD wCmd, int nBodysize, LPBYTE pBody );
    virtual void ResendCommand( void );

    virtual int  GetErrorCode( void )       {   return  m_nError;   }
    virtual void ClearErrorCode( void )     {   m_nError = 0;       }

    virtual BOOL SetThread(UINT (*pThread)(CCommCtrl *pComm));
};

UINT CommThread(CCommCtrl *pComm);

#endif  // _CCOMMCTRL_H_
