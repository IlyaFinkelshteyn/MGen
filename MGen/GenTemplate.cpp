#include "stdafx.h"
#include "GenTemplate.h"


/* if (flag!=0), then use the contents of randrsl[] to initialize mm[]. */
#define mix(a,b,c,d,e,f,g,h) \
{ \
	a ^= b << 11; d += a; b += c; \
	b ^= c >> 2;  e += b; c += d; \
	c ^= d << 8;  f += c; d += e; \
	d ^= e >> 16; g += d; e += f; \
	e ^= f << 10; h += e; f += g; \
	f ^= g >> 4;  a += f; g += h; \
	g ^= h << 8;  b += g; h += a; \
	h ^= a >> 9;  c += h; a += b; \
}

void CGenTemplate::CheckVar(CString * sName, CString * sValue, char* sSearch, int * Dest, int vmin, int vmax)
{
	if (*sName == sSearch) {
		*Dest = atoi(*sValue);
		if ((vmin != -1) && (vmax != -1)) {
			if (*Dest < vmin) *Dest = vmin;
			if (*Dest > vmax) *Dest = vmax;
		}
	}
}

void CGenTemplate::LoadVar(CString * sName, CString * sValue, char* sSearch, CString * Dest)
{
	if (*sName == sSearch) {
		*Dest = *sValue;
	}
}

void CGenTemplate::LoadConfig(CString fname)
{
	CString st, st2, st3;
	ifstream fs;
	//prepare f to throw if failbit gets set
	//ios_base::iostate exceptionMask = fs.exceptions() | ios::failbit;
	//fs.exceptions(exceptionMask);
	//try {
		fs.open(fname);
	//}
	//catch (ios_base::failure& e) {
		//CString* est = new CString;
		//est->Format("LoadConfig %s got error %s", fname, e.what());
		//::PostMessage(m_hWnd, WM_DEBUG_MSG, 1, (LPARAM)est);
		//return;
	//}
	char pch[2550];
	int pos = 0;
	int i = 0;
	//try {
		while (fs.good()) {
			i++;
			// Get line
			fs.getline(pch, 2550);
			st = pch;
			// Remove unneeded
			pos = st.Find("#");
			if (pos != -1) st = st.Left(pos);
			st.Trim();
			pos = st.Find("=");
			if (pos != -1) {
				// Get variable name and value
				st2 = st.Left(pos);
				st3 = st.Mid(pos + 1);
				st2.Trim();
				st3.Trim();
				st2.MakeLower();
				LoadConfigLine(&st2, &st3);
			}
		}
	//}
	//catch (ios_base::failure& e) {
		//fs.close();
		//CString* est = new CString;
		//est->Format("LoadConfig %s got error %s at line %d", fname, e.what(), i);
		//::PostMessage(m_hWnd, WM_DEBUG_MSG, 1, (LPARAM)est);
		//return;
	//}
	fs.close();
}

bool CGenTemplate::dirExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool CGenTemplate::fileExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return false;   // this is a directory!

	return true;    // this is not a directory!
}

bool CGenTemplate::nodeExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	return true;    // file exists
}

CGenTemplate::CGenTemplate()
{
}


CGenTemplate::~CGenTemplate()
{
	StopMIDI();
}

void CGenTemplate::isaac()
{
	register ub4 i, x, y;

	cc = cc + 1;    /* cc just gets incremented once per 256 results */
	bb = bb + cc;   /* then combined with bb */

	for (i = 0; i<256; ++i)
	{
		x = mm[i];
		switch (i % 4)
		{
		case 0: aa = aa ^ (aa << 13); break;
		case 1: aa = aa ^ (aa >> 6); break;
		case 2: aa = aa ^ (aa << 2); break;
		case 3: aa = aa ^ (aa >> 16); break;
		}
		aa = mm[(i + 128) % 256] + aa;
		mm[i] = y = mm[(x >> 2) % 256] + aa + bb;
		randrsl[i] = bb = mm[(y >> 10) % 256] + x;

		/* Note that bits 2..9 are chosen from x but 10..17 are chosen
		from y.  The only important thing here is that 2..9 and 10..17
		don't overlap.  2..9 and 10..17 were then chosen for speed in
		the optimized version (rand.c) */
		/* See http://burtleburtle.net/bob/rand/isaac.html
		for further explanations and analysis. */
	}
}

void CGenTemplate::randinit(int flag)
{
	int i;
	ub4 a, b, c, d, e, f, g, h;
	aa = bb = cc = 0;
	a = b = c = d = e = f = g = h = 0x9e3779b9;  /* the golden ratio */

	for (i = 0; i<4; ++i)          /* scramble it */
	{
		mix(a, b, c, d, e, f, g, h);
	}

	for (i = 0; i<256; i += 8)   /* fill in mm[] with messy stuff */
	{
		if (flag)                  /* use all the information in the seed */
		{
			a += randrsl[i]; b += randrsl[i + 1]; c += randrsl[i + 2]; d += randrsl[i + 3];
			e += randrsl[i + 4]; f += randrsl[i + 5]; g += randrsl[i + 6]; h += randrsl[i + 7];
		}
		mix(a, b, c, d, e, f, g, h);
		mm[i] = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
		mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
	}

	if (flag)
	{        /* do a second pass to make all of the seed affect all of mm */
		for (i = 0; i<256; i += 8)
		{
			a += mm[i]; b += mm[i + 1]; c += mm[i + 2]; d += mm[i + 3];
			e += mm[i + 4]; f += mm[i + 5]; g += mm[i + 6]; h += mm[i + 7];
			mix(a, b, c, d, e, f, g, h);
			mm[i] = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
			mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
		}
	}

	isaac();            /* fill in the first set of results */
	randcnt = 256;        /* prepare to use the first set of results */
}

unsigned int CGenTemplate::rand2() {
	cur_rand2++;
	if (cur_rand2 > 1) {
		cur_rand2 = 0;
		cur_rand++;
		if (cur_rand > 255) {
			cur_rand = 0;
			isaac();
		}
	}
	//return randrsl[cur_rand];
	return ((randrsl[cur_rand]) >> (cur_rand2 * 16)) % RAND_MAX;
}

void CGenTemplate::InitRandom()
{
	// Init rand
	srand((unsigned int)time(NULL));
	// ISAAC
	ub4 i;
	aa = bb = cc = (ub4)0;
	for (i = 0; i < 256; ++i) mm[i] = randrsl[i] = rand()*rand();
	randinit(1);
}

void CGenTemplate::InitVectors()
{
		// Create vectors
	pause = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	note = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	len = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	coff = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	poff = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	noff = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	att = vector<vector<unsigned char>>(t_allocated, vector<unsigned char>(v_cnt));
	tempo = vector<unsigned short>(t_allocated);
	stime = vector<double>(t_allocated);
	ntime = vector<double>(t_allocated);
}

void CGenTemplate::ResizeVectors(int size)
{
	milliseconds time_start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
	if (!mutex_output.try_lock_for(chrono::milliseconds(5000))) {
		::PostMessage(m_hWnd, WM_DEBUG_MSG, 1, (LPARAM)new CString("Critical error: ResizeVectors mutex timed out"));
	}
	pause.resize(size);
	note.resize(size);
	len.resize(size);
	coff.resize(size);
	poff.resize(size);
	noff.resize(size);
	tempo.resize(size);
	stime.resize(size);
	ntime.resize(size);
	att.resize(size);
	for (int i = t_allocated; i < size; i++) {
		pause[i].resize(v_cnt);
		note[i].resize(v_cnt);
		len[i].resize(v_cnt);
		coff[i].resize(v_cnt);
		poff[i].resize(v_cnt);
		noff[i].resize(v_cnt);
		att[i].resize(v_cnt);
	}
	// Count time
	milliseconds time_stop = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
	CString* st = new CString;
	st->Format("ResizeVectors from %d to %d (in %d ms)", t_allocated, size, time_stop - time_start);
	::PostMessage(m_hWnd, WM_DEBUG_MSG, 0, (LPARAM)st);

	t_allocated = size;
	mutex_output.unlock();
}

void CGenTemplate::StartMIDI(int midi_device_i, int latency)
{
	// Clear error flag
	buffer_underrun = 0;
	midi_play_step = 0;
	midi_start_time = 0;
	midi_sent_t = 0;
	midi_sent = 0;
	TIME_START;
	Pm_OpenOutput(&midi, midi_device_i, NULL, OUTPUT_BUFFER_SIZE, TIME_PROC, NULL, latency);
	CString* st = new CString;
	st->Format("Pm_OpenOutput: buffer size %d, latency %d", OUTPUT_BUFFER_SIZE, latency);
	::PostMessage(m_hWnd, WM_DEBUG_MSG, 4, (LPARAM)st);
}

void CGenTemplate::SendMIDI(int step1, int step2)
{
	milliseconds time_start = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
	PmTimestamp timestamp_current = TIME_PROC(TIME_INFO);
	PmTimestamp timestamp;
	if (midi_sent_t != 0) timestamp = midi_sent_t;
	else timestamp = timestamp_current;
	// Check if we have buffer underrun
	if (timestamp < timestamp_current) {
		CString* st = new CString;
		st->Format("SendMIDI got buffer underrun in %d ms (steps %d - %d)", timestamp_current - timestamp, step1, step2);
		::PostMessage(m_hWnd, WM_DEBUG_MSG, 1, (LPARAM)st);
		timestamp = timestamp_current;
		buffer_underrun = 1;
	}
	PmTimestamp timestamp0 = timestamp;
	// Set playback start
	if (step1 == 0) midi_start_time = timestamp0;
	// Check if buffer is full
	if (midi_sent_t - timestamp_current > MIN_MIDI_BUFFER_MSEC) {
		CString* st = new CString;
		st->Format("SendMIDI: no need to send (full buffer = %d ms) (steps %d - %d) playback is at %d", 
			midi_sent_t - timestamp_current, step1, step2, timestamp_current - midi_start_time);
		::PostMessage(m_hWnd, WM_DEBUG_MSG, 4, (LPARAM)st);
		return;
	}
	int i, ncount = 0;
	if (!mutex_output.try_lock_for(chrono::milliseconds(3000))) {
		::PostMessage(m_hWnd, WM_DEBUG_MSG, 0, (LPARAM)new CString("SendMIDI mutex timed out: will try later"));
	}
	int step21;
	int step22;
	// Move to note start
	if (coff[step1][0] > 0) step21 = step1 - coff[step1][0];
	else step21 = step1;
	// Find last step not too far
	for (i = step21; i <= step2; i++) {
		step22 = i;
		if (stime[i] - stime[step1] + timestamp - timestamp_current > MAX_MIDI_BUFFER_MSEC) break;
	}
	// Count notes
	for (i = step21; i < step22; i++) {
		if (i + len[i][0] > step22) break;
		ncount++;
		if (noff[i][0] == 0) break;
		i += noff[i][0] - 1;
	}
	// Send notes
	PmEvent* buffer = new PmEvent[ncount*2];
	i = step21;
	for (int x = 0; x < ncount; x++) {
		// Note ON
		timestamp = stime[step21] - stime[step1] + timestamp0;
		buffer[x*2].timestamp = timestamp;
		buffer[x*2].message = Pm_Message(0x90, note[i][0], att[i][0]);
		// Note OFF
		timestamp = ntime[i + len[i][0] - 1] - stime[step1] + timestamp0;
		buffer[x * 2 + 1].timestamp = timestamp;
		buffer[x * 2 + 1].message = Pm_Message(0x90, note[i][0], 0);
		if (noff[i][0] == 0) break;
		i += noff[i][0];
	}
	// Save last sent position
	midi_sent = step22;
	midi_sent_t = timestamp0 + ntime[midi_sent-1] - stime[step1];
	mutex_output.unlock();
	Pm_Write(midi, buffer, ncount*2);
	delete [] buffer;
	// Count time
	milliseconds time_stop = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
	CString* st = new CString;
	st->Format("Pm_Write %d notes (steps %d/%d - %d/%d) [to future %d to %d ms] (in %d ms) playback is at %d ms", 
		ncount, step21, step1, midi_sent, step2, timestamp0 - timestamp_current, midi_sent_t - timestamp_current, 
		time_stop - time_start, timestamp_current-midi_start_time);
	::PostMessage(m_hWnd, WM_DEBUG_MSG, 4, (LPARAM)st);
}

void CGenTemplate::StopMIDI()
{
	::PostMessage(m_hWnd, WM_DEBUG_MSG, 4, (LPARAM)new CString("Pm_Close"));
	if (midi != 0) Pm_Close(midi);
	midi = 0;
}

int CGenTemplate::randbw(int n1, int n2)
{
	return n1 + (double)(n2 - n1) * (double)rand2() / (double)RAND_MAX;
}

int CGenTemplate::GetPlayStep() {
	if (buffer_underrun == 1) {
		midi_play_step = 0;
	}
	else {
		// Don't need lock, because this function is called from OnDraw, which already has lock
		/*
		if (!mutex_output.try_lock_for(chrono::milliseconds(100))) {
			::PostMessage(m_hWnd, WM_DEBUG_MSG, 1, (LPARAM)new CString("GetPlayStep mutex timed out"));
		}
		*/
		int step1 = midi_play_step;
		int step2 = midi_sent;
		int cur_step, currentElement;
		int searchElement = TIME_PROC(TIME_INFO) - midi_start_time;
		while (step1 <= step2) {
			cur_step = (step1 + step2) / 2;
			currentElement = stime[cur_step];
			if (currentElement < searchElement) {
				step1 = cur_step + 1;
			}
			else if (currentElement > searchElement) {
				step2 = cur_step - 1;
			}
			else {
				break;
			}
		}
		midi_play_step = cur_step;
		//mutex_output.unlock();
	}
	return midi_play_step;
}
