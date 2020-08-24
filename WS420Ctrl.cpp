
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#include "Serial.h"
#include "WS420Ctrl.h"

CWS420Ctrl   gWiseCom;							// Wisecude420용 통신

CWS420Ctrl::CWS420Ctrl(void)
{


}


CWS420Ctrl::~CWS420Ctrl(void)
{
}

//  Resend 전송시 에러를 방지하기 위하여
BOOL CWS420Ctrl::Initialize( void )
{

	return TRUE;
}

/* *********************************************
 * command send & response
 *
 * input
 * rs			serial port class
 * str			command + [CR(0x0d)]
 * response		response buff
 * rsplen		response buff size
 * timeout		receive wait time [1/1000sec]
 *
 * ret
 *  0:			success 		[#P][cmd] + data
 *  1:			error response 	[#N][cmd] + error code
 * -1:			no response
 * -2:			frame error
   *********************************************/
int CWS420Ctrl::SendCommand( CSerial *rs, const char* str ) //, char* response, int rsplen, int timeout )
{
	char command[16];
	int ret;
	
	sprintf(command, _T("#%s\r"), str );

	ret =  rs->SendData( command, strlen(command));
	if( ret == 0 ) return -1;
	return 0;

}

/* *********************************************
 * check_response
 *
 * input
 * response:		response buffer point
 * size:			response receive length
 * cmd:				extract command from response
 * dat:				extract data from response
 * datlen:			data max length
 *
 * return
 * 0:				no response
 * 1:				positive response
 * 2:				Negative response
 * -1:				frame format error
   *********************************************/
int CWS420Ctrl::check_response( char *response, int size, char *cmd, char *dat, int datlen )
{
	if( size == 0 ) return 0;
	if( response[0] != '#' ) return -1;
	if( response[size-1] != '\0' ) return -1;
	if( response[size-2] != '\r' ) return -1;


	if( response[1] == 'P' || response[1] == 'N' )
	{
		if( cmd != NULL ) memcpy( cmd, &response[2], 2 );

		if( dat != NULL )  
		{
			memset( dat, 0x00, datlen );
			memcpy( dat, &response[4], size - 6 ); 
		}
		return ( response[1] == 'P' ? 1 : 2 );
	}

	return -1;
}


/* *********************************************
 * Search Port
 *
 * input
 * portname:		find port name
 * length:			buff length
 *
 * return
 * 1:				success
 * 0:				not found
   *********************************************/

int CWS420Ctrl::SearchPort( CSerial *rs )
{
	int		port;
	char	response[1024];
	int		ret;

	for( port = 1; port < 32; port ++ ) {
		if( rs->Open( port, 115200 ) == TRUE ) 
		{
			ret = SendCommand(rs, "GV" ); //, response, sizeof( response ), 1000);
			if( ret == 0 ) {
				memset( response, 0x00, sizeof( response ) );
				ret = rs->ReadUpto( response, sizeof( response ) -1, 500, 0x00 );
				rs->Close();
				ret = check_response( response, ret, NULL, NULL, 0 );
				if( ret > 0  ) return port;
			}
		}
	}
	return 0;
}

BOOL CWS420Ctrl::Open( CSerial *rs, int nPort, int nBaud)
{

	return(rs->Open( nPort, nBaud));
}

BOOL CWS420Ctrl::Close( CSerial *rs )
{
	return(rs->Close());
}

int CWS420Ctrl::ReadDataWaiting( CSerial *rs )
{
	return(rs->ReadDataWaiting());
}

int CWS420Ctrl::ReadData( CSerial *rs, void *buffer, int limit )
{
	return(rs->ReadData(buffer, limit));
}

int CWS420Ctrl::ReadUpto( CSerial *rs, void *buffer, int limit, int timeout, char last)
{
	return (rs->ReadUpto(buffer, limit, timeout, last));
}

BOOL CWS420Ctrl::Flush( CSerial *rs )
{
	return(rs->Flush());
}

