/* ================================================================================ *
 * Comm.h                                                                           *
 *																					*
 * Serial Port Controll Class                                                       *
 * Serial Port를 통한 RS232 / RS422 / RS485 / Modem을 controll할 수 있도록 한다.    *
 * ================================================================================ */
#ifndef _COMM_H_
#define _COMM_H_

#define MAXCOMM                         10

/* ================================================================================ *
 * Define Window Message                                                            *
 * ================================================================================ */
#define WM_COMM                         WM_USER + 4

#define CM_RXCHAR                       0
#define CM_TXCHAR                       1
#define CM_TXEMPTY                      2
#define CM_CTSHIGH                      3
#define CM_CTSLOW                       4
#define CM_CTSCHANGE                    5
#define CM_RTSOUT                       6
#define CM_RING                         7
#define CM_QUEUEOVERFLOW                100

#define QUEUESIZE           1024*1024   // Queue Buffer size

#include <windows.h>

///////////////////////////////////////////////////////////////////////////////////////
//  CLASS : CQueue

class CQueue
{
protected:
    LPBYTE  m_pBuffer;
    int     m_iHead, m_iTail;           // Queue Top & Buttom Index
    int     m_nSize;                    // Queue에 들어 있는 Data갯수

public:
    // Contructor & Destructor
    CQueue();                           // Constructor
    virtual ~CQueue();                  // Destructor

    // Member function
    // Buffer가 비어 있는지를 검사한다.
    virtual BOOL  isEmpty() { return ((m_nSize==0) ? TRUE: FALSE); }
    virtual void  Clear(void);          // Buffer에 있는 내용을 모두 지우고 변수를 
                                        // 0으로 초기화 한다.
    // Data 처리 함수
    virtual int   GetSize(void) { return m_nSize; } // Data 수를 얻는다.
    virtual BOOL  PutByte(BYTE  ch);    // Queue에 1 byte를 넣는다. m_nSize, m_iTail 증가
    virtual BOOL  GetByte(BYTE *pch);   // Queue로 1 Byte를 얻는다. m_nSize, m_iHead 감소
    virtual BOOL  PutData( int nCount, LPBYTE lpDat );
    virtual int   GetData( int nCount, LPBYTE lpDat );

//    virtual int   SaveFile( CFile  cfFile );
};


///////////////////////////////////////////////////////////////////////////////////////
//  CLASS : CComm

class CComm
{
protected:
    HANDLE	m_hPort;                    // Port Handler
    int     m_nPort;                    // Port Number
    int     m_nBaudrate;                // Baud Rate
    int     m_nStopbit;                 // Stop Bit
    int     m_nParity;                  // Parity
    int     m_nThreadInx;               // Thread Port Index
    BOOL    m_fOpen;                    // Port가 Open되었는지를 나타냄.
    HWND    m_hWnd;                     // Message 수신 Handle
    HANDLE  m_hThreadProc;              // ThreadProc Handle
    DWORD   m_dwThreadID;               // ThreadID
    BOOL    m_bActThread;               // Active Thread

    OVERLAPPED  m_olRead;               // OVERLAPED Structure for Read
    OVERLAPPED  m_olWrite;              // OVERLAPED Structure for Write

public:
    CQueue  m_Queue;                    // Input Buffer
    BOOL    m_bThread;                  // Thread 상태  TRUE : Thread 동작중, FALSE : Thread 종료
    
    // Contructor & Destructor
    CComm();                            // Default Contructor
    virtual ~CComm();                   // Destructor

    // Open/Close Port
    virtual BOOL OpenPort(HWND hWnd, int nPort, int nBaudrate, int nStopbit, int nParity);
    virtual void ClosePort(void);
	virtual void ClosePort_value(void);
	virtual void SetHwnd(HWND hWnd); //csjang

    // Port관련 함수
    virtual DWORD  SendData(const void *pBuf, UINT nCount);
    virtual DWORD  RecvData( void );
	
    //virtual int	   IsRcvEmpty(void);
    virtual int    GetPortNum(void) { return m_nPort; }
    virtual int    GetParity(void) { return m_nParity; }
    virtual int    GetBaudrate(void) { return m_nStopbit; }
    virtual BOOL   isOpen(void) { return m_fOpen;  }
    virtual HANDLE GetHandle(void) { return m_hPort; }

    virtual void   SetRTS( BOOL fStat );
    
    // Thread 관련 함수
    virtual BOOL  SetThread(UINT (*pThread)(CComm *pComm));
    virtual BOOL  isActiveThread(void) { return ((m_bActThread==TRUE) ? TRUE: FALSE); }

    // Message관련 함수
    virtual BOOL SendMsg( UINT Msg, UINT MsgState );
};

#endif
