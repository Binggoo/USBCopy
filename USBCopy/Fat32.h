#pragma once
#include "Fs.h"
class CFat32 : public Fs
{
public:
	CFat32();
	~CFat32(void);

	virtual BOOL FormatPartition();
};

