///////////////////////////////////////////////////////////////////
///                                                             ///
/// PassScan.h                                                  ///
///                                                             ///
/// (c) Copyright BTC SECU Co., Ltd. All rights reserved.       ///
///                                                             ///
///////////////////////////////////////////////////////////////////
/// Rev. 4.0.0.13
/// 2019-09-06

#pragma once

#ifndef _PASS_SCAN_API_H_
#define _PASS_SCAN_API_H_

#ifdef PASS_SCAN_API_EXPORT
#define PASS_SCAN_API	//__declspec(dllexport)
#else
#define PASS_SCAN_API	//__declspec(dllimport) 
#endif
#ifdef __cplusplus
extern "C" {
#endif 

#define MAX_PASSPORT_DATA_LEN	89

	typedef enum enumFailedPassportDataErrorStatus
	{
		CHECKDIGIT_SUCCESS = 0,
		CHECKDIGIT_PASSPORT_NUMBER_ERROR,
		CHECKDIGIT_DATEofBIRTH_ERROR,
		CHECKDIGIT_DATEofEXPIORY_ERROR,
		CHECKDIGIT_OPTIONAL_DATA_ERROR,
		CHECKDIGIT_TOTAL_ERROR,
		LONG_LENGTH_ERROR,
		SHORT_LENGTH_ERROR
	} FailedPassportDataErrorStatus;

    typedef struct _PASSPORT_INFO {
        char cType;						/// Passport type
        char szCountry[4];              /// Issued Country
        char szName[40];        		/// Name (in the format of "Surname, Name")
        char szPassportNo[10];			/// Passport No
        char szNationality[4];			/// Nationality
        unsigned short nBirth_Year;		/// Date of birth - Year
        unsigned short nBirth_Month;	/// Date of birth - Month
        unsigned short nBirth_Day;		/// Date of birth - Day
        char cGender;					/// Gender
        unsigned short nExp_Year;		/// Expiration Date - Year
        unsigned short nExp_Month;		/// Expiration Date - Mont
        unsigned short nExp_Day;		/// Expiration Date - Day
    } PASSPORT_INFO;
    
    typedef void (CALLBACK *PassportInputProc)(PASSPORT_INFO *pPassportInfo, bool bStatus, void *nContext);
    typedef void (CALLBACK *PassportInputProc_Raw)(BYTE *pPaspordRawData, int nDataSize, void *nContext);
	typedef void (CALLBACK *FailedPassportDataInputProc)(BYTE *pFailedPassportData, int nDataSize, int checkDigit, void *nContext);
	typedef void (CALLBACK *QRCodeInputProc)(BYTE *pQrCodeData, int nDataSize, void *nContext);
	// PassportReadInputProc -> 여권 삽입시 올라오는 Data Callback
	typedef void (CALLBACK *PassportReadInputProc)(BYTE *pQrCodeData, int nDataSize, void *nContext);
	typedef void (CALLBACK *FwDownloadProc)(int command, void *nContext);
	typedef void (CALLBACK *UartImageProc)(void *nContext);

    
    /// Enable logging for debugging purpose
    /// 
    /// Parameter
    ///     szLogFilename: Path and filename of the log file or an empty string
    ///                     e.g) d:\test\log_test.txt
    ///                     <note> If an empty string is passed as the parameter, the API creates a logfile automatically for you.
    ///                            In this case, the log file will be created in a sub-foler of the folder where the API file (PassScan.dll) is existing.
    /// Return
    ///     true:   A log file is successfully created as specified
    ///     false:  Failed on creating a log file. Check the GetLastError() for more information.
    /// 
    PASS_SCAN_API
        bool WINAPI PassScan_EnableLog(const char *szLogFilename);
    
    /// Equivalent to the above function except the return value type and log file naming policy. The log file naming is done by the API automatically, 
    /// the same as passing an empty string with PassScan_EnableLog() funtion.
    /// Return
    ///     1: Success 
    ///     0: Failure 
    ///     -1: No Permission. Access Denied.
    ///     -2: No such folder or file
    PASS_SCAN_API
        int WINAPI PassScan_EnableLog_Ex();

    /// Intialize the PassScan API to use.
    /// 
    /// Parameter
    ///     szPortName: Serial port name to open. If this value is NULL, the function seeks entire 
    ///     COM ports available on the system to connect with a passport scanner.  
    /// Return
    ///     true:	The API has been successfully initialized. 
    ///     false:	Initialization failed. Check the GetLastError() for more information.
    /// 
    PASS_SCAN_API
        bool WINAPI PassScan_Open(const char *szPortName = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_OpenGTF(const char *szPortName, bool enableLog);
    
    /// Equivalent to the above function except the return value type. 1: Success, 0: Failure
    PASS_SCAN_API
        int  WINAPI PassScan_Open_Ex(const char *szPortName = NULL);
    
    /// Equivalent to the above function except that this function enables logging by calling PassScan_EnableLog_Ex() function internally.
    PASS_SCAN_API
        int  WINAPI PassScan_Open_Ex_Log(const char *szPortName = NULL);

    /// Intialize the PassScan API to use. Similar function with PassScan_Open() except receiving port number as a parameter.
    ///
    /// Parameter
    ///     nPortNo: Serial port number to open. If this value is -1, the function seeks entire 
    ///     COM ports available on the system to connect with a passport scanner.  
    /// Return
    ///     true:	The API has been successfully initialized. 
    ///     false:	Initialization failed. Check the GetLastError() for more information.
    /// 
    PASS_SCAN_API
        bool WINAPI PassScan_Open_PortNo(int nPortNo = -1);

	PASS_SCAN_API
		bool WINAPI PassScan_Open_PortNoGTF(int nPortNo , bool enableLog);
    
    /// Equivalent to the above function except the return value type. 1: Success, 0: Failure
    PASS_SCAN_API
        int  WINAPI PassScan_Open_PortNo_Ex(int nPortNo = -1);
    
    /// Close the PassScan API.
    ///
    /// Parameter
    ///     NONE
    /// Return
    ///     None
    ///
    PASS_SCAN_API
        void WINAPI PassScan_Close(void);
    
    
    /// Read synchronously a passport information from the passport scanner. 
    ///
    ///     Parameter
    ///         pPassportInfo	:   Address of a memory buffer of PASSPORT_INFO type where the passport data will be stored. 
    ///                             The buffer must be allocated prior to call this function.
    ///                             <NOTE> This function is a blocking function which does not return until it meets some specific conditions.
    ///                             Use PassScan_ReadOverlapped() function instead in case of asynchronous reading is required.
    ///         nTimeOut:           User input standby time in second; maximum is 60
    ///     Return
    ///                             true: Successfully read a passport information
    ///                             false: Failed to read a passport information; Call GetLastError() function for detail information of the error.
    ///
    PASS_SCAN_API
        bool WINAPI PassScan_Read(PASSPORT_INFO *pPassportInfo, int nTimeOut = 60);
    
    
    /// Read synchronously a passport information from the passport scanner. 
    ///
    ///     Parameter
    ///         pPassportDataBuffer:Address of a memory buffer where the raw data of the passport information will be stored. 
    ///                             The application must allocat this buffer properly before calling the function.
    ///         nBufferSize:        Size of the passport data buffer; if this size is less than actual passport data, the data will be truncated.
    ///             
    ///                             <NOTE> This function is a blocking function which does not return until it meets some specific conditions.
    ///                             Use PassScan_ReadOverlapped() function instead in case of asynchronous reading is required.
    ///         nTimeOut:           User input standby time in second; maximum is 60
    ///     Return
    ///                             0: Failed to read a passport information; Call GetLastError() function for detail information of the error.
    ///                             otherwise: Total passport data size returned through 'pPassportDataBuffer'
    ///
    PASS_SCAN_API
        unsigned int WINAPI PassScan_Read_Raw(BYTE *pPassportDataBuffer, unsigned int nBufferSize = MAX_PASSPORT_DATA_LEN, int nTimeOut = 60);
   
    
    /// Same function with above PassScan_Read() except receiving the password information through the multiple parameters
    ///
    ///		Parameter				: See the function delcaration
    ///		
    ///		Return
    ///								1: Successfully read a passport information
    ///								0: Failed to read a passport information; Call GetLastError() function for detail information of the error.
    ///
    PASS_SCAN_API
        unsigned int WINAPI PassScan_Read_Ex(
                                char *cType,			// Passport type
                                char *szCountry,		// Issued Country
                                char *szName,			// Name (in the format of "Surname, Name")
                                char *szNationality,	// Nationality
                                char *szPassportNo,		// Passport No
                                char *nBirth_Year,		// Date of birth - Year
                                char *nBirth_Month,		// Date of birth - Month
                                char *nBirth_Day,		// Date of birth - Day
                                char *cGender,			// Gender
                                char *nExp_Year,		// Expiration Date - Year
                                char *nExp_Month,		// Expiration Date - Mont
                                char *nExp_Day,			// Expiration Date - Day
                                int	 nTimeOut = 60
                            );
    
    /// Read asynchronously a passport information from the passport scanner. 
    ///
    ///     Parameter
    ///     OnPassportInput:    Callback function that will be called when a passport data is available.
    ///     nTimeOut:           User input standby time in second; maximum is 60
    ///     nContext
    ///     Return
    ///                         true: Successfully read a passport information
    ///                         false: Failed to read a passport information; Call GetLastError() function for detail information of the error.
    ///
    PASS_SCAN_API
    bool WINAPI PassScan_ReadOverlapped(PassportInputProc OnPassportInput, int nTimeOut = 60, void *nContext = NULL);
    
    
    /// Read asynchronously a passport information from the passport scanner. 
    ///     
    ///     Parameter
    ///         OnPassportInputRaw:	Callback function that will be called when a passport data is available.
    ///         nTimeOut:           User input standby time in second; maximum is 60
    ///         nContext
    ///     Return
    ///                             true: Successfully read a passport information
    ///                             false: Failed to read a passport information; Call GetLastError() function for detail information of the error.
    ///                             
    ///                             In comparion with PassScan_ReadOverlapped() function, this function returns the received passport data 
    ///                             in a raw data format. An application will need to decode the received data separately.
    ///
    PASS_SCAN_API
    bool WINAPI PassScan_ReadOverlapped_Raw(PassportInputProc_Raw OnPassportInputRaw, int nTimeOut = 60, void *nContext = NULL);
    
    /// Send a request for an asynchronous read 
    ///     
    ///     Parameter
    ///         nTimeOut:           User input standby time in second; maximum is 60
    ///     Return
    ///                             0: If any kind of error. Call GetLastError() function for detail information of the error.
    ///                             1: The request has been successully sent. Use PassScan_Async_Read() function to retrieve the respoinse of the request.
    ///                     
    ///                             This function only sends a request to read input data to the device, and returns immediately.
    ///                             PassScan_Async_Read() function is used to retrieve the requested input data or to check if data is available.
    PASS_SCAN_API
        unsigned int WINAPI PassScan_Read_Request(int nTimeOut = 60);
    
    /// Checks and retrieves input data after a PassScan_Read_Request() function call.
    /// <NOTE> To retrieve input data properly, PassScan_Read_Request() must be called beforehand.
    ///
    ///     Parameter               : See in the function delcaration
    ///                     
    ///     Return
    ///                             0: No pending read request. Call to PassScan_Read_Request() has not been done yet. Or invalid parameter value(s)
    ///                             1: There is a pending request, and API is waiting for a response.
    ///                             2: There is a ready to be read input data, and parameters have been filled. 
    ///                                Once the data buffer is retrieved, the internal buffer will be cleared out.
    ///
    PASS_SCAN_API
        unsigned int  WINAPI PassScan_Async_Read (
                char *cType,            // Passport type
                char *szCountry,        // Issued Country
                char *szName,	        // Name (in the format of "Surname, Name")
                char *szNationality,    // Nationality
                char *szPassportNo,     // Passport No
                char *nBirth_Year,      // Date of birth - Year
                char *nBirth_Month,     // Date of birth - Month
                char *nBirth_Day,       // Date of birth - Day
                char *cGender,          // Gender
                char *nExp_Year,        // Expiration Date - Year
                char *nExp_Month,       // Expiration Date - Mont
                char *nExp_Day          // Expiration Date - Day
            );
    
    /// Checks and retrieves input data after a PassScan_Read_Request() function call. Same with PassScan_Async_Read(), except the input data format.
    /// <NOTE> To retrieve input data properly, PassScan_Read_Request() must be called beforehand.
    ///     Parameter
    ///         pPassportDataBuffer: Address of a memory buffer where the raw data of the passport information will be stored. 
    ///                              The application must allocat this buffer properly before calling the function.
    ///         nBufferSize:         Size of the passport data buffer; if this size is less than actual passport data, the data will be truncated.
    ///                              
    ///     Return
    ///                             0: No pending read request. Call to PassScan_Read_Request() has not been done yet. Or invalid parameter value(s)
    ///                             1: There is a pending request, and API is waiting for a response.
    ///                             other: There is a ready to be read input data, and the data buffer have been filled, and the return value is 
    ///                                    the actually copied data size.
    ///                                    Once the data buffer is retrieved, the internal buffer will be cleared out.
    ///
    PASS_SCAN_API
        unsigned int WINAPI PassScan_Async_Read_Raw(BYTE *pPassportDataBuffer, unsigned int nBufferSize = MAX_PASSPORT_DATA_LEN);

	PASS_SCAN_API
		bool WINAPI PassScan_QRCode(QRCodeInputProc OnQRCodeInput, void *nContext = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_FaildPassportDataCallback(FailedPassportDataInputProc OnFailedPassportDataCallback, void *nContext = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_PassportCallback(QRCodeInputProc OnQRCodeInput, void *nContext = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_FwDownload(FwDownloadProc OnFwDownloadCallback, void *nContext = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_UartImageCallback(UartImageProc OnUartImageCallback, void *nContext = NULL);

	PASS_SCAN_API
		bool WINAPI PassScan_SendData(BYTE *pData, int nDataSize);

	PASS_SCAN_API
		unsigned int WINAPI PassScan_getFwVer(BYTE *pFwVerDataBuffer, unsigned int nBufferSize, int nTimeOut /*= 60*/);

	PASS_SCAN_API
		unsigned int WINAPI PassScan_CheckDigit_Check();

	PASS_SCAN_API
		unsigned int WINAPI PassScan_PassportDataCorrectionStatus_Check();

	PASS_SCAN_API
		unsigned int WINAPI PassScan_ImageMemorySaveStatus_Check();

	PASS_SCAN_API
		unsigned int WINAPI PassScan_ImageSendingStatus_Check();

	PASS_SCAN_API
		unsigned int WINAPI PassScan_TimeOutSec_Check();

	PASS_SCAN_API
		void WINAPI PassScan_EnableLogGTF(bool EnableLog);







    /// Cancel the outstanding asynchronous read operation invoked by the previous PassScan_ReadOverlapped() function call. 
    ///     
    ///     Parameter
    ///         None
    ///     Return
    ///         None
    ///     
    PASS_SCAN_API
    void WINAPI PassScan_CancelOverlapped();

	PASS_SCAN_API
		void WINAPI PassScan_Ocr_Mode_Usb();

	PASS_SCAN_API
		void WINAPI PassScan_Ocr_Mode_Vcom();

	PASS_SCAN_API
		void WINAPI PassScan_Ocr_Mode_Serial();

	PASS_SCAN_API
		void WINAPI PassScan_CheckDigit_OnOff(int OnOff);

	PASS_SCAN_API
		void WINAPI PassScan_PassportDataCorrection_OnOff(int OnOff);

	PASS_SCAN_API
		void WINAPI PassScan_ImageSending_OnOff(int OnOff);

	PASS_SCAN_API
		void WINAPI PassScan_ImageMemorySave_OnOff(int OnOff);

	PASS_SCAN_API
		void WINAPI PassScan_TimeOutSetting(unsigned int OnOff);

	PASS_SCAN_API
		void WINAPI PassScan_ImageMemory_Format();

	PASS_SCAN_API
		void WINAPI PassScan_SavedImage_Send();

	////////////////////////////// GTF Function /////////////////////////////////////////////////////////////////////////////

	PASS_SCAN_API
		unsigned int WINAPI PassScan_Read_Raw_GTF(BYTE *pPassportDataBuffer, unsigned int nBufferSize = MAX_PASSPORT_DATA_LEN, int nTimeOut = 60);

	PASS_SCAN_API
		void WINAPI PassScan_Xsps_Decryption(CString xspsFilePath);

	PASS_SCAN_API
		 void WINAPI PassScan_Encryption_PassportNumber_Decryption(char* descryptedPassportNumber, CString xspsFilePath);

	PASS_SCAN_API
		void WINAPI PassScan_Encryption_String_Decryption(char* decryptedString, CString encryptionString);

	PASS_SCAN_API
		void WINAPI PassScan_String_Enecryption(char* encryptedString, CString plainString);

	PASS_SCAN_API
		BOOL WINAPI PassScan_ConnectionOK_GTF();

	PASS_SCAN_API
		bool WINAPI PassScan_ReadOverlapped_Raw_GTF(PassportInputProc_Raw OnPassportInputRaw, int nTimeOut = 60, void *nContext = NULL);

	PASS_SCAN_API
		BOOL WINAPI PassScan_ReadCancle_GTF();

	PASS_SCAN_API
		int WINAPI PassScan_sum(int a, int b);


#ifdef __cplusplus
}
#endif /// __cplusplus

#endif ///_PASS_SCAN_API_H_
