#pragma once
#include "GenTemplate.h"
class CGenRS1 :
	public CGenTemplate
{
public:
	CGenRS1();
	~CGenRS1();
	void LoadConfigLine(CString * sN, CString * sV);
	void Generate() override;

	int min_tempo = 160;
	int max_tempo = 200;
	int min_len = 1;
	int max_len = 5;
	int min_note = 60;
	int max_note = 71;
	int note_step = 3;
	int sleep_ms = 10;
};

