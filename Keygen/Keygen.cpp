// Keygen.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Lisence.h"

#define LISENCE_FILE "lisence.dat"
#define LOCK_FILE    "lock.dat"

BOOL g_bLockFile = FALSE;
BOOL g_bLockString = FALSE;
char g_szLockFile[MAX_PATH] = {0};
char g_szLockString[256] = {0};

void ValidateArgs(int argc,char *argv[])
{
	for (int i = 1; i < argc;i++)
	{
		if (argv[i][0] == _T('-') || argv[i][0] == _T('/'))
		{
			TCHAR tcFlag = argv[i][1];
			switch (tolower(tcFlag))
			{
			case _T('f'):
				g_bLockFile = TRUE;
				if (strlen(argv[i]) > 3)
				{
					strcpy_s(g_szLockFile,strlen(&argv[i][3]) + 1,&argv[i][3]);
				}
				else
				{
					strcpy_s(g_szLockFile,strlen(LOCK_FILE) + 1,LOCK_FILE);
				}
				break;

			case _T('s'):
				if (strlen(argv[i]) > 3)
				{
					g_bLockString = TRUE;
					strcpy_s(g_szLockString,strlen(&argv[i][3]) + 1,&argv[i][3]);
				}
				break;

			}
		}
	}
}

int StringToByte(BYTE *byDest, const char *src)
{
	int len = strlen(src) / 2;
	char temp[3] = {NULL};

	for (int i = 0; i < len;i++)
	{
		strcpy_s(temp,3,src + i * 2);

		byDest[i] = strtol(temp,NULL,16);
	}

	return len;
}

int main(int argc, char* argv[])
{

	ValidateArgs(argc,argv);

	CLisence lisence;

	BYTE byKey[KEY_LEN] = {0};

	if (g_bLockFile)
	{
		BYTE *pByte = lisence.GetKeyFromFile(g_szLockFile);

		memcpy(byKey,pByte,KEY_LEN);
	}
	else if (g_bLockString)
	{
		// 把string转换成byte;
		BYTE *pLock = new BYTE[strlen(g_szLockString) / 2];
		memset(pLock,0,strlen(g_szLockString) / 2);

		int len = StringToByte(pLock,g_szLockString);

		BYTE *pByte = lisence.GetKey(pLock,len);

		memcpy(byKey,pByte,KEY_LEN);

		delete []pLock;
	}
	else
	{
		BYTE *pByte = lisence.GetLock();
		printf("Machine Lock Code:\n");
		for (int i = 0;i < LOCK_LEN;i++)
		{
			printf("%02X",pByte[i]);
		}
		printf("\n\n");

		pByte = lisence.GetKey();

		memcpy(byKey,pByte,KEY_LEN);

	}

	FILE *fp = fopen(LISENCE_FILE,"wb");

	if (fp != NULL)
	{
		fwrite(byKey,1,sizeof(byKey),fp);
		fclose(fp);

		printf("Generate Key:\n");
		for (int i = 0; i < KEY_LEN;i++)
		{
			printf("%02X",byKey[i]);
		}
		printf("\n\n");

		return 0;
	}
	else
	{
		printf("create %s file error\n\n",LISENCE_FILE);

		return 1;
	}
}

