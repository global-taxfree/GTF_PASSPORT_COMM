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

extern CCommCtrl	gCom;								// GTF�� ����
extern CWS420Ctrl   gWiseCom;							// Wisecude420�� ���
extern CDawinCtrl   gDawin;								// DAWIN�� �����Լ�

extern INT AutoDetect(HWND hWnd, INT iBaudRate );
extern BOOL CheckPort(HWND hWnd, INT iPort, INT iBaudRate );//2017.06.22 �߰�
extern BOOL CheckPort_OKPOS(HWND hWnd, INT iPort, INT iBaudRate );//2017.06.22 �߰�

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
CString g_MRZ3; //2017.05.18 MRZ3 �߰�
int  g_ScanKind = 0;					// ��ĳ�� ���� 0:GTF ����  1:wisecube420  2:DAWIN 3:OKPOS ���ǽ�ĳ��
int  n_SelectScanKind = -1;				// ��ĳ�� ���� 0:GTF ����  1:wisecube420  2:DAWIN 3:OKPOS ���ǽ�ĳ��

int  n_fixPort = -1;					// �������� ��ĳ�� ��Ʈ

CSerial rs232;
CRITICAL_SECTION g_cx;

BOOL m_bTimeout = false;			//Timeout �� ó�� �߰�


///// BTC ���� �߰� //////
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

extern "C" __declspec(dllexport) int __stdcall OpenPort();								// * ��Ʈ�ڵ� ��ĵ�Ͽ� PORT OPEN �Լ���
extern "C" __declspec(dllexport) int __stdcall ClosePort();								// * ��Ʈ CLOSE �Լ�
extern "C" __declspec(dllexport) int __stdcall Scan();									// * ���ǽ�ĵ��� �Լ�
extern "C" __declspec(dllexport) int __stdcall ScanCancel();							// * ���ǽ�ĵ ���� ��� �Լ�
extern "C" __declspec(dllexport) int __stdcall ReceiveData(int TimeOut);				// * ���ǽ�ĵ���� ������ ���� �������Ľ� �Լ�
extern "C" __declspec(dllexport) int __stdcall GetMRZ1(char *refMRZ1);
extern "C" __declspec(dllexport) int __stdcall GetMRZ2(char *refMRZ2);
extern "C" __declspec(dllexport) int __stdcall GetMRZ3(char *refMRZ3);
extern "C" __declspec(dllexport) int __stdcall GetPassportInfo(char *refPassInfo);		// * ������ �Ľ� ���� �԰ݿ� ���缭 ������ �����Լ�
extern "C" __declspec(dllexport) int __stdcall Clear();                         // * ���� ������� ����
extern "C" __declspec(dllexport) int __stdcall CheckReceiveData();              // * ���� �� ������ �������� �Լ�
extern "C" __declspec(dllexport) int __stdcall OpenPortByNum(int PorNum);				// * ��Ʈ��ȣ �����Ͽ� PORT OPEN �Լ���(��)
extern "C" __declspec(dllexport) int __stdcall OpenPortByNumber(int PorNum);			// * ��Ʈ��ȣ �����Ͽ� PORT OPEN �Լ���
extern "C" __declspec(dllexport) int __stdcall IsOpen();						// * PORT OPEN ���� Ȯ�� �Լ�
extern "C" __declspec(dllexport) int __stdcall SetPassportScanType(int nType);			//2017.05.31 �߰�
extern "C" __declspec(dllexport) int __stdcall SetSavePath(char *strFilePath);			//2017.07.19 �߰�

using std::vector;
BOOL CGTF_PASSPORT_COMMApp::InitInstance()
{
	CWinApp::InitInstance();

	g_ScanKind = Set_ScannerKind();							// ��ĳ�� ���� �Ķ����

	return TRUE;
}


extern "C" __declspec(dllexport) int __stdcall OpenPort()
{
	PrintLog("OpenPort >> Open Start\n");
	CString strTempLog ;
	if(n_SelectScanKind <0)
		g_ScanKind = Set_ScannerKind();							// ��ĳ�� ���� �Ķ����
	else
		g_ScanKind = n_SelectScanKind;

	switch(g_ScanKind)
	{
	case 0:
	case 3:
	case 4:
		{
			//2018.02.27 PORT ���� �����ص� ��쿣 ������ ��Ʈ�θ� ���
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
					//gCom.ClosePort();//2018.02.27 ���� �� closeport ����
					g_nPortNum = nSearchPort;
				}else
				{
					PrintLog("OpenPort >> Fix portcheck fail %d \n", nSearchPort);
				}
				SetCursor(hPrvCur);
			}else
			{
				g_nPortNum = 0;
				//USB ���� ���� ã�ƺ�
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
						int nType = nSearchPort/1000; //1000 ���� �������� Type ���� ������
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
							//gCom.ClosePort();//2018.02.27 ���� �� closeport ����
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

			// �ڵ����� port ��ȣ �˻�
			if(g_nPortNum == 0)
			{	
				if(g_ScanKind == 0)
				{
					g_nPortNum = AutoDetect(g_hWnd, g_nBaudRate);
				}
			}
			
			// Open�� ���¿� ���� Port Open ó��
			if( !g_bCommOpened ){
				if(g_nPortNum > 0 && g_nPortNum != -1 )
				{	
					//Sleep(200);//ver.1 sleep �߰� 
					PrintLog("OpenPort >> g_nPortNum:%d \n", g_nPortNum);
					PrintLog("OpenPort >> PortOpen 1 \n");
					if (g_ScanKind == 4) {	// BTC
						if (GetFileAttributes(_T("C:\\GTF_PASSPORT\\log")) == -1) {
							b_enabledLog = false;
						}
						else {
							b_enabledLog = true;
						}
						g_bCommOpened = PassScan_Open_PortNoGTF(g_nPortNum, b_enabledLog);	// BTC Serail Open �Լ�
						b_ImageSendOn = PassScan_ImageSendingStatus_Check();	// Image ���� ���� Check (1: Send On / 2: Send Off)
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
			g_bCommOpened = gDawin.Connect();					// 1:����  0:����
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

	//g_ScanKind = Set_ScannerKind();							// ��ĳ�� ���� �Ķ����

	if(n_SelectScanKind <0)
		g_ScanKind = Set_ScannerKind();							// ��ĳ�� ���� �Ķ����
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
				//bCheck = true;//OKPOS ���ǽ�ĳ���϶� ��ĵ ��� �ٷ� ����
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
				//2018.02.27 ���� �� closeport ����
				//Sleep(200);//ver.1 sleep �߰� 
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
		g_bCommOpened = gDawin.Connect();					// 1:����  0:����
		break;

	case 4:
		if (GetFileAttributes(_T("C:\\GTF_PASSPORT\\log")) == -1) {
			b_enabledLog = false;
		}
		else {
			b_enabledLog = true;
		}
		g_nPortNum = PorNum;
		g_bCommOpened = PassScan_Open_PortNoGTF(g_nPortNum, b_enabledLog);	// BTC Serail Open �Լ�
		b_ImageSendOn = PassScan_ImageSendingStatus_Check();	// Image ���� ���� Check (1: Send On / 2: Send Off)
		
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
			if (g_CommBuf[0] == 0xe4) {		// BTC ���� �������� ���� �� ������ ���� ó��
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
				//�����Ͱ� 7�̸� ����
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
						memset(MRZ3,0x00,45);//2017.05.18 ó��
	
						strncpy(MRZ1, (const char *)&rcv_packet[4],44);
						strncpy(MRZ2, (const char *)&rcv_packet[48],44);
						/*
						int nFixMrz = fixMrzData(MRZ1, MRZ2 , MRZ3);
						if(nFixMrz <0) //2017.05.18 ����
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
				default: //2017.11.16 default �߰�
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
			//2016.12.27 Waiting ó�� ����
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
						ret = gWiseCom.ReadUpto( &rs232, &response[1], sizeof(response) - 2, 100, 0X00 );		// ��� 2����Ʈ�� �ٽ� ����

						if( ret > 0 ) 
						{
							ret = gWiseCom.check_response( response, ret + 1, cmd, dat, sizeof(dat) );

							switch(ret)
							{
								case 0:
									printf( "[ER]:no response\n" );
									break;
								case 1:					// ���� ���� (P)
									printf( "[OK]:%s \nCMD:%s DAT:%s\n",response, cmd, dat );

									if( cmd[0] == 'G' && cmd[1] == 'S' ) {                             // #GS ������ Ȯ��
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

									if( cmd[0] == 'S' && cmd[1] == 'T' ) {                              // Set Status ��� #ST ����� ���ۻ��� ����
										if( dat[0] == 'O' && timer_req == 1 )							// STO : ��ȸ���ۻ���
										{
											timercount = 100;
											timer_req = 0;
										}
										else
											timercount = -1;	// disable timer
									}

									break;
								case 2:					// ���������� (N)
									printf( "[OK]:%s \nCMD:%s ERR:%s\n",response, cmd, dat );
									char strDat[5];
									strDat[4] = 0x00;
									memcpy(strDat, dat, 4);
									nErrNo = atoi(strDat) * -1; 
									return nErrNo;

									break;
								case -1:				// ����
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

						if(response[1] == 'E')			// ���� �ν� ����
						{
							nErrNo = -10;
							break;
						}

						//2017.06.15 size üũ �߰�
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
	case 3: //OKPOS ���ǽ�ĳ��. ��ĵ ��, ���ǽ�ĳ�ʿ� ������ ���� �� �־�� �Ѵ�. Timeout üũ ���� ����
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

			//���Ź��� 3����Ʈ�� ������ ����
			memset(cmd,	0x00,	sizeof(cmd));
			memset(rcv_packet,	0x00,	sizeof(rcv_packet));

			while(run_f)
			{
				//Scan ��� �� ���� ��� �ð� �����Ͽ� nLoop ó��
				if(nLoopCnt> 50) //�ִ� ���ð� 5��
				{
					nErrNo = -1;
					bJobEnd = true;
					PrintLog("ReceiveData >> Result : gtf Timeout\n");
				}
				else
					nLoopCnt++;

				switch(state)
				{
					case 0: //��ȣ���
						cnt = gCom.m_Queue.GetData( nCmdCnt, g_CommBuf );
					
						if ( cnt>0)
						{
							memcpy((char*)&cmd[rcv_len],(const char*)g_CommBuf, sizeof(cmd)); //Ŀ�ǵ� ��ȣ ó��
							memcpy((char*)&rcv_packet[rcv_len],(const char*)g_CommBuf, cnt); //���� �� ������
							nCmdCnt  = nCmdCnt - cnt;
						}
						rcv_len+=cnt;
						if(rcv_len >=3)
						{
							nTotalDataLen = cmd[1] * 256 + cmd[2]; //�ѱ��� ���
							strTemp.Format("total Len:%d\n", nTotalDataLen);
							PrintLog(strTemp);
							state = 1;
						}
						break;
					case 1: //������ ���
						cnt = gCom.m_Queue.GetData( nTotalDataLen, g_CommBuf );
						if(cmd[0] == 'I')
						{
							if(cnt)
							{
								memcpy((char*)&rcv_packet[rcv_len],(const char*)g_CommBuf, cnt); //���� �� ������
								rcv_len+=cnt;
							}								
							
							if( rcv_len  >= nTotalDataLen )
							{
								if(nTotalDataLen < (88))//mrz ���ǵ����� ���� �̸��� ���� return
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
										//strncpy(cTempData, (const char *)&rcv_packet[3],nTotalDataLen-3);//3����Ʈ cmd ���� �ϰ� ����
										//strRes = (CString)cTempData;
										//strRes.TrimRight();
										PrintLog("ReceiveData >> Result : Mrz Data Short, Len :%d\n", nTotalDataLen );
										//memset(cTempData,0x00,500);
										//ZeroMemory(&cTempData,500);
										//strRes.Empty();
									}

								}else //���ǵ����� ���� ������ ��쿡��
								{
									sendCmd(1);//Cancel ��� ȣ��

									PrintLog("ReceiveData >> Result Normal \n" );
									memset(cTempData,0x00,500);
									//strncpy(cTempData, (const char *)&rcv_packet[3],100);//3����Ʈ cmd ���� �ϰ� ����
									strncpy(cTempData, (const char *)&rcv_packet[3],nTotalDataLen );//3����Ʈ cmd ���� �ϰ� ����
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
									memset(MRZ3,0x00,45);//2017.05.18 ó��
									
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

									if(nFixMrz <0) //2017.05.18 ����
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
									strncpy(MRZ1, (const char *)&rcv_packet[3],44);//3����Ʈ cmd ���� �ϰ� ����
									strncpy(MRZ2, (const char *)&rcv_packet[48],44);//1����Ʈ ���� ���� ���� �ϰ� ����
									g_MRZ1 = (CString)MRZ1;
									g_MRZ2 = (CString)MRZ2;
									*/
									
									
									bJobEnd = true;
								}		
							}
						}
						//���°� ó��
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
				//recv Count ����
				//�����ڵ� ó��
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
			gWiseCom.SendCommand( &rs232, _T("STQ") );			// ��ȸ���ۻ��� (������ ���̸� �����͸� �а� ������ �ۺ�, ���� ���� ����)
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

	if( g_ScanKind !=0 && g_ScanKind != 3 && g_ScanKind != 4)			// GTF ���� / OKPOS ���ǽ�ĳ�� ������ ���� ���� (ĵ�� ���)
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
				// ���� ������ ����
				/////////////////////////////////////
				nReturn = 1;
				ExceptionReceiveData();
				break;
			}else if(nRet == -1)
			{
				/////////////////////////////////////
				// ���� ������ ����
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
				Sleep(500);//Timteout �� Sleep 100-> 500�߰� 
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
				// ���� ������ ����
				/////////////////////////////////////
				nReturn = 1;
				break;
			}else if(nRet < 0)
			{
				/////////////////////////////////////
				// ���� ������ ����
				/////////////////////////////////////
				Sleep(100);

				ExceptionReceiveData();

				nReturn = nRet;						// ������ ���� �ڵ带 ������ ��. (���� ����)
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

	// DAWIN�� ���, ������ ����� �Ұ�(����)
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

	// DAWIN�� ���, ������ ����� �Ұ�(����)
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

	// DAWIN�� ���, ������ ����� �Ұ�(����)
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

	//2016.12.26 disconnect ����
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

		//Timeout �� Sleep ó��
		if(g_ScanKind == 0 && m_bTimeout)
		{
			PrintLog("ClosePort >> GTF ps-01 Close Sleep(1000) \n");
			Sleep(1000);
			m_bTimeout = false;//���� �ʱ�ȭ
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
		//2016.12.26 disconnect �߰�
		gDawin.DisConnect();
		g_bCommOpened = FALSE;
		break;

	case 4:
		if (b_ImageSendOn == 1) {
			
			PassScan_Close();	// �̹��� ���� �ݹ��Լ� �Ϸ� �Ǹ� Close
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
	case 4: //BTC PassportReader�� GTF�� ���� �� ������� g_MRZ1 g_MRZ2�� Data ���� ����
	case 3: //OKPOS ���ǽ�ĳ�� �߰�
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

			PrintLog(strTempLog);//�������� �α� ����

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
		// ���� ������ ����
		/////////////////////////////////////
		nReturn = 1;
	}else if(nRet == -1){
		/////////////////////////////////////
		// ���� ������ ����
		/////////////////////////////////////
		nReturn = -1;
	}else if(nRet == 0){
		/////////////////////////////////////
		// ������ �̼���
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

extern "C" __declspec(dllexport) int __stdcall SetPassportScanType(int nType)			//2017.05.31 �߰�
{
	int nRet = 1;
	g_ScanKind = nType;
	n_SelectScanKind = nType;
	return nRet;
}

extern "C" __declspec(dllexport) int __stdcall SetSavePath(char *strFilePath)			//2017.05.31 �߰�
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

//	nRet = GetPrivateProfileInt ("ENV", "SCANNER", 0, szCurPath);			// ��ĳ�� ����
	nRet = GetPrivateProfileInt ("ENV", "SCANNER", 0, strFileName);			// ��ĳ�� ����

	n_fixPort = GetPrivateProfileInt ("ENV", "PORT_NO_FIX", -1, strFileName);// �������� ��ĳ�� ��Ʈ 

	strFileName ="";

	return nRet;
}
//2017.01.03 USB Ÿ������ ����� ��� registry ���� ���� ã�ƺ���.
//int AutoDetect_usb(int nType)
std::vector<int> AutoDetect_usb(int nType)
{
	std::vector<int> v;
	int iPort = -1;

	try{
		HKEY hKey;  
    
		//https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms724895(v=vs.85).aspx  
		//������ �������� Ű�� ���� �⺻Ű �̸�  
		//������ �������� ����Ű �̸�  
		//��������Ű�� ���� �ڵ�  
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
		//hKey - ��������Ű �ڵ�  
		//index - ���� ������ �ε���.. �ټ��� ���� ���� ��� �ʿ�  
		//szName - �׸��� ����� �迭  
		//dwSize - �迭�� ũ��  
		while (ERROR_SUCCESS == RegEnumValue(hKey, index, szName, &dwSize, NULL, NULL, NULL, NULL))  
		{  
			index++; 
			strName.Empty();
			strPort.Empty();
    
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911(v=vs.85).aspx  
			//szName-�����ͽ��� �׸��� �̸�  
			//dwType-�׸��� Ÿ��, ���⿡���� �η� ������ ���ڿ�  
			//szData-�׸��� ����� �迭  
			//dwSize2-�迭�� ũ��  
			RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE)szData, &dwSize2);  
			//serialPort.AddString(CString(szData));  
			//serialPort.Format("%s \r\n PORT:%s NAME:%s", serialPort, CString(szData), CString(szName));
			
			strName = CString(szName);
			strPort = CString(szData);

			memset(szData, 0x00, sizeof(szData));  
			memset(szName, 0x00, sizeof(szName));  
			dwSize = 100;  
			dwSize2 = 20;  
			if(nType == 0 && strName.Find("Silabser") >=0) //���� GTF ���ǽ�ĳ��
			{
				iPort = _ttoi(strPort.Mid(3));
				g_nBaudRate = 115200;
				v.push_back(iPort);
				//break;
			}
			else if(nType == 3 && strName.Find("USBSER0") >=0) // OKPOS TYPE üũ
			{
				/*���� �̻�� ����̹� üũ ���� 
				iPort = _ttoi(strPort.Mid(3));
				iPort += 1000;
				g_nBaudRate = 115200;
				//g_nBaudRate = 9600;//2017.12.24 ����
				v.push_back(iPort);
				//break;
				*/
			}else if(nType ==3 && strName.Find("VCP") >=0 ) //OKPOS TYPE üũ. Ÿ ���ǽ�ĳ�� ��ã�� ���..
			{
				iPort = _ttoi(strPort.Mid(3));
				iPort += 2000;
				g_nBaudRate = 9600;
				v.push_back(iPort);
				//break;
			}
			else if (nType == 4 && strName.Find("NLSCOM") >= 0) //BTCSECU ���� ��ĳ�� Ÿ�� �߰�
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
		//������ �������� Ű�� ���� �⺻Ű �̸�  
		//������ �������� ����Ű �̸�  
		//��������Ű�� ���� �ڵ�  
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
		//hKey - ��������Ű �ڵ�  
		//index - ���� ������ �ε���.. �ټ��� ���� ���� ��� �ʿ�  
		//szName - �׸��� ����� �迭  
		//dwSize - �迭�� ũ��  
		while (ERROR_SUCCESS == RegEnumValue(hKey, index, szName, &dwSize, NULL, NULL, NULL, NULL))  
		{  
			index++; 
			strName.Empty();
			strPort.Empty();
    
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911(v=vs.85).aspx  
			//szName-�����ͽ��� �׸��� �̸�  
			//dwType-�׸��� Ÿ��, ���⿡���� �η� ������ ���ڿ�  
			//szData-�׸��� ����� �迭  
			//dwSize2-�迭�� ũ��  
			RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE)szData, &dwSize2);  
			//serialPort.AddString(CString(szData));  
			//serialPort.Format("%s \r\n PORT:%s NAME:%s", serialPort, CString(szData), CString(szName));
			
			strName = CString(szName);
			strPort = CString(szData);

			memset(szData, 0x00, sizeof(szData));  
			memset(szName, 0x00, sizeof(szName));  
			dwSize = 100;  
			dwSize2 = 20;  
			if(nType == 0 && strName.Find("Silabser") >=0) //���� GTF ���ǽ�ĳ��
			{
				nRetType = 0;
				g_nBaudRate = 115200;
				break;
			}
			else if(nType == 3 && strName.Find("USBSER000") >=0) // OKPOS TYPE üũ
			{
				/*
				//������ ��Ʈ�� �ٸ��� continue
				if(nSelectPort != _ttoi(strPort.Mid(3)))
				{
					continue;
				}
				nRetType = 3;
				g_nBaudRate = 115200;
				break;
				*/
			}else if(nType ==3 && strName.Find("VCP") >=0 ) //OKPOS TYPE üũ. Ÿ ���ǽ�ĳ�� ��ã�� ���..
			{
				//������ ��Ʈ�� �ٸ��� continue
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
	if(g_ScanKind==3)			// OKPOS ���ǽ�ĳ�ʿ����� ����
	{
		//OKPOS Cancel ��ɾ�
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
				ExceptionReceiveData();//���� Clear 
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
	//2017.03.20 �α���� ����
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
	fprintf(fp, "[%s][%d] �� ", curdttm, getpid());
		
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
	fprintf(fp, "[%s][%d] �� \n", curdttm, getpid());
	
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
// return 0 �̼��� ,1 ���� ,-1 ����
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
		//mrz2 ������ ����
		//���ǹ�ȣ
		strPassportNo = strMrz2.Mid(0, 9);
		if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(9,1)))) //CheckDigit
		{
			strPassportNo.Replace("O", "0");//������ O -> ���� 0 ġȯ
			if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(9,1)))) //CheckDigit
			{
				nRet = -1;
			}
		}		
		//�����ڵ�
		strCountry = strMrz2.Mid(10, 3);
		strCountry.Replace("0", "O"); //���� 0 -> ������ O ġȯ
		//�������
		strDateOfbirth= strMrz2.Mid(13, 6) ;
		strDateOfbirth.Replace("O", "0"); //������ O -> ���� 0 ġȯ
		if(!checkCRC(strDateOfbirth , _ttoi(strMrz2.Mid(19,1))))
		{
			nRet = -1;
		}
		//����
		strGender = strMrz2.Mid(20, 1);
		strGender.Replace("0", "O"); //���� 0 -> ������ O ġȯ		
		
		//������
		strExpiredate= strMrz2.Mid(21, 6);
		strExpiredate.Replace("O", "0"); //������ O -> ���� 0 ġȯ
		if(!checkCRC(strExpiredate , _ttoi(strMrz2.Mid(27,1))))
		{
			nRet = -1;
		}		
		//MRZ2 ������
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
		//mrz2 ������ ����
		//���ǹ�ȣ
		strPassportNo = strMrz2.Mid(18, 11);
		if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(29,1)))) //CheckDigit
		{
			strPassportNo.Replace("O", "0");//������ O -> ���� 0 ġȯ
			if(!checkCRC(strPassportNo , _ttoi(strMrz2.Mid(29,1)))) //CheckDigit
			{
				nRet = -1;
			}
		}
		//�����ڵ�
		strCountry = strMrz2.Mid(15, 3);
		strCountry.Replace("0", "O"); //���� 0 -> ������ O ġȯ
		//�������
		strDateOfbirth= strMrz2.Mid(0, 6) ;
		strDateOfbirth.Replace("O", "0"); //������ O -> ���� 0 ġȯ
		if(!checkCRC(strDateOfbirth , _ttoi(strMrz2.Mid(6,1))))
		{
			nRet = -1;
		}
		//����
		strGender = strMrz2.Mid(7, 1);
		strGender.Replace("0", "O"); //���� 0 -> ������ O ġȯ		
		
		//������
		strExpiredate= strMrz2.Mid(8, 6);
		strExpiredate.Replace("O", "0"); //������ O -> ���� 0 ġȯ
		if(!checkCRC(strExpiredate , _ttoi(strMrz2.Mid(14,1))))
		{
			nRet = -1;
		}

		//MRZ2 ������
		if( nRet >= 0 )
		{
			strncpy((char * )mrz2+18, (const char *)strPassportNo,11);
			strncpy((char * )mrz2 +15, (const char *)strCountry,3);
			strncpy((char * )mrz2 , (const char *)strDateOfbirth,6);
			strncpy((char * )mrz2 + 7, (const char *)strGender,1);
			strncpy((char * )mrz2 + 8, (const char *)strExpiredate,6);
		}	
	}else //���� ���̽�
	{
		nRet = -1;
	}

	//�޸� Clear
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
		
		if( tmp == 60) // '<' �� ��� ����
			continue;

		nSum += ( crc_Val[ i % 3 ] * (tmp > 57  ? tmp - 5 : tmp - 8) ); //���ڴ� ����. A��0, B��1 ... ���� ġȯ�Ͽ� check Digit.

	}
	if(nSum % 10 != nCRC)
	{
		bRet = FALSE;
	}

	return bRet;
}
