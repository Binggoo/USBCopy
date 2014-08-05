#pragma once
#include "SerialPort.h"

#define POWER_ON           0x0011
#define POWER_OFF          0x0010
#define RED_LIGHT_ON       0x0021
#define RED_LIGHT_OFF      0x0020
#define GREEN_LIGHT_ON     0x0031
#define GREEN_LIGHT_OFF    0x0030
#define GREEN_LIGHT_FLASH  0x0041
#define RESET_COMMAND      0xD2D2
#define WORK_COMMAND       0xD4D4
#define BUZZER_ON          0xD5D5
#define BUZZER_OFF         0xD8D8
#define ALL_PASS           0xDBDB
#define ALL_FAIL           0xDADA
#define SWITCH_SCREEN      0xDFDF

#define SLEEP_TIME         100

class CPortCommand
{
public:
	CPortCommand(void);
	~CPortCommand(void);

	void Init(CSerialPort *pSerialPort,HANDLE hFile);

	void RedLight(int nPort,BOOL bLight,BOOL bMaster);
	void GreenLight(int nPort,BOOL bLight,BOOL bMaster);
	void GreenLightFlash(int nPort,BOOL bMaster);
	void WorkLight();
	void AllPass();
	void AllFail();
	void Power(int nPort,BOOL bOn);

	
	void Reset();
	void Buzzer(BOOL bOn);
	void SwitchScreen();


private:
	CSerialPort *m_pSerialPort;
	HANDLE m_hLogFile;
};

