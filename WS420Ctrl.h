#pragma once
class CWS420Ctrl : public CSerial
{
public:
	CWS420Ctrl(void);
	~CWS420Ctrl(void);

	BOOL Initialize(void);
	int SendCommand( CSerial *rs, const char* str );
	int check_response( char *response, int size, char *cmd, char *dat, int datlen );
	int SearchPort( CSerial *rs );
	BOOL Open( CSerial *rs, int nPort, int nBaud);
	BOOL Close( CSerial *rs );
	int  Flush( CSerial *rs );
	int ReadDataWaiting( CSerial *rs );
	int ReadData( CSerial *rs, void *buffer, int limit );
	int ReadUpto( CSerial *rs, void *buffer, int limit, int timeout, char last);
};

