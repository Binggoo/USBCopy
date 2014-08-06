#pragma once
#include "SerialPort.h"

#define POWER_ON                      0x0011
#define POWER_OFF                     0x0010
#define RED_LIGHT_ON                  0x0021
#define RED_LIGHT_OFF                 0x0020
#define GREEN_LIGHT_ON                0x0031
#define GREEN_LIGHT_OFF               0x0030
#define GREEN_LIGHT_FLASH             0x0041

#define GREEN_RED_LIGHT_ON            0x0051
#define GREEN_RED_LIGHT_OFF           0x0050

#define POWER_RESET                   0xD2D2
#define LIGHT_RESET                   0xD3D3
#define ENABLE_FLASH_LIGHT            0xD4D4
#define BUZZER_ON                     0xD5D5
#define BUZZER_OFF                    0xD6D6

#define ALL_POWER_ON                  0xD7D7
#define ALL_RED_LIGHT_ON              0xD8D8
#define ALL_RED_LIGHT_OFF             0xD9D9
#define ALL_GREEN_LIGHT_ON            0xDADA
#define ALL_GREEN_LIGHT_OFF           0xDBDB

#define SWITCH_SCREEN                 0xDFDF

#define SLEEP_TIME         100

class CPortCommand
{
public:
	CPortCommand(void);
	~CPortCommand(void);

	void Init(CSerialPort *pSerialPort,HANDLE hFile);

	void RedLight(int nPort,BOOL bLight);
	void GreenLight(int nPort,BOOL bLight);
	void GreenLightFlash(int nPort);
	void RedGreenLight(int nPort,BOOL bLight);

	void EnableFlashLight();

	void AllPass();
	void AllFail();

	void Power(int nPort,BOOL bOn);

	
	void ResetPower();
	void ResetLight();
	void Buzzer(BOOL bOn);
	void SwitchScreen();


private:
	CSerialPort *m_pSerialPort;
	HANDLE m_hLogFile;
};

