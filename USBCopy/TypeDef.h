#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef IN_OUT
#define IN_OUT
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef union tag_QWORD
{
	BYTE ByPart[8];
	struct
	{
		DWORD dwLow;
		DWORD dwHigh;
	} DualPart;
	LONGLONG QuadPart;
	INT64	 QuadIntPart;
}QWORD;

inline QWORD operator +(QWORD a, QWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart + b.QuadPart;

	return c;
}; 



inline QWORD operator +(QWORD a, int b)
{
	QWORD c;
	c.QuadPart = a.QuadPart + (LONGLONG)b;
	return c;
}

inline QWORD operator +(QWORD a, DWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart + (LONGLONG)b;
	return c;
}

inline QWORD operator * (QWORD a, DWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart * (LONGLONG)b;
	return c;
}

inline QWORD operator / (QWORD a, BYTE b)
{
	QWORD c;
	c.QuadPart = a.QuadPart / (LONGLONG)b;
	return c;
}

inline QWORD operator / (QWORD a, DWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart / (LONGLONG)b;
	return c;
}

inline QWORD operator % (QWORD a, DWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart / (LONGLONG)b;
	return c;
}

inline QWORD operator -(QWORD a, DWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart - (LONGLONG)b;
	return c;
}

inline QWORD operator -(QWORD a, QWORD b)
{
	QWORD c;
	c.QuadPart = a.QuadPart - b.QuadPart;
	return c;
}

inline QWORD operator +=(QWORD &a, QWORD b)
{	
	a.QuadPart += b.QuadPart;
	return a;
}

inline QWORD operator +=(QWORD &a, DWORD b)
{	
	a.QuadPart += (LONGLONG)b;
	return a;
}

inline QWORD operator +=(QWORD &a, int b)
{	
	a.QuadPart += (LONGLONG)b;
	return a;
}

inline QWORD operator -=(QWORD &a, QWORD b)
{	
	a.QuadPart -= b.QuadPart;
	return a;
}

inline QWORD operator -=(QWORD &a, DWORD b)
{	
	a.QuadPart -= (LONGLONG)b;
	return a;
}

inline QWORD operator -=(QWORD &a, int b)
{	
	a.QuadPart -= (LONGLONG)b;
	return a;
}

inline bool  operator ==(QWORD a,int b)
{
	if (a.QuadPart == (LONGLONG)b)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator ==(QWORD a,QWORD b)
{
	if (a.QuadPart == b.QuadPart)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator != (QWORD a,int b)
{
	if (a.QuadPart == (LONGLONG)b)
	{
		return false;
	}
	else
	{
		return true;
	}
}

inline bool  operator != (QWORD a,QWORD b)
{
	if (a.QuadPart == b.QuadPart)
	{
		return false;
	}
	else
	{
		return true;
	}
}

inline bool  operator > (QWORD a,int b)
{
	if (a.QuadPart > (LONGLONG)b)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator > (QWORD a,QWORD b)
{
	if (a.QuadPart > b.QuadPart)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator < (QWORD a,int b)
{
	if (a.QuadPart < (LONGLONG)b)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator < (QWORD a,QWORD b)
{
	if (a.QuadPart < b.QuadPart)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator >= (QWORD a,int b)
{
	if (a.QuadPart >= (LONGLONG)b)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool  operator >= (QWORD a,QWORD b)
{
	if (a.QuadPart >= b.QuadPart)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool operator <= (QWORD a,int b)
{
	if (a.QuadPart <= (LONGLONG)b)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool operator <= (QWORD a,QWORD b)
{
	if (a.QuadPart <= b.QuadPart)
	{
		return true;
	}
	else
	{
		return false;
	}
}


#define SectorToBytes(x) (x<<9)
#define BytesToSector(x) (x>>9)

#ifndef ERRCODE
#define ERRCODE unsigned short
#endif

#endif //_TYPEDEF_H