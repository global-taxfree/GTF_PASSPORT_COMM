// Serial.cpp

#include "stdafx.h"

#include "Serial.h"
#include <algorithm>    // std::find

CSerial::CSerial()
{
	memset( &m_OverlappedRead, 0, sizeof( OVERLAPPED ) );
 	memset( &m_OverlappedWrite, 0, sizeof( OVERLAPPED ) );
	m_hIDComDev = NULL;
	m_bOpened = FALSE;
}

CSerial::~CSerial()
{
	Close();
}

HANDLE CSerial::GetHandle()
{
	return m_hIDComDev;
}

BOOL CSerial::Open(int nPort, int nBaud )
{
	if( m_bOpened ) return( TRUE );

	char szPort[15];
	char szComParams[50];
	DCB dcb;

	sprintf( szPort, _T("\\\\.\\COM%d"), nPort );
	m_hIDComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL );
	if( m_hIDComDev == NULL ) return( FALSE );

	memset( &m_OverlappedRead, 0, sizeof( OVERLAPPED ) );
 	memset( &m_OverlappedWrite, 0, sizeof( OVERLAPPED ) );

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts( m_hIDComDev, &CommTimeOuts );

	sprintf( szComParams, _T("COM%d:%d,n,8,1"), nPort, nBaud );

	m_OverlappedRead.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	m_OverlappedWrite.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	dcb.DCBlength = sizeof( DCB );
	GetCommState( m_hIDComDev, &dcb );
	dcb.BaudRate = nBaud;
	dcb.ByteSize = 8;
	unsigned char ucSet;
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_DTRDSR ) != 0 );
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_RTSCTS ) != 0 );
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_XONXOFF ) != 0 );
	if( !SetCommState( m_hIDComDev, &dcb ) ||
		!SetupComm( m_hIDComDev, 10000, 10000 ) ||
		m_OverlappedRead.hEvent == NULL ||
		m_OverlappedWrite.hEvent == NULL ){
		DWORD dwError = GetLastError();
		if( m_OverlappedRead.hEvent != NULL ) CloseHandle( m_OverlappedRead.hEvent );
		if( m_OverlappedWrite.hEvent != NULL ) CloseHandle( m_OverlappedWrite.hEvent );
		CloseHandle( m_hIDComDev );
		return( FALSE );
		}

	m_bOpened = TRUE;

	return( m_bOpened );

}

BOOL CSerial::Close( void )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( TRUE );

	if( m_OverlappedRead.hEvent != NULL ) CloseHandle( m_OverlappedRead.hEvent );
	if( m_OverlappedWrite.hEvent != NULL ) CloseHandle( m_OverlappedWrite.hEvent );
	CloseHandle( m_hIDComDev );
	m_bOpened = FALSE;
	m_hIDComDev = NULL;

	return( TRUE );

}

BOOL CSerial::WriteCommByte( unsigned char ucByte )
{
	BOOL bWriteStat;
	DWORD dwBytesWritten;

	bWriteStat = WriteFile( m_hIDComDev, (LPSTR) &ucByte, 1, &dwBytesWritten, &m_OverlappedWrite );
	if( !bWriteStat && ( GetLastError() == ERROR_IO_PENDING ) ){
		if( WaitForSingleObject( m_OverlappedWrite.hEvent, 1000 ) ) dwBytesWritten = 0;
		else{
			GetOverlappedResult( m_hIDComDev, &m_OverlappedWrite, &dwBytesWritten, FALSE );
			m_OverlappedWrite.Offset += dwBytesWritten;
			}
		}

	return( TRUE );

}

int CSerial::SendData( const char *buffer, int size )
{
#if 0
	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwBytesWritten = 0;
	int i;
	for( i=0; i<size; i++ ){
		WriteCommByte( buffer[i] );
		dwBytesWritten++;
	}
#else
	BOOL bWriteStat;
	DWORD dwBytesWritten;

	bWriteStat = WriteFile( m_hIDComDev, (LPSTR)buffer, size, &dwBytesWritten, &m_OverlappedWrite );
	if( !bWriteStat && ( GetLastError() == ERROR_IO_PENDING ) ){
		if( WaitForSingleObject( m_OverlappedWrite.hEvent, 1000 ) ) dwBytesWritten = 0;
		else{
			GetOverlappedResult( m_hIDComDev, &m_OverlappedWrite, &dwBytesWritten, FALSE );
			m_OverlappedWrite.Offset += dwBytesWritten;
		}
	}
#endif
	return( (int) dwBytesWritten );
}

int CSerial::ReadDataWaiting( void )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwErrorFlags;
	COMSTAT ComStat;

	memset(&ComStat, 0, sizeof(ComStat));
	ClearCommError( m_hIDComDev, &dwErrorFlags, &ComStat );

	return( (int) ComStat.cbInQue );

}

int CSerial::ReadData( void *buffer, int limit )
{
	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	BOOL bReadStatus;
	DWORD dwBytesRead, dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError( m_hIDComDev, &dwErrorFlags, &ComStat );
	if( !ComStat.cbInQue ) return( 0 );

	dwBytesRead = (DWORD) ComStat.cbInQue;
	if( limit < (int) dwBytesRead ) dwBytesRead = (DWORD) limit;

	bReadStatus = ReadFile( m_hIDComDev, buffer, dwBytesRead, &dwBytesRead, &m_OverlappedRead );
	if( !bReadStatus ){
		if( GetLastError() == ERROR_IO_PENDING ){
			WaitForSingleObject( m_OverlappedRead.hEvent, 2000 );
			GetOverlappedResult( m_hIDComDev, &m_OverlappedRead, &dwBytesRead, FALSE );
			return( (int) dwBytesRead );
		}
		return( 0 );
	}

	return( (int) dwBytesRead );
}

int CSerial::ReadData( void *buffer, int limit, int timeout)
{
	DWORD start = GetTickCount();
	while(1) {
		if((GetTickCount()-start) > (DWORD)timeout) return 0;
		if(ReadDataWaiting()) break;
		Sleep(100);
	}
	return ReadData(buffer, limit);
}

int CSerial::ReadUpto( void *buffer, int limit, int timeout, char last)
{
	int lent = 0;
	DWORD start = GetTickCount();
	char* pbs = (char*)buffer;
	char* pb = pbs;
	while(1) {
		if(ReadDataWaiting()) {
			int len = ReadData(pb, limit);
			lent += len;

			//if(pb[len] == last) break;
			if(std::find (pb, pb+len, last) != pb+len) 
				break;
			pb += len;
			lent += len;
		}
		else if((GetTickCount()-start) > (DWORD)timeout) {
			break;
		}
		Sleep(100);
	}
	return lent;
}

int CSerial::SendDataAsync( const char *buffer, int size )
{
	BOOL bWriteStat;
	DWORD dwBytesWritten;

	bWriteStat = WriteFile( m_hIDComDev, (LPSTR)buffer, size, &dwBytesWritten, &m_OverlappedWrite );
	//if( !bWriteStat && ( GetLastError() == ERROR_IO_PENDING ) ){
	//	if( WaitForSingleObject( m_OverlappedWrite.hEvent, 1000 ) ) dwBytesWritten = 0;
	//	else{
	//		GetOverlappedResult( m_hIDComDev, &m_OverlappedWrite, &dwBytesWritten, FALSE );
	//		m_OverlappedWrite.Offset += dwBytesWritten;
	//	}
	//}
	return( (int) dwBytesWritten );
}

int CSerial::Flush()
{
#if 0
	char buffer[256];
	while(ReadData(buffer, 256) > 0);
	return 0;
#else
	return PurgeComm(m_hIDComDev, PURGE_RXCLEAR);
#endif
}
