#include "StdAfx.h"
#include "Utils.h"
#include "PortCommand.h"


CPortCommand::CPortCommand( void )
{

}


CPortCommand::~CPortCommand(void)
{
}

void CPortCommand::Init( CSerialPort *pSerialPort,HANDLE hFile )
{
	m_pSerialPort = pSerialPort;
	m_hLogFile = hFile;
}

void CPortCommand::RedLight( int nPort,BOOL bLight )
{
	WORD wCommand = 0;
	if (bLight)
	{
		wCommand = RED_LIGHT_ON;
	}
	else
	{
		wCommand = RED_LIGHT_OFF;
	}

	// 母盘位置暂时未定
	wCommand |= (nPort<<8);

	BYTE buffer[2] = {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::GreenLight( int nPort,BOOL bLight)
{
	WORD wCommand = 0;
	if (bLight)
	{
		wCommand = GREEN_LIGHT_ON;
	}
	else
	{
		wCommand = GREEN_LIGHT_OFF;
	}

	// 母盘位置暂时未定
	wCommand |= (nPort<<8);

	BYTE buffer[2] = {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::RedGreenLight( int nPort,BOOL bLight )
{
	WORD wCommand = 0;
	if (bLight)
	{
		wCommand = GREEN_RED_LIGHT_ON;
	}
	else
	{
		wCommand = GREEN_RED_LIGHT_OFF;
	}

	// 母盘位置暂时未定
	wCommand |= (nPort<<8);

	BYTE buffer[2] = {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::GreenLightFlash( int nPort )
{
	WORD wCommand = GREEN_LIGHT_FLASH;

	// 母盘位置暂时未定
	wCommand |= (nPort<<8);

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::EnableFlashLight()
{
	WORD wCommand = ENABLE_FLASH_LIGHT;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::AllPass()
{
	WORD wCommand = ALL_GREEN_LIGHT_ON;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::AllFail()
{
	WORD wCommand = ALL_RED_LIGHT_ON;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::ResetPower()
{
	WORD wCommand = POWER_RESET;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::ResetLight()
{
	WORD wCommand = LIGHT_RESET;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::Buzzer( BOOL bOn )
{
	WORD wCommand = 0;
	if (bOn)
	{
		wCommand = BUZZER_ON;
	}
	else
	{
		wCommand = BUZZER_OFF;
	}

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::SwitchScreen()
{
	WORD wCommand = SWITCH_SCREEN;

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}

void CPortCommand::Power( int nPort,BOOL bOn )
{
	WORD wCommand = 0;
	if (bOn)
	{
		wCommand = POWER_ON;
	}
	else
	{
		wCommand = POWER_OFF;
	}

	wCommand |= (nPort<<8);

	BYTE buffer[2]= {NULL};
	memcpy(buffer,&wCommand,2);

	m_pSerialPort->WriteBytes(buffer,2);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] TX = %02X %02X"),buffer[0],buffer[1]);
	Sleep(SLEEP_TIME);
}
