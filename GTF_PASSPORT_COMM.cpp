// GTF_PASSPORT_COMM.cpp : Defines the initialization routines for the DLL.
//
#include "stdAfx.h"
#include "GTF_PASSPORT_COMM.h"
#include "ComDef.h"
#include "Comm.h"
#include "CommCtrl.h"
#include "Serial.h"
#include "WS420Ctrl.h"
#include "DawinCtrl.h"
#include "BTC_PassScan.h"

void HexDump(unsigned char *pDcs, int len);

#define DATA_LEN	106

extern CCommCtrl	gCom;								// GTF용 구형
extern CWS420Ctrl   gWiseCom;							// Wisecude420용 통신
extern CDawinCtrl   gDawin;								// DAWIN용 내부함수

extern INT AutoDetect(HWND hWnd, INT iBaudRate );
extern BOOL CheckPort(HWND hWnd, INT iPort, INT iBaudRate );//2017.06.22 추가
extern BOOL CheckPort_OKPOS(HWND hWnd, INT iPort, INT iBaudRate );//2017.06.22 추가

static UINT	g_nPortNum = 0;
static UINT	g_nBaudRate = 115200;
static BOOL g_bCommOpened = FALSE;

static BYTE	g_CommBuf[512];
// }

int Set_ScannerKind(void);
int sendCmd(int nType);
//int AutoDetect_usb(int nType);
std::vector<int> AutoDetect_usb(int nType);
int AutoDetect_usb_type(int nType, int nPort);

int fixMrzData(const char *mrz1 , const char *mrz2 , const char *mrz3);
BOOL checkCRC(const char *data , int nCRC);

void GetCurDtTm(char *targetbuf, int type);
int PrintLog(const char *fmt, ...);
int WriteLog(const char *fmt, int len);
int StringFind(char *buf, int chk, int cnt);
void traceDebug(char *szFormat, ...);
void HexDump(unsigned char *pDcs, int len);

HWND g_hWnd;
CString g_MRZ1;
CString g_MRZ2;
CString g_MRZ3; //2017.05.18 MRZ3 추가
int  g_ScanKind = 0;					// 스캐너 종류 0:GTF 구형  1:wisecube420  2:DAWIN 3:OKPOS 여권스캐너
int  n_SelectScanKind = -1;				// 스캐너 종류 0:GTF 구형  1:wisecube420  2:DAWIN 3:OKPOS 여권스캐너

int  n_fixPort = -1;					// 강제설정 스캐너 포트

CSerial rs232;
CRITICAL_SECTION g_cx;

BOOL m_bTimeout = false;			//Timeout 시 처리 추가


///// BTC 변수 추가 //////
int b_ImageSendOn = 2;
bool b_enabledLog;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//


/////////////////////////////////////////////////////////////////////////////
// CGTF_PASSPORT_COMMApp

BEGIN_MESSAGE_MAP(CGTF_PASSPORT_COMMApp, CWinApp)
	//{{AFX_MSG_MAP(CGTF_PASSPORT_COMMApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGTF_PASSPORT_COMMApp construction

CGTF_PASSPORT_COMMApp::CGTF_PASSPORT_COMMApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGTF_PASSPORT_COMMApp object

CGTF_PASSPORT_COMMApp theApp;

extern "C" __declspec(dllexport) int __stdcall OpenPort();								// * 포트자동 스캔하여 PORT OPEN 함수명
extern "C" __declspec(dllexport) int __stdcall ClosePort();								// * 포트 CLOSE 함수
extern "C" __declspec(dllexport) int __stdcall Scan();									// * 여권스캔명령 함수
extern "C" __declspec(dllexport) int __stdcall ScanCancel();							// * 여권스캔 강제 취소 함수
extern "C" __declspec(dllexport) int __stdcall ReceiveData(int TimeOut);				// * 여권스캔이후 데이터 수신 데이터파싱 함수
extern "C" __declspec(dllexport) int __stdcall GetMRZ1(char *refMRZ1);
extern "C" __declspec(dllexport) int __stdcall GetMRZ2(char *refMRZ2);
extern "C" __declspec(dllexport) int __stdcall GetMRZ3(char *refMRZ3);
extern "C" __declspec(dllexport) int __stdcall GetPassportInfo(char *refPassInfo);		// * 데이터 파싱 이후 규격에 맞춰서 나오는 리턴함수
extern "C" __declspec(dllexport) int __stdcall Clear();                         // * 현재 사용하지 않음
extern "C" __declspec(dllexport) int __stdcall CheckReceiveData();              // * 수신 된 데이터 검증로직 함수
extern "C" __declspec(dllexport) int __stdcall OpenPortByNum(int PorNum);				// * 포트번호 지정하여 PORT OPEN 함수명(구)
extern "C" __declspec(dllexport) int __stdcall OpenPortByNumber(int PorNum);			// * 포트번호 지정하여 PORT OPEN 함수명
extern "C" __declspec(dllexport) int __stdcall IsOpen();						// * PORT OPEN 여부 확인 함수
extern "C" __declspec(dllexport) int __stdcall SetPassportScanType(int nType);			//2017.05.31 추가
extern "C" __declspec(dllexport) int __stdcall SetSavePath(char *strFilePath);			//2017.07.19 추가

using std::vector;
BOOL CGTF_PASSPORT_COMMApp::InitInstance()
{
	CWinApp::InitInstance();

	g_ScanKind = Set_ScannerKind();							// 스캐너 종류 파라미터

	return TRUE;
}


extern "C" __declspec(dllexport) int __stdcall OpenPort()
{
	PrintLog("OpenPort >> Open Start\n");
	CString strTempLog ;
	if(n_SelectScanKind <0)
		g_ScanKind = Set_ScannerKind();							// 스캐너 종류 파라미터
	else
		g_ScanKind = n_SelectScanKind;

	switch(g_ScanKind)
	{
	case 0:
	case 3:
	case 4:
		{
			//2018.02.27 PORT 강제 설정해둔 경우엔 지정된 포트로만 통신
			if(n_fixPort > -1)
			{
				g_nPortNum = 0;
				PrintLog("OpenPort >> Fix portcheck start %d \n", n_fixPort);
				HCURSOR hCur    = LoadCursor(NULL, IDC_WAIT);
				HCURSOR hPrvCur = SetCursor(hCur);
				HWND hWnd = g_hWnd;
				int nSearchPort = n_fixPort;

				BOOL nCheck = FALSE;
				if(g_ScanKind == 0)
				{
					g_nBaudRate = 115200;
					nCheck = CheckPort( hWnd, nSearchPort, g_nBaudRate );
				}else if (g_ScanKind == 3)
				{
					g_nBaudRate = 9600;
					/*
					nCheck = CheckPort_OKPOS(hWnd, nSearchPort, g_nBaudRate );
					*/
					nCheck = gCom.OpenPort(hWnd, nSearchPort, g_nBaudRate );
				}
						
				if(nCheck)
				{
					PrintLog("OpenPort >> Fix portcheck ok %d \n", nSearchPort);
					//gCom.ClosePort();//2018.02.27 성공 시 closeport 삭제
					g_nPortNum = nSearchPort;
				}else
				{
					PrintLog("OpenPort >> Fix portcheck fail %d \n", nSearchPort);
				}
				SetCursor(hPrvCur);
			}else
			{
				g_nPortNum = 0;
				//USB 연결 먼저 찾아봄
				//g_nPortNum = AutoDetect_usb(g_ScanKind);
				vector<int> vPort = AutoDetect_usb(g_ScanKind);
				//std::vector<int> vPort;
				if( vPort.size() >0)
				{
					HCURSOR hCur    = LoadCursor(NULL, IDC_WAIT);
					HCURSOR hPrvCur = SetCursor(hCur);
					//HWND hWnd = this->m_hWnd;
					HWND hWnd = g_hWnd;
					
						PrintLog("OpenPort >> size %d \n", vPort.size());
					for(int i=0; i < vPort.size() ; i++)
					{
						int nSearchPort = vPort.at(i);
						int nType = nSearchPort/1000; //1000 으로 나눈값을 Type 으로 생각함
						/*
						if(nType == 1)
						{
							g_nBaudRate = 115200;
						}
						else if(nType == 2)
						{
							g_nBaudRate = 9600;
						}
						*/
						//g_nBaudRate = 115200;
						if(g_ScanKind == 3)
						{
							g_nBaudRate = 9600;
						}else
						{
							g_nBaudRate = 115200;
						}

						nSearchPort = nSearchPort % 1000;

						//PrintLog("OpenPort >> portcheck %d \n", vPort.at(i));
						PrintLog("OpenPort >> portcheck %d \n", nSearchPort );
						BOOL nCheck = FALSE;
						if(g_ScanKind == 0)
						{
							//nCheck = CheckPort( hWnd, vPort.at(i), g_nBaudRate );
							nCheck = CheckPort( hWnd, nSearchPort, g_nBaudRate );
						}else if (g_ScanKind == 3)
						{
							//nCheck = CheckPort_OKPOS(hWnd, vPort.at(i), g_nBaudRate );
							nCheck = CheckPort_OKPOS(hWnd, nSearchPort, g_nBaudRate );
						}
						else if (g_ScanKind == 4)	// BTC
						{
							nCheck = 1;
						}
						if(nCheck)
						{
							//PrintLog("OpenPort >> portcheck ok %d \n", vPort.at(i));
							PrintLog("OpenPort >> portcheck ok %d \n", nSearchPort);
							//gCom.ClosePort();//2018.02.27 성공 시 closeport 삭제
							//g_nPortNum = vPort.at(i);
							g_nPortNum = nSearchPort;
							break;
						}
						PrintLog("OpenPort >> portcheck %d Continue \n", nSearchPort );
					}
					vPort.clear();
					SetCursor(hPrvCur);
				}
			}

			// 자동으로 port 번호 검색
			if(g_nPortNum == 0)
			{	
				if(g_ScanKind == 0)
				{
					g_nPortNum = AutoDetect(g_hWnd, g_nBaudRate);
				}
			}
			
			// Open된 상태에 따라서 Port Open 처리
			if( !g_bCommOpened ){
				if(g_nPortNum > 0 && g_nPortNum != -1 )
				{	
					//Sleep(200);//ver.1 sleep 추가 
					PrintLog("OpenPort >> g_nPortNum:%d \n", g_nPortNum);
					PrintLog("OpenPort >> PortOpen 1 \n");
					if (g_ScanKind == 4) {	// BTC
						if (GetFileAttributes(_T("C:\\GTF_PASSPORT\\log")) == -1) {
							b_enabledLog = false;
						}
						else {
							b_enabledLog = true;
						}
						g_bCommOpened = PassScan_Open_PortNoGTF(g_nPortNum, b_enabledLog);	// BTC Serail Open 함수
						b_ImageSendOn = PassScan_ImageSendingStatus_Check();	// Image 전송 여부 Check (1: Send On / 2: Send Off)
					}
					else {
						//g_bCommOpened = gCom.OpenPort(g_hWnd, g_nPortNum, g_nBaudRate );
						g_bCommOpened = TRUE;
					}
					PrintLog("OpenPort >> PortOpen 2 \n");
				}
			}else{
				gCom.SetHwnd(g_hWnd);
			}
		}
		break;

	case 1:
		g_nPortNum = gWiseCom.SearchPort(&rs232);
		g_bCommOpened = gWiseCom.Open(&rs232, g_nPortNum, g_nBaudRate );
		break;

	case 2:
		if(!g_bCommOpened)
		{
			g_bCommOpened = gDawin.Connect();					// 1:성공  0:실패
		}
		break;

	}

	if( g_bCommOpened )
	{
		::InitializeCriticalSection(&g_cx);
		PrintLog("OpenPort >> Open success (return 1)\n");
		return 1;
	}
	PrintLog("OpenPort >> Open fail (return 0)\n");
	return 0;
}


extern "C" __declspec(dllexport) int __stdcall OpenPortByNum(int PorNum)
{
	return OpenPortByNumber( PorNum);
}

extern "C" __declspec(dllexport) int __stdcall OpenPortByNumber(int PorNum)
{
	PrintLog("OpenPortByNumber >> Open Start(port:%d)\n",PorNum);

	//g_ScanKind = Set_ScannerKind();							// 스캐너 종류 파라미터

	if(n_SelectScanKind <0)
		g_ScanKind = Set_ScannerKind();							// 스캐너 종류 파라미터
	else
		g_ScanKind = n_SelectScanKind;


	if(g_ScanKind == 3)
	{
		//AutoDetect_usb_type(g_ScanKind, PorNum);
		g_nBaudRate = 9600;
	}else
	{
		g_nBaudRate = 115200;
	}
	CString strTemp;
	strTemp.Format("OpenPortByNumber >> ScnnerKind: %d , PorNum: %d , g_nBaudRate %d \n", g_ScanKind, PorNum,g_nBaudRate);
	PrintLog(strTemp);
	switch(g_ScanKind)
	{
	case 0:
	case 3:
		
		if( !g_bCommOpened ){
			BOOL bCheck  = false;
			if(g_ScanKind == 3)
			{
				//AutoDetect_usb_type(g_ScanKind, PorNum);
				g_nBaudRate = 9600;
				//bCheck = CheckPort_OKPOS(g_hWnd, PorNum, g_nBaudRate );
				bCheck = gCom.OpenPort(g_hWnd, PorNum, g_nBaudRate );
				//bCheck = true;//OKPOS 여권스캐너일때 스캔 명령 바로 전송
			}else
			{
				g_nBaudRate = 115200;
				bCheck = CheckPort( g_hWnd, PorNum, g_nBaudRate );
			}
			/*
			BOOL bCheck = CheckPort_OKPOS(g_hWnd, PorNum, g_nBaudRate );
			*/
			if(bCheck)
			{
				//2018.02.27 성공 시 closeport 삭제
				//Sleep(200);//ver.1 sleep 추가 
				//g_bCommOpened = gCom.OpenPort(g_hWnd, PorNum, g_nBaudRate );
				g_bCommOpened = TRUE;
				g_nPortNum = PorNum;

			}else
			{
				if(g_ScanKind == 3)
				{
					gCom.ClosePort();
				}
			}
		}else{
			gCom.SetHwnd(g_hWnd);
		}
		break;

	case 1:
		g_bCommOpened = gWiseCom.Open(&rs232, PorNum, g_nBaudRate );
		break;

	case 2:
		g_bCommOpened = gDawin.Connect();					// 1:성공  0:실패
		break;

	case 4:
		if (GetFileAttributes(_T("C:\\GTF_PASSPORT\\log")) == -1) {
			b_enabledLog = false;
		}
		else {
			b_enabledLog = true;
		}
		g_nPortNum = PorNum;
		g_bCommOpened = PassScan_Open_PortNoGTF(g_nPortNum, b_enabledLog);	// BTC Serail Open 함수
		b_ImageSendOn = PassScan_ImageSendingStatus_Check();	// Image 전송 여부 Check (1: Send On / 2: Send Off)
		
		if (b_ImageSendOn == 1) {
			;
		}



		break;
	}

	if( g_bCommOpened )
	{
		::InitializeCriticalSection(&g_cx);
		PrintLog("OpenPortByNumber >> Open success (return 1)\n");
		return 1;
	}

	PrintLog("OpenPortByNumber >> Open fail (return 0)\n");
	return 0;
}


enum
{
	CHK_STX,
	CHK_LENGTH,
	CHK_COMMAND,
	CHK_DATA,
	CHK_ETX,
	CHK_LRC
};

int ExceptionReceiveData(int nLen = 200) //Serial Port buffer Clear 
{
	switch(g_ScanKind)
	{
	case 0:
	case 3:
	    //gCom.m_Queue.GetData( 200, g_CommBuf );
		gCom.m_Queue.GetData( nLen, g_CommBuf );
		break;

	case 1:
		break;

	case 2:
		break;
	}

	return 1;
}

int ReceiveCommData()
{
    int     cnt;

	char    MRZ1[45];
	char    MRZ2[45];
	char    MRZ3[45]; //2017
	int     rcv_len=0;
	int     state=CHK_STX;
	BYTE    rcv_packet[512];

	char    cTempData[500];

    int     len=0;
	int		run_f=1;
	int		nErrNo = 1;

	int		nloop = 0;

	memset(rcv_packet,	0x00,	sizeof(rcv_packet));
	memset(g_CommBuf,	0x00,	sizeof(g_CommBuf));
	switch(g_ScanKind)
	{
	case 0:
		{
		for(int i =0; i < 20 ; i ++)
		{
			cnt = gCom.m_Queue.GetData( DATA_LEN, g_CommBuf );
			//cnt = gCom.m_Queue.GetData( 25, g_CommBuf );
			if (g_CommBuf[0] == 0xe4) {		// BTC 실패 프로토콜 왔을 때 무조건 실패 처리
				return -1;
			}
			printf("%d ", cnt);

			PrintLog("ReceiveCommData >> recv len: %d \n", cnt);
			
			if(cnt !=0 && (rcv_len + cnt) != 106 && (rcv_len + cnt) != 7)
			{
				CString str = TEXT("");
				CString s;
				for(int i=0;i<cnt;i++)
				{
					str.Format("%02x ", (BYTE)g_CommBuf[i]);
					s += str;
				}

				//PrintLog("ReceiveCommData >> ERROR DATA len: %d , data : %s \n", cnt, s);
				PrintLog("ReceiveCommData >> ERROR DATA len: %d  \n", cnt);
				str.Empty();
				s.Empty();
			}
			

			if(!cnt)
			{
				//데이터가 7이면 에러
				if(rcv_len == 7)
					return -1;
				return 0;
			}
			else
			{
					memcpy((char*)&rcv_packet[rcv_len], (const char*)g_CommBuf, cnt);
			}
			
			rcv_len+=cnt;


			
			PrintLog("ReceiveCommData >> rcv_len:%d  \n" , rcv_len);
			if( rcv_len >= 106  )
			{
				break;
			}
			Sleep(10);
			PrintLog("ReceiveCommData >> continue  \n");
		}

		while(run_f)
		{
			if(nloop > 100)
			{
				PrintLog("ReceiveCommData >> loof over 100 \n");
				rcv_len=0;
				state=CHK_STX;
				return 0;
			}
			nloop ++;
			switch(state)
			{
				case CHK_STX:
					if(rcv_packet[0]==0x02)
					{
						state=CHK_LENGTH;
					}else
					{
						rcv_len=0;
						state=CHK_STX;
						return 0;
					}
					break;
				case CHK_LENGTH:
					if(rcv_len>2)
					{
						//len =  *( (unsigned short *)&rcv_packet[1]);
						len =  rcv_packet[1]<<8 | rcv_packet[2];
						if(rcv_len> (len+3))
						{
							state=CHK_COMMAND;
						}
						else
						{
							rcv_len=0;
							state=CHK_STX;
							return 0;
						}
					}
					else
					{
						rcv_len=0;
						state=CHK_STX;
						return 0;
					}

					break;
				case CHK_COMMAND:
					if(rcv_packet[3]==0xB1)
					{
						state=CHK_DATA;
					}
					else
					{
						rcv_len=0;
						state=CHK_STX;
						return 0;
					}
					break;
				case CHK_DATA:
				case CHK_ETX:
					if(len == 102)  //success
					{
						memset(MRZ1,0x00,45);
						memset(MRZ2,0x00,45);
						memset(MRZ3,0x00,45);//2017.05.18 처리
	
						strncpy(MRZ1, (const char *)&rcv_packet[4],44);
						strncpy(MRZ2, (const char *)&rcv_packet[48],44);
						/*
						int nFixMrz = fixMrzData(MRZ1, MRZ2 , MRZ3);
						if(nFixMrz <0) //2017.05.18 오류
						{
							g_MRZ1 = "";
							g_MRZ2 = "";
						}else
						{
							g_MRZ1 = (CString)MRZ1;
							g_MRZ2 = (CString)MRZ2;
						}*/

						g_MRZ1 = (CString)MRZ1;
						g_MRZ2 = (CString)MRZ2;


						if(rcv_packet[104] == 0x03) //ETX
							state=CHK_LRC;
						else
						{
							rcv_len=0;
							state=CHK_STX;
							return 0;
						}
					}
					else
					{
						if(rcv_packet[5] == 0x03) //ETX
							state=CHK_LRC;
						else
						{
							rcv_len=0;
							state=CHK_STX;
							return 0;
						}
						return -1;
					}
					break;
				case CHK_LRC:
					rcv_len = (len == 102)?  (rcv_len-106) : (rcv_len-7);
					if(rcv_len)
					{
						int i = (len == 102)?  106:7;

						for(int j=0;j<rcv_len;j++,i++)
						{
							rcv_packet[j]=rcv_packet[i];
						}
					}
					run_f=0;
					state=CHK_STX;
					break;
				default: //2017.11.16 default 추가
					run_f=0;
					return 0;
					break; 
			}
		}
		}

		break;

	case 1:
		{
		char response[1024];		// command response buff
		char cmd[3] = {0};
		char dat[1024];
		int  timer_req  = 0;
		int	 timercount = -1;		// timeout count value of one shot state
		int  ret = 0;
		int  cnt = 0;
		
		CString strLog = "";

		gWiseCom.Flush(&rs232);

		memset(response, 0x00, sizeof(response));
		memset(dat, 0x00, sizeof(dat));

		while(1)
		{
			//2016.12.27 Waiting 처리 삭제
			//if( gWiseCom.ReadDataWaiting(&rs232) )
			//{
				ret = gWiseCom.ReadData(&rs232, response, 1);
				if( ret == 1 ) 
				{
					// **********************************************
					// command response 
					// **********************************************
					if( response[0] == '#' )
					{
						ret = gWiseCom.ReadUpto( &rs232, &response[1], sizeof(response) - 2, 100, 0X00 );		// 명령 2바이트를 다시 보냄

						if( ret > 0 ) 
						{
							ret = gWiseCom.check_response( response, ret + 1, cmd, dat, sizeof(dat) );

							switch(ret)
							{
								case 0:
									printf( "[ER]:no response\n" );
									break;
								case 1:					// 정상 응답 (P)
									printf( "[OK]:%s \nCMD:%s DAT:%s\n",response, cmd, dat );

									if( cmd[0] == 'G' && cmd[1] == 'S' ) {                             // #GS 기기상태 확인
										printf( "sensor: ");
										if(      dat[0] == '0') printf( "Document not detected\n");
										else if( dat[0] == '1') printf( "Carriage is in left side\n");
										else if( dat[0] == '2') printf( "Carriage is in right side\n");

										printf( "State : ");
										if(      dat[1] == 'I') printf( "Idle state\n");
										else if( dat[1] == 'R') printf( "Run State\n");
										else if( dat[1] == 'O') printf( "One shot state\n");
										else if( dat[1] == 'P') printf( "Run State\n");
										else if( dat[1] == 'Q') printf( "One shot state\n");
									}

									if( cmd[0] == 'S' && cmd[1] == 'T' ) {                              // Set Status 명령 #ST 기기의 동작상태 설정
										if( dat[0] == 'O' && timer_req == 1 )							// STO : 일회동작상태
										{
											timercount = 100;
											timer_req = 0;
										}
										else
											timercount = -1;	// disable timer
									}

									break;
								case 2:					// 비정상응답 (N)
									printf( "[OK]:%s \nCMD:%s ERR:%s\n",response, cmd, dat );
									char strDat[5];
									strDat[4] = 0x00;
									memcpy(strDat, dat, 4);
									nErrNo = atoi(strDat) * -1; 
									return nErrNo;

									break;
								case -1:				// 에러
									printf( "[ER]:%s\n",response);
									break;
							}
						}

					}
					else if( response[0] == 0x02 ) 
					{
						// **********************************************
						// passport data receive
						// **********************************************
						memset(&response[1], 0x00, sizeof(response) - 1);
						ret = gWiseCom.ReadUpto( &rs232, &response[1], sizeof(response) - 2, 100, 0X03 );
						response[ret] = 0x00;
						printf( "%s\n", &response[1] );

						if(response[1] == 'E')			// 문자 인식 오류
						{
							nErrNo = -10;
							break;
						}

						//2017.06.15 size 체크 추가
						if(sizeof(response) < 90) 
						{
							nErrNo = -10;
							break;
						}

						memset(MRZ1,0x00,45);
						memset(MRZ2,0x00,45);

						strncpy(MRZ1, (const char *)&response[2],44);
						strncpy(MRZ2, (const char *)&response[46],44);
	
						g_MRZ1 = (CString)MRZ1;
						g_MRZ2 = (CString)MRZ2;

						timercount = -1;

						break;
					}
				}else 
				{
					Sleep(10);
					cnt ++;
				}
			//} 
			/*
			else 
			{
				Sleep(100);
				cnt ++;
			}
			*/

			// **********************************************
			// oneshot mode timeout check
			// **********************************************
			if( timercount > 0 ) 
			{
				timercount --;
				if( timercount == 0 ) {
					ret = gWiseCom.SendCommand( &rs232, _T("STI") );
					timercount = -1;
				}
			}

			if(cnt == 10)
			{
				nErrNo = 0;
				break;
			}
		}
		break;
		}

	case 2:
		break;
	case 3: //OKPOS 여권스캐너. 스캔 전, 여권스캐너에 여권이 먼저 들어가 있어야 한다. Timeout 체크 하지 않음
		{
			//char response[1024];		// command response buff
			char cmd[3] = {0};
			BOOL bJobEnd = false;
			int nTotalDataLen = 0;
			int nCmdCnt = 3;
			//char dat[1024];

			int nLoopCnt = 0;
			CString strRes ;
			CString strTemp ;
			strRes.Empty();
			state = 0;
			cnt = 0;

			//수신받은 3바이트를 변수에 세팅
			memset(cmd,	0x00,	sizeof(cmd));
			memset(rcv_packet,	0x00,	sizeof(rcv_packet));

			while(run_f)
			{
				//Scan 명령 후 수신 대기 시간 감안하여 nLoop 처리
				if(nLoopCnt> 50) //최대 대기시간 5초
				{
					nErrNo = -1;
					bJobEnd = true;
					PrintLog("ReceiveData >> Result : gtf Timeout\n");
				}
				else
					nLoopCnt++;

				switch(state)
				{
					case 0: //신호대기
						cnt = gCom.m_Queue.GetData( nCmdCnt, g_CommBuf );
					
						if ( cnt>0)
						{
							memcpy((char*)&cmd[rcv_len],(const char*)g_CommBuf, sizeof(cmd)); //커맨드 신호 처리
							memcpy((char*)&rcv_packet[rcv_len],(const char*)g_CommBuf, cnt); //받은 총 데이터
							nCmdCnt  = nCmdCnt - cnt;
						}
						rcv_len+=cnt;
						if(rcv_len >=3)
						{
							nTotalDataLen = cmd[1] * 256 + cmd[2]; //총길이 계산
							strTemp.Format("total Len:%d\n", nTotalDataLen);
							PrintLog(strTemp);
							state = 1;
						}
						break;
					case 1: //데이터 대기
						cnt = gCom.m_Queue.GetData( nTotalDataLen, g_CommBuf );
						if(cmd[0] == 'I')
						{
							if(cnt)
							{
								memcpy((char*)&rcv_packet[rcv_len],(const char*)g_CommBuf, cnt); //받은 총 데이터
								rcv_len+=cnt;
							}								
							
							if( rcv_len  >= nTotalDataLen )
							{
								if(nTotalDataLen < (88))//mrz 여권데이터 길이 미만인 경우는 return
								{
									//return -1;
									nErrNo = -1;									
									bJobEnd = true;
								
									if(nTotalDataLen <=3)
									{
										PrintLog("ReceiveData >> Result : Mrz Data Empty \n");
									}else
									{
										//memset(cTempData,0x00,500);
										//strncpy(cTempData, (const char *)&rcv_packet[3],nTotalDataLen-3);//3바이트 cmd 제외 하고 시작
										//strRes = (CString)cTempData;
										//strRes.TrimRight();
										PrintLog("ReceiveData >> Result : Mrz Data Short, Len :%d\n", nTotalDataLen );
										//memset(cTempData,0x00,500);
										//ZeroMemory(&cTempData,500);
										//strRes.Empty();
									}

								}else //여권데이터 길이 정상인 경우에는
								{
									sendCmd(1);//Cancel 명령 호출

									PrintLog("ReceiveData >> Result Normal \n" );
									memset(cTempData,0x00,500);
									//strncpy(cTempData, (const char *)&rcv_packet[3],100);//3바이트 cmd 제외 하고 시작
									strncpy(cTempData, (const char *)&rcv_packet[3],nTotalDataLen );//3바이트 cmd 제외 하고 시작
									/*
									strRes = (CString)cTempData; 
									AfxExtractSubString( g_MRZ1, strRes, 0, 0x0d);
									AfxExtractSubString( g_MRZ2, strRes, 1, 0x0d);
									AfxExtractSubString( g_MRZ3, strRes, 2, 0x0d);
									*/
									int nMrzCount =0;
									int nMrzOffset =0;
									memset(MRZ1,0x00,45);
									memset(MRZ2,0x00,45);
									memset(MRZ3,0x00,45);//2017.05.18 처리
									
									for(int i=0; i < nTotalDataLen ; i ++)
									{
										if(cTempData[i] == 0x0d)
										{
											if(nMrzCount == 0)
											{
												strncpy(MRZ1,(const char *)&cTempData[nMrzOffset], (i - nMrzOffset) > 44 ? 44 : (i - nMrzOffset) );
												nMrzCount ++;
												nMrzOffset = i+1;
											}else if (nMrzCount ==1)
											{
												strncpy(MRZ2,(const char *)&cTempData[nMrzOffset], (i - nMrzOffset) > 44 ? 44 : (i - nMrzOffset) );
												nMrzCount ++;
												nMrzOffset = i+1;
											}else if (nMrzCount ==2)
											{
												strncpy(MRZ3,(const char *)&cTempData[nMrzOffset], (i - nMrzOffset) > 44 ? 44 : (i - nMrzOffset) );
												nMrzCount ++;
												nMrzOffset = i+1;
											}
										}
									}

									PrintLog("ReceiveData >> Result Parse \n" );
									g_MRZ1 = CString(MRZ1);
									g_MRZ2 = CString(MRZ2);
									g_MRZ3 = CString(MRZ3);

									PrintLog("ReceiveData >> Result Data Set \n" );

									int nFixMrz = fixMrzData((const char *)g_MRZ1, (const char *)g_MRZ2 , (const char *)g_MRZ3);

									if(nFixMrz <0) //2017.05.18 오류
									{
										g_MRZ1 = "";
										g_MRZ2 = "";
										g_MRZ3 = "";
										nErrNo = -1;
										PrintLog("ReceiveData >> Result : Check Digit Error\n");
									}else
									{
										nErrNo = 1;
										PrintLog("ReceiveData >> Result : Success\n");
									}

									memset(cTempData,0x00,500);
									ZeroMemory(&cTempData,500);
									strRes.Empty();
									PrintLog("ReceiveData >> Result Buffer Clear \n" );
									/*
									strRes = (CString)cTempData; 
									AfxMessageBox(strRes);

									memset(MRZ1,0x00,45);
									memset(MRZ2,0x00,45);
									strncpy(MRZ1, (const char *)&rcv_packet[3],44);//3바이트 cmd 제외 하고 시작
									strncpy(MRZ2, (const char *)&rcv_packet[48],44);//1바이트 개행 문자 제외 하고 시작
									g_MRZ1 = (CString)MRZ1;
									g_MRZ2 = (CString)MRZ2;
									*/
									
									
									bJobEnd = true;
								}		
							}
						}
						//상태값 처리
						else if(
							cmd[0]  == 'M'		// GetCommunicationMode
							|| cmd[0]  == 'R'	//GetSerialNumber
							|| cmd[0] == 'T'	//GetProductInfo
							|| cmd[0] == 'V'    //GetVersion
							|| cmd[0] == 'W'    //GetOCRVersion
							)
						{
							//nErrNo = 1;
							//bJobEnd = true;
						}
						break;
				}	
				//recv Count 증가
				//에러코드 처리
				if(bJobEnd)
					break;
				Sleep(100);
			}
		}

		break;
	}

	return nErrNo;
}

extern "C" __declspec(dllexport) int __stdcall Scan()
{
	PrintLog("Scan >> start\n");
	switch(g_ScanKind)
	{
	case 0:
		unsigned char packet[6];
		if( g_bCommOpened )
		{
			ExceptionReceiveData(1024);

			// Scanner Code send
			memset(packet,0x00,6);
			packet[0]=0x02;  //STX
			packet[1]=0x00;  //size
			packet[2]=0x02;  //size
			packet[3]=0xA1;  //Command ID
			packet[4]=0x03;  //ETX
			for(int i=1;i<5;i++)
				packet[5]^=packet[i];

			gCom.SendData( packet,6);
			
			PrintLog("Scan >> send ok\n");
			return 1;
		}
		break;

	case 1:
		if( g_bCommOpened )
		{
			gWiseCom.SendCommand( &rs232, _T("STQ") );			// 일회동작상태 (여권이 놓이면 데이터를 읽고 데이터 송부, 오류 응답 있음)
			PrintLog("Scan >> send ok\n");
			return 1;
		}

		break;

	case 2:
		if( g_bCommOpened )
		{
			gDawin.Scan();
			PrintLog("Scan >> send ok\n");
			return 1;
		}
		break;
	case 3:
		if( g_bCommOpened )
		{
			Sleep(100);

			ExceptionReceiveData(1024);

			unsigned char packet[3];
			// Scanner Code send
			memset(packet,0x00,3);
			packet[0]=0x49;  
			packet[1]=0x00;  
			packet[2]=0x00;  
			
			//for(int i=1;i<5;i++)
			//	packet[5]^=packet[i];

			gCom.SendData( packet,3);
			Sleep(100);
			PrintLog("Scan >> send ok\n");
			return 1;
		}
		break;

	case 4:
		if (g_bCommOpened)
		{
			PrintLog("Scan >> send ok\n");
			return 1;
		}
		break;
	}
	PrintLog("Scan >> send false\n");
	return 0;
}

extern "C" __declspec(dllexport) int __stdcall ScanCancel()
{
	PrintLog("ScanCancel >> start\n");
	int nRet = 0;
	unsigned char packet[6];

	if( g_ScanKind !=0 && g_ScanKind != 3 && g_ScanKind != 4)			// GTF 구형 / OKPOS 여권스캐너 에서만 구현 가능 (캔슬 모드)
	{
		PrintLog("ScanCancel >> cancel\n");
		return 0;
	}

	
	if( g_bCommOpened )
	{
		if(g_ScanKind == 0)
		{
			// Scanner Code send
			memset(packet,0x00,6);
			packet[0]=0x02;  //STX
			packet[1]=0x00;  //size
			packet[2]=0x02;  //size
			packet[3]=0xAC;  //Command ID
			packet[4]=0x03;  //ETX
			for(int i=1;i<5;i++)
				packet[5]^=packet[i];

			gCom.SendData( packet,6);

			ExceptionReceiveData();
			PrintLog("ScanCancel >> cancel\n");
			return 1;
		
		}else if (g_ScanKind == 3)
		{
			sendCmd(1);
			PrintLog("ScanCancel >> cancel\n");
			nRet=1;
		}
		else if (g_ScanKind == 4)
		{
			// Scanner Code send
			memset(packet, 0x00, 6);
			packet[0] = 0x02;  //STX
			packet[1] = 0x00;  //size
			packet[2] = 0x02;  //size
			packet[3] = 0xAC;  //Command ID
			packet[4] = 0x03;  //ETX
			for (int i = 1; i<5; i++)
				packet[5] ^= packet[i];

			PassScan_SendData(packet, 6);

			PrintLog("ScanCancel >> cancel\n");
			return 1;
		}
	}

	PrintLog("ScanCancel >> no action cancel\n");
	return 0;
}

extern "C" __declspec(dllexport) int __stdcall ReceiveData(int TimeOut)
{
	PrintLog("ReceiveData >> start\n");
	int nRetry = 0;
	int nRet = 0;
	int nReturn = 0;

	switch(g_ScanKind)
	{
	case 0:
	case 3:
		while(1)
		{
			g_MRZ1.Empty();
			g_MRZ2.Empty();

			nRet = ReceiveCommData();
			if(nRet == 1)
			{
				/////////////////////////////////////
				// 정상 데이터 수신
				/////////////////////////////////////
				nReturn = 1;
				ExceptionReceiveData();
				break;
			}else if(nRet == -1)
			{
				/////////////////////////////////////
				// 오류 데이터 수신
				/////////////////////////////////////
				Sleep(100);

				ExceptionReceiveData();

				nReturn = -1;
				PrintLog("ReceiveData >> READ ERROR return :%d\n",nReturn);	
				return nReturn;
			}

			if(nRetry >= (TimeOut*10))
			{
				ScanCancel();
				Sleep(500);//Timteout 시 Sleep 100-> 500추가 
				ExceptionReceiveData();
				nReturn = 0;
				PrintLog("ReceiveData >> READ TIME OUT \n");	
				m_bTimeout = true;
				break;
			}
			if(nRetry %5 == 0)
				PrintLog("ReceiveData >> Waing Retry Count:%d\n",nRetry);	
			Sleep(100);
			nRetry++;
		}
		break;

	case 1:
		while(1)
		{
			g_MRZ1.Empty();
			g_MRZ2.Empty();

			nRet = ReceiveCommData();
			if(nRet == 1)
			{
				/////////////////////////////////////
				// 정상 데이터 수신
				/////////////////////////////////////
				nReturn = 1;
				break;
			}else if(nRet < 0)
			{
				/////////////////////////////////////
				// 오류 데이터 수신
				/////////////////////////////////////
				Sleep(100);

				ExceptionReceiveData();

				nReturn = nRet;						// 원래의 에러 코드를 가지고 감. (전용 에러)
				PrintLog("ReceiveData >> return :%d\n", nReturn);	
				return nReturn;
			}

			if(nRetry >= (TimeOut*10))
			{
				ScanCancel();
				Sleep(100);
				ExceptionReceiveData();
				nReturn = 0;
				break;
			}

			//Sleep(100);
			nRetry++;
		}

		break;

	case 2:
		{
			g_MRZ1.Empty();
			g_MRZ2.Empty();
			char    MRZ1[45];
			char    MRZ2[45];
			memset(MRZ1,	0x00,	45);
			memset(MRZ2,	0x00,	45);

			gDawin.GetMRZ1(MRZ1);
			gDawin.GetMRZ2(MRZ2);
			g_MRZ1 = (CString)MRZ1;
			g_MRZ2 = (CString)MRZ2;

			//PrintLog("ReceiveData Dawin>> mrz1 :%s\n", g_MRZ1);	
			//PrintLog("ReceiveData Dawin>> mrz2 :%s\n", g_MRZ2);	

			memset(MRZ1,	0x00,	45);
			memset(MRZ2,	0x00,	45);
			ZeroMemory(&MRZ1,45);
			ZeroMemory(&MRZ2,45);

			PrintLog("ReceiveData >> return 1\n");	
			return 1;
		}
		break;

	case 4: 
		{		
			BYTE PassportData[89];
			unsigned int nDataSize = 0;
			nDataSize = ::PassScan_Read_Raw_GTF(PassportData, 89, 30 - 3/*Time Out Setting*/);
			PrintLog("Scan >> send ok\n");

			if (nDataSize == 1460) {
				// TIME OUT
				PrintLog("ReceiveData >> return :%d\n", 0);
				PrintLog("BTC Passport Reader : TimeOut");

				BYTE Command_Init[] = { 0xE4, 0x77, 0x10, 0x02, 0x00, 0x00 };	//Error Buzzer CMD

				PassScan_SendData(Command_Init, 6);
			
				return 0;
			}
			else if (nDataSize == 0){
				// Read Failed
				PrintLog("ReceiveData >> return :%d\n", -1);
				PrintLog("BTC Passport Reader : Read Failed");

				
				if (b_ImageSendOn == 1) {
					Sleep(500);
				}
				else if (b_enabledLog) {
					Sleep(300);
				}
				
				return -1;
			}else {
				// Read Success
				g_MRZ1.Empty();
				g_MRZ2.Empty();

				char    MRZ1[45];
				char    MRZ2[45];

				for (int i = 0; i < 44; i++) {
					MRZ1[i] = PassportData[i];
					MRZ2[i] = PassportData[i + 44];
				}
				MRZ1[44] = '\0';
				MRZ2[44] = '\0';

				g_MRZ1 = (CString)MRZ1;
				g_MRZ2 = (CString)MRZ2;

				memset(MRZ1, 0x00, 45);
				memset(MRZ2, 0x00, 45);
				ZeroMemory(&MRZ1, 45);
				ZeroMemory(&MRZ2, 45);

				PrintLog("ReceiveData >> return %d\n", 1);
				PrintLog("BTC Passport Reader : Read success");
				return 1;
			}
		}
		break;
	}

	PrintLog("ReceiveData >> return :%d\n",nReturn);	
	return nReturn;
}


extern "C" __declspec(dllexport) int __stdcall GetMRZ1(char *refMRZ1)
{
	PrintLog("GetMRZ1 >> start\n");
	char MRZ1[44+1];

	// DAWIN의 경우, 데이터 취득이 불가(공백)
	/////////////////////////////////////////////

	memset(MRZ1,	0x00,	sizeof(MRZ1));
	memcpy(MRZ1, (LPSTR)(LPCTSTR)g_MRZ1,		g_MRZ1.GetLength());

	memcpy(refMRZ1,	MRZ1,	g_MRZ1.GetLength());
	PrintLog("GetMRZ1 >> end\n");
	return 1;

}

extern "C" __declspec(dllexport) int __stdcall GetMRZ2(char *refMRZ2)
{
	PrintLog("GetMRZ2 >> start\n");
	char MRZ2[44+1];

	// DAWIN의 경우, 데이터 취득이 불가(공백)
	/////////////////////////////////////////////

	memset(MRZ2,	0x00,	sizeof(MRZ2));
	memcpy(MRZ2, (LPSTR)(LPCTSTR)g_MRZ2,		g_MRZ2.GetLength());

	memcpy(refMRZ2,	MRZ2,	g_MRZ2.GetLength());
	PrintLog("GetMRZ2 >> end\n");
	return 1;
}
extern "C" __declspec(dllexport) int __stdcall GetMRZ3(char *refMRZ3)
{
	PrintLog("GetMRZ3 >> start\n");
	char MRZ3[44+1];

	// DAWIN의 경우, 데이터 취득이 불가(공백)
	/////////////////////////////////////////////

	memset(MRZ3,	0x00,	sizeof(MRZ3));
	memcpy(MRZ3, (LPSTR)(LPCTSTR)g_MRZ3,		g_MRZ3.GetLength());

	memcpy(refMRZ3,	MRZ3,	g_MRZ3.GetLength());
	PrintLog("GetMRZ3 >> end\n");
	return 1;
}

extern "C" __declspec(dllexport) int __stdcall Clear()
{
	PrintLog("Clear >> start\n");
	g_MRZ1.Empty();
	g_MRZ2.Empty();

	//2016.12.26 disconnect 막음
//	if(g_ScanKind == 2)
//	{
//		gDawin.DisConnect();
//		g_bCommOpened = FALSE;
//	}
	PrintLog("Clear >> end\n");
	return 1;
}

extern "C" __declspec(dllexport) int __stdcall ClosePort()
{
	PrintLog("ClosePort >> start\n");
	if(g_bCommOpened == FALSE)
	{
		PrintLog("ClosePort >> already close\n");
		return 1;
	}

	switch(g_ScanKind)
	{
	case 0:
	case 3:
		gCom.ClosePort();
		gCom.Initialize();
		g_bCommOpened = FALSE;
		if(g_ScanKind == 3)
			Sleep(100);

		//Timeout 시 Sleep 처리
		if(g_ScanKind == 0 && m_bTimeout)
		{
			PrintLog("ClosePort >> GTF ps-01 Close Sleep(1000) \n");
			Sleep(1000);
			m_bTimeout = false;//변수 초기화
			ExceptionReceiveData(500);
		}
		break;
	case 1:
		gWiseCom.Close(&rs232);
		g_bCommOpened = FALSE;
		break;

	case 2:
		g_MRZ1.Empty();
		g_MRZ2.Empty();
		//2016.12.26 disconnect 추가
		gDawin.DisConnect();
		g_bCommOpened = FALSE;
		break;

	case 4:
		if (b_ImageSendOn == 1) {
			
			PassScan_Close();	// 이미지 전송 콜백함수 완료 되면 Close
			g_bCommOpened = FALSE;
		}
		else {
			PassScan_Close();
			g_bCommOpened = FALSE;
		}

		break;
	}
	

	::DeleteCriticalSection(&g_cx);
	
	PrintLog("ClosePort >> close success\n");
	return 1;
}

extern "C" __declspec(dllexport) int __stdcall GetPassportInfo(char *refPassInfo)
{
	PrintLog("GetPassportInfo >> start\n");
	char PassportInfo[65+1];

	memset(PassportInfo,	0x00,	sizeof(PassportInfo));
	memset(PassportInfo,	0x20,	65);


	switch(g_ScanKind)
	{
	case 0:
	case 1:
	case 4: //BTC PassportReader도 GTF와 동일 한 방식으로 g_MRZ1 g_MRZ2에 Data 저장 했음
	case 3: //OKPOS 여권스캐너 추가
		if(g_MRZ1.GetLength() !=0 && g_MRZ2.GetLength() != 0 && g_MRZ3.GetLength() !=0 )
		{
		}
		else if(g_MRZ1.GetLength() !=0 && g_MRZ2.GetLength() != 0)
		{
			CString tmpName = g_MRZ1.Mid(5, g_MRZ1.GetLength());
			int tmpPos = tmpName.Find("<<", 0);
			CString firstName = tmpName.Mid(0, tmpPos) ;
			CString lastName = tmpName.Mid(tmpPos, tmpName.GetLength());
			lastName.Replace("<", "");

			CString fullName;
			fullName.Format(_T("%s %s"), (LPCTSTR)firstName, (LPCTSTR)lastName);

			CString passportNo = g_MRZ2.Mid(0, 9);
			passportNo.Replace("<", "");

			CString country = g_MRZ2.Mid(10, 3);
			country.Replace("<", "");

			CString dateOfbirth = g_MRZ2.Mid(13, 6);
			dateOfbirth.Replace("<", "");

			CString gender = g_MRZ2.Mid(20, 1);
			gender.Replace("<", "");

			CString expiredate = g_MRZ2.Mid(21, 6);
			expiredate.Replace("<", "");

			memcpy(PassportInfo,	(LPCTSTR)fullName.GetBuffer(0), fullName.GetLength());
			memcpy(PassportInfo+40,	(LPCTSTR)passportNo.GetBuffer(0), passportNo.GetLength());
			memcpy(PassportInfo+49,	(LPCTSTR)country.GetBuffer(0), country.GetLength());
			memcpy(PassportInfo+52,	(LPCTSTR)gender.GetBuffer(0), gender.GetLength());
			memcpy(PassportInfo+53,	(LPCTSTR)dateOfbirth.GetBuffer(0), dateOfbirth.GetLength());
			memcpy(PassportInfo+59,	(LPCTSTR)expiredate.GetBuffer(0), expiredate.GetLength());


			CString strTempLog ;
			strTempLog.Format("data :[%s] , Len:%d\n" , PassportInfo, strlen(PassportInfo));			

			PrintLog(strTempLog);//여권정보 로그 삭제

			PrintLog("Passport Out Data Setting before\n");
			memcpy(refPassInfo,	PassportInfo,	strlen(PassportInfo));
			//CString strTempLog ;
			//strTempLog.Format("data :[%s] , Len:%d" , refPassInfo, strlen(refPassInfo));			
			//AfxMessageBox(strTempLog);
			PrintLog("Passport Out Data Setting after\n");
			PrintLog("GetPassportInfo >> data return\n");
			return 1;
		}else{
			PrintLog("GetPassportInfo >> Empty data\n");
			return 0;
		}

		break;

	case 2:
		{
			gDawin.GetData(refPassInfo);
			PrintLog("GetPassportInfo >> data return\n");
			return 1;
		}
		break;
	}

	return 1;
}

extern "C" __declspec(dllexport) int __stdcall CheckReceiveData()
{
	int nRet = 0;
	int nReturn = 0;

	PrintLog("CheckReceiveData >> start\n");
	nRet = ReceiveCommData();
	if(nRet == 1){
		/////////////////////////////////////
		// 정상 데이터 수신
		/////////////////////////////////////
		nReturn = 1;
	}else if(nRet == -1){
		/////////////////////////////////////
		// 오류 데이터 수신
		/////////////////////////////////////
		nReturn = -1;
	}else if(nRet == 0){
		/////////////////////////////////////
		// 데이터 미수신
		/////////////////////////////////////
		nReturn = 0;
	}

	PrintLog("CheckReceiveData >> return value:%d\n", nReturn);
	return nReturn;
}

extern "C" __declspec(dllexport) int __stdcall IsOpen()
{
	int nRet = 0;
	if(g_bCommOpened)
		nRet = 1;
	return nRet;
}

extern "C" __declspec(dllexport) int __stdcall SetPassportScanType(int nType)			//2017.05.31 추가
{
	int nRet = 1;
	g_ScanKind = nType;
	n_SelectScanKind = nType;
	return nRet;
}

extern "C" __declspec(dllexport) int __stdcall SetSavePath(char *strFilePath)			//2017.05.31 추가
{
	int nRet = 1;
	if(g_ScanKind == 2)
	{
		gDawin.SetSavePath(strFilePath);
	}
	return nRet;
}

int Set_ScannerKind(void)
{
	CString strFileName;
//	strFileName = _T("\\GTF_SET.ini");
	strFileName = _T("C:\\GTF_PASSPORT\\GTF_SET.ini");

	int nRet;

//	TCHAR szCurPath[MAX_PATH];
//	memset(szCurPath, 0x00, sizeof(szCurPath));

//	GetCurrentDirectory(MAX_PATH, szCurPath);
//	strncat(szCurPath,  (LPCSTR)strFileName , MAX_PATH);

//	nRet = GetPrivateProfileInt ("ENV", "SCANNER", 0, szCurPath);			// 스캐너 종류
	nRet = GetPrivateProfileInt ("ENV", "SCANNER", 0, strFileName);			// 스캐너 종류

	n_fixPort = GetPrivateProfileInt ("ENV", "PORT_NO_FIX", -1, strFileName);// 강제설정 스캐너 포트 

	strFileName ="";

	return nRet;
}
//2017.01.03 USB 타입으로 연결된 경우 registry 에서 먼저 찾아본다.
//int AutoDetect_usb(int nType)
std::vector<int> AutoDetect_usb(int nType)
{
	std::vector<int> v;
	int iPort = -1;

	try{
		HKEY hKey;  
    
		//https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724895(v=vs.85).aspx  
		//오픈할 레지스터 키에 대한 기본키 이름  
		//오픈할 레지스터 서브키 이름  
		//레지스터키에 대한 핸들  
		RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), &hKey);  
    
		TCHAR szData[20], szName[100];  
		CString strName ;
		CString strPort ;
		DWORD index = 0, dwSize=100, dwSize2 = 20, dwType = REG_SZ;  
		//CString serialPort ="";
		//serialPort.ResetContent();  
		memset(szData, 0x00, sizeof(szData));  
		memset(szName, 0x00, sizeof(szName));  
    
    
		//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724865(v=vs.85).aspx  
		//hKey - 레지스터키 핸들  
		//index - 값을 가져올 인덱스.. 다수의 값이 있을 경우 필요  
		//szName - 항목값이 저장될 배열  
		//dwSize - 배열의 크기  
		while (ERROR_SUCCESS == RegEnumValue(hKey, index, szName, &dwSize, NULL, NULL, NULL, NULL))  
		{  
			index++; 
			strName.Empty();
			strPort.Empty();
    
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911(v=vs.85).aspx  
			//szName-레지터스터 항목의 이름  
			//dwType-항목의 타입, 여기에서는 널로 끝나는 문자열  
			//szData-항목값이 저장될 배열  
			//dwSize2-배열의 크기  
			RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE)szData, &dwSize2);  
			//serialPort.AddString(CString(szData));  
			//serialPort.Format("%s \r\n PORT:%s NAME:%s", serialPort, CString(szData), CString(szName));
			
			strName = CString(szName);
			strPort = CString(szData);

			memset(szData, 0x00, sizeof(szData));  
			memset(szName, 0x00, sizeof(szName));  
			dwSize = 100;  
			dwSize2 = 20;  
			if(nType == 0 && strName.Find("Silabser") >=0) //구형 GTF 여권스캐너
			{
				iPort = _ttoi(strPort.Mid(3));
				g_nBaudRate = 115200;
				v.push_back(iPort);
				//break;
			}
			else if(nType == 3 && strName.Find("USBSER0") >=0) // OKPOS TYPE 체크
			{
				/*현재 미사용 드라이버 체크 차단 
				iPort = _ttoi(strPort.Mid(3));
				iPort += 1000;
				g_nBaudRate = 115200;
				//g_nBaudRate = 9600;//2017.12.24 변경
				v.push_back(iPort);
				//break;
				*/
			}else if(nType ==3 && strName.Find("VCP") >=0 ) //OKPOS TYPE 체크. 타 여권스캐너 못찾은 경우..
			{
				iPort = _ttoi(strPort.Mid(3));
				iPort += 2000;
				g_nBaudRate = 9600;
				v.push_back(iPort);
				//break;
			}
			else if (nType == 4 && strName.Find("NLSCOM") >= 0) //BTCSECU 여권 스캐너 타입 추가
			{
				iPort = _ttoi(strPort.Mid(3));
				v.push_back(iPort);
				//break;
			}
		}  
		strName.Empty();
		strPort.Empty();
		RegCloseKey(hKey);  
	}catch(...)
	{
	}

	//return iPort;
	return v;
}

int AutoDetect_usb_type(int nType, int nSelectPort)
{
	int nRetType = -1;

	try{
		HKEY hKey;  
    
		//https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724895(v=vs.85).aspx  
		//오픈할 레지스터 키에 대한 기본키 이름  
		//오픈할 레지스터 서브키 이름  
		//레지스터키에 대한 핸들  
		RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), &hKey);  
    
		TCHAR szData[20], szName[100];  
		CString strName ;
		CString strPort ;
		DWORD index = 0, dwSize=100, dwSize2 = 20, dwType = REG_SZ;  
		//CString serialPort ="";
		//serialPort.ResetContent();  
		memset(szData, 0x00, sizeof(szData));  
		memset(szName, 0x00, sizeof(szName));  
    
    
		//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724865(v=vs.85).aspx  
		//hKey - 레지스터키 핸들  
		//index - 값을 가져올 인덱스.. 다수의 값이 있을 경우 필요  
		//szName - 항목값이 저장될 배열  
		//dwSize - 배열의 크기  
		while (ERROR_SUCCESS == RegEnumValue(hKey, index, szName, &dwSize, NULL, NULL, NULL, NULL))  
		{  
			index++; 
			strName.Empty();
			strPort.Empty();
    
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911(v=vs.85).aspx  
			//szName-레지터스터 항목의 이름  
			//dwType-항목의 타입, 여기에서는 널로 끝나는 문자열  
			//szData-항목값이 저장될 배열  
			//dwSize2-배열의 크기  
			RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE)szData, &dwSize2);  
			//serialPort.AddString(CString(szData));  
			//serialPort.Format("%s \r\n PORT:%s NAME:%s", serialPort, CString(szData), CString(szName));
			
			strName = CString(szName);
			strPort = CString(szData);

			memset(szData, 0x00, sizeof(szData));  
			memset(szName, 0x00, sizeof(szName));  
			dwSize = 100;  
			dwSize2 = 20;  
			if(nType == 0 && strName.Find("Silabser") >=0) //구형 GTF 여권스캐너
			{
				nRetType = 0;
				g_nBaudRate = 115200;
				break;
			}
			else if(nType == 3 && strName.Find("USBSER000") >=0) // OKPOS TYPE 체크
			{
				/*
				//설정된 포트와 다르면 continue
				if(nSelectPort != _ttoi(strPort.Mid(3)))
				{
					continue;
				}
				nRetType = 3;
				g_nBaudRate = 115200;
				break;
				*/
			}else if(nType ==3 && strName.Find("VCP") >=0 ) //OKPOS TYPE 체크. 타 여권스캐너 못찾은 경우..
			{
				//설정된 포트와 다르면 continue
				if(nSelectPort != _ttoi(strPort.Mid(3)))
				{
					continue;
				}
				nRetType = 3;
				g_nBaudRate = 9600;
				break;
			}
		}  
		strName.Empty();
		strPort.Empty();
		RegCloseKey(hKey);  
	}catch(...)
	{
	}

	return nRetType;
}
int sendCmd(int nType)
{
	int nRet = 0;
	if(g_ScanKind==3)			// OKPOS 여권스캐너에서만 동작
	{
		//OKPOS Cancel 명령어
		if(nType == 0 || nType == 1)
		{
			if( g_bCommOpened )
			{
				unsigned char packet[4];
				// Scanner Code send
				memset(packet,0x00,4); // 'C'
				packet[0]=0x43;  
				packet[1]=0x00;  
				packet[2]=0x01;  
				if(nType == 0 )
				{
					packet[4]=0x00;  
				}
				else if(nType == 1 )
				{
					packet[4]=0x01;  
				}
				PrintLog("sendCmd >> send before \n");
				gCom.SendData( packet,4);
				PrintLog("sendCmd >> send after \n");
				Sleep(100);
				ExceptionReceiveData();//버퍼 Clear 
				nRet=1;
			}
		}
	}
	return nRet ;
}

void GetCurDtTm(char *targetbuf, int type)
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

void traceDebug(char *szFormat, ...)
{
	/*
	char s[2048];
	va_list vlMarker;

	va_start(vlMarker, szFormat);
	vsprintf(s, szFormat, vlMarker);
	va_end(vlMarker);

	OutputDebugString((LPCWSTR)s);
	*/
}

int PrintLog(const char *fmt, ...)
{
	//2017.03.20 로그출력 막음
	//return 0;

	va_list	ap;
	char curdttm[32];
	FILE *fp;
	char logfilename[100] = {0x00 , };
		
	memset(curdttm, 0x00, sizeof(curdttm)); 
	GetCurDtTm(curdttm, YYYYMMDD);
	_mkdir("..\\gtf_log");

	//sprintf(logfilename, "c:\\SMART_POS\\log\\gtf%s.log", curdttm);
	sprintf(logfilename, "c:\\GTF_PASSPORT\\log\\gtf_pass_%s.log", curdttm);
	
	fp = fopen(logfilename, "a+");
	if(fp == NULL) return -1;
		
	memset(curdttm, 0x00, sizeof(curdttm)); GetCurDtTm(curdttm, YYYY_MM_DD_hh_mm_ss_SSS);
	fprintf(fp, "[%s][%d] ▷ ", curdttm, getpid());
		
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
		
	fflush(fp);
	fclose(fp);
	return TRUE;
}

int WriteLog(const char *fmt, int len)
{
	char curdttm[32];
	FILE *fp;
	char logfilename[100] = {0x00 , };

	memset(curdttm, 0x00, sizeof(curdttm)); 
	GetCurDtTm(curdttm, YYYYMMDD);
	sprintf(logfilename, "c:\\SMART_POS\\log\\gtf%s.log", curdttm);

	_mkdir("gtf_log");
	fp = fopen(logfilename, "a+");
	if(fp == NULL) return -1;

	memset(curdttm, 0x00, sizeof(curdttm)); GetCurDtTm(curdttm, YYYY_MM_DD_hh_mm_ss_SSS);
	fprintf(fp, "[%s][%d] ▷ \n", curdttm, getpid());
	
	fwrite(fmt, len, 1, fp);

	fflush(fp);
	fclose(fp);

	return TRUE;
}

int StringFind(char *buf, int chk, int cnt)
{
	int i=0, tmp=0, cntchk=0;

	for(i=0;i<=(int)strlen(buf);i++)
	{
		tmp = (int)buf[i];

		if(tmp == chk)
		{
			cntchk ++;
			if(cntchk == cnt) return i;
		}
	}
	return -1;
}
// return 0 미수정 ,1 수정 ,-1 오류
int fixMrzData(const char *mrz1 , const char *mrz2 , const char *mrz3)
{
	int nRet = 0;
	//CString strTemp = "";
	CString strMrz1 = "";
	CString strMrz2 = "";
	CString strMrz3 = "";
	CString strPassportNo ;
	CString strCountry ;
	CString strDateOfbirth ;
	CString strExpiredate ;
	CString strGender ;

	if(strlen(mrz1) > 0 )
	{
		strMrz1 = (CString)mrz1;
	}

	if(strlen(mrz2) > 0 )
	{
		strMrz2 = (CString)mrz2;
	}

	if(strlen(mrz3) > 0 )
	{
		strMrz3 = (CString)mrz3;
	}

	//2Line Mrz
	if( strMrz3.GetLength() <= 0 && strMrz1.GetLength() > 32 && strMrz2.GetLength() > 32)
	{		
		//mrz2 데이터 검증
		//여권번호
		strPassportNo = strMrz2.Mid(0, 9);
		if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(9,1)))) //CheckDigit
		{
			strPassportNo.Replace("O", "0");//영문자 O -> 숫자 0 치환
			if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(9,1)))) //CheckDigit
			{
				nRet = -1;
			}
		}		
		//국가코드
		strCountry = strMrz2.Mid(10, 3);
		strCountry.Replace("0", "O"); //숫자 0 -> 영문자 O 치환
		//생년월일
		strDateOfbirth= strMrz2.Mid(13, 6) ;
		strDateOfbirth.Replace("O", "0"); //영문자 O -> 숫자 0 치환
		if(!checkCRC(strDateOfbirth , _ttoi(strMrz2.Mid(19,1))))
		{
			nRet = -1;
		}
		//성별
		strGender = strMrz2.Mid(20, 1);
		strGender.Replace("0", "O"); //숫자 0 -> 영문자 O 치환		
		
		//만기일
		strExpiredate= strMrz2.Mid(21, 6);
		strExpiredate.Replace("O", "0"); //영문자 O -> 숫자 0 치환
		if(!checkCRC(strExpiredate , _ttoi(strMrz2.Mid(27,1))))
		{
			nRet = -1;
		}		
		//MRZ2 재조립
		if( nRet >= 0 )
		{
			strncpy((char * )mrz2, (const char *)strPassportNo,9);
			strncpy((char * )mrz2 +10, (const char *)strCountry,3);
			strncpy((char * )mrz2 +13, (const char *)strDateOfbirth,6);
			strncpy((char * )mrz2 +20, (const char *)strGender,1);
			strncpy((char * )mrz2 +21, (const char *)strExpiredate,6);
		}	
	}
	//3Line Mrz
	else if( strlen(mrz3) > 0 && strlen(mrz1) > 14 && strlen(mrz2) > 14)
	{
		//mrz2 데이터 검증
		//여권번호
		strPassportNo = strMrz2.Mid(18, 11);
		if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(29,1)))) //CheckDigit
		{
			strPassportNo.Replace("O", "0");//영문자 O -> 숫자 0 치환
			if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(29,1)))) //CheckDigit
			{
				nRet = -1;
			}
		}
		//국가코드
		strCountry = strMrz2.Mid(15, 3);
		strCountry.Replace("0", "O"); //숫자 0 -> 영문자 O 치환
		//생년월일
		strDateOfbirth= strMrz2.Mid(0, 6) ;
		strDateOfbirth.Replace("O", "0"); //영문자 O -> 숫자 0 치환
		if(!checkCRC(strDateOfbirth , _ttoi(strMrz2.Mid(6,1))))
		{
			nRet = -1;
		}
		//성별
		strGender = strMrz2.Mid(7, 1);
		strGender.Replace("0", "O"); //숫자 0 -> 영문자 O 치환		
		
		//만기일
		strExpiredate= strMrz2.Mid(8, 6);
		strExpiredate.Replace("O", "0"); //영문자 O -> 숫자 0 치환
		if(!checkCRC(strExpiredate , _ttoi(strMrz2.Mid(14,1))))
		{
			nRet = -1;
		}

		//MRZ2 재조립
		if( nRet >= 0 )
		{
			strncpy((char * )mrz2+18, (const char *)strPassportNo,11);
			strncpy((char * )mrz2 +15, (const char *)strCountry,3);
			strncpy((char * )mrz2 , (const char *)strDateOfbirth,6);
			strncpy((char * )mrz2 + 7, (const char *)strGender,1);
			strncpy((char * )mrz2 + 8, (const char *)strExpiredate,6);
		}	
	}else //오류 케이스
	{
		nRet = -1;
	}

	//메모리 Clear
	strMrz1.Empty();
	strMrz2.Empty();
	strMrz3.Empty();
	strPassportNo.Empty();
	strCountry.Empty();
	strDateOfbirth.Empty();
	strExpiredate.Empty();

	return nRet;
}
BOOL checkCRC(const char* strData , int nCRC)
{
	BOOL bRet = TRUE;
	int crc_Val[3]= {7,3,1};
	int tmp =0;
	int nSum = 0 ;
		
	for(int i=0;i < (int)strlen(strData);i++)
	{
		tmp = (int)strData[i];
		
		if( tmp == 60) // '<' 는 계산 제외
			continue;

		nSum += ( crc_Val[ i % 3 ] * (tmp > 57  ? tmp - 5 : tmp - 8) ); //숫자는 유지. A는0, B는1 ... 으로 치환하여 check Digit.

	}
	if(nSum % 10 != nCRC)
	{
		bRet = FALSE;
	}

	return bRet;
}
