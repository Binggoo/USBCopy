// Lockgen.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Lisence.h"

#define LOCK_FILE "lock.dat"

int _tmain(int argc, _TCHAR* argv[])
{
	
	FILE *fp = fopen(LOCK_FILE,"wb");

	if (fp == NULL)
	{
		printf("create lock file error.\n\n");
		return 1;
	}

	CLisence lisence;
	BYTE *pLock = lisence.GetLock();

	printf("Machine Lock Code:\n");
	for (int i = 0; i < LOCK_LEN;i++)
	{
		printf("%02X",pLock[i]);
	}
	printf("\n\n");

	fwrite(pLock,1,LOCK_LEN,fp);
	fclose(fp);

	return 0;
}

