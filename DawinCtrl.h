#pragma once
class CDawinCtrl
{
public:
	CDawinCtrl(void);
	~CDawinCtrl(void);

	BOOL Initialize(void);
	BOOL Connect(void);
	BOOL DisConnect(void);
	BOOL Test_DisConnect(void);
	BOOL Scan(void);
	BOOL GetData(char *refPassInfo);
	BOOL DawinFuncLoad(void);
	BOOL GetMRZ1(char *refPassInfo);
	BOOL GetMRZ2(char *refPassInfo);
	BOOL SetSavePath(char *strSavePath);
};

