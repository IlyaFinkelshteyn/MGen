#include "../stdafx.h"
#include "GenCP1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGenCP1::CGenCP1() {
	av_cnt = 2;
	v_cnt = 2;
	track_id[0] = 2;
	track_id[1] = 1;
}

CGenCP1::~CGenCP1() {
}

void CGenCP1::LoadConfigLine(CString * sN, CString * sV, int idata, float fdata) {
	CheckVar(sN, sV, "cantus_id", &cantus_id2);
	CheckVar(sN, sV, "notes_per_measure", &npm);

	CGenCA1::LoadConfigLine(sN, sV, idata, fdata);
}

int CGenCP1::InitCP() {
	// Set rule colors
	for (int i = 0; i < MAX_SEVERITY; ++i) {
		flag_color[i] = Color(0, 255.0 / MAX_SEVERITY*i, 255 - 255.0 / MAX_SEVERITY*i, 0);
	}
	// Check that method is selected
	if (method == mUndefined) {
		WriteLog(5, "Error: method not specified in algorithm configuration file");
		error = 2;
	}
	ac.resize(av_cnt);
	acc.resize(av_cnt);
	acc_old.resize(av_cnt);
	apc.resize(av_cnt);
	apcc.resize(av_cnt);
	asmooth.resize(av_cnt);
	aleap.resize(av_cnt);
	aslur.resize(av_cnt);
	anflags.resize(av_cnt);
	anflagsc.resize(av_cnt);
	// Check harmonic meaning loaded
	LoadHarmVar();
	return error;
}

void CGenCP1::MakeNewCP() {
	// Set pitch limits
	if (cantus_high) {
		for (int i = 0; i < c_len; ++i) {
			max_cc[i] = acc[cfv][i] - min_between;
			min_cc[i] = acc[cfv][i] - max_between;
		}
	}
	else {
		for (int i = 0; i < c_len; ++i) {
			min_cc[i] = acc[cfv][i] + min_between;
			max_cc[i] = acc[cfv][i] + max_between;
		}
	}
	// Convert limits to diatonic and recalibrate
	for (int i = 0; i < c_len; ++i) {
		min_c[i] = CC_C(min_cc[i], tonic_cur, minor_cur);
		max_c[i] = CC_C(max_cc[i], tonic_cur, minor_cur);
		min_cc[i] = C_CC(min_c[i], tonic_cur, minor_cur);
		max_cc[i] = C_CC(max_c[i], tonic_cur, minor_cur);
	}
	if (random_seed) {
		RandCantus(ac[cpv], acc[cpv], 0, c_len);
	}
	else {
		FillCantus(acc[cpv], 0, c_len, min_cc);
	}
}

void CGenCP1::SingleCPInit() {
	// Copy cantus
	acc = scpoint;
	// Get diatonic steps from chromatic
	for (int v = 0; v < acc.size(); ++v) {
		for (int i = 0; i < c_len; ++i) {
			ac[v][i] = CC_C(acc[v][i], tonic_cur, minor_cur);
		}
	}
	// Save value for future use;
	acc_old = acc;
	/*
	if (!swa_inrange) {
		GetRealRange(ac[cfv], acc[cfv]);
		ApplySourceRange();
	}
	// Set pitch limits
	// If too wide range is not accepted, correct range to increase scan performance
	if (!accept[37]) {
	for (int i = 0; i < c_len; ++i) {
	min_c[i] = max(minc, c[i] - correct_range);
	max_c[i] = min(maxc, c[i] + correct_range);
	}
	}
	else {
	*/
	// Calculate limits
	if (cantus_high) {
		for (int i = 0; i < c_len; ++i) {
			max_cc[i] = min(acc[cfv][i] - min_between, acc[cpv][i] + correct_range);
			min_cc[i] = max(cf_nmax - sum_interval, acc[cpv][i] - correct_range);
		}
	}
	else {
		for (int i = 0; i < c_len; ++i) {
			min_cc[i] = max(acc[cfv][i] + min_between, acc[cpv][i] - correct_range);
			max_cc[i] = min(cf_nmin + sum_interval, acc[cpv][i] + correct_range);
		}
	}
	// Convert limits to diatonic and recalibrate
	for (int i = 0; i < c_len; ++i) {
		min_c[i] = CC_C(min_cc[i], tonic_cur, minor_cur);
		max_c[i] = CC_C(max_cc[i], tonic_cur, minor_cur);
		min_cc[i] = C_CC(min_c[i], tonic_cur, minor_cur);
		max_cc[i] = C_CC(max_c[i], tonic_cur, minor_cur);
	}
	sp1 = fn;
	sp2 = c_len;
	ep1 = max(0, sp1 - 1);
	ep2 = c_len;
	// Clear flags
	++accepted3;
	fill(flags.begin(), flags.end(), 0);
	flags[0] = 1;
	for (int i = 0; i < ep2; ++i) {
		anflagsc[cpv][i] = 0;
	}
	// Matrix scan
	if (task != tEval) {
		// Exit if no violations
		if (!smatrixc) return;
		// Create map
		smap.resize(smatrixc);
		int map_id = 0;
		for (int i = 0; i < c_len; ++i) if (smatrix[i]) {
			smap[map_id] = i;
			++map_id;
		}
		// Shuffled smap
		//unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		//::shuffle(smap.begin(), smap.end(), default_random_engine(seed));
		sp1 = 0;
		sp2 = sp1 + s_len; // End of search window
		if (sp2 > smatrixc) sp2 = smatrixc;
		// Record window
		wid = 0;
		wpos1[wid] = sp1;
		wpos2[wid] = sp2;
		// Add last note if this is last window
		// End of evaluation window
		if (method == mScan) {
			ep2 = GetMaxSmap() + 1;
			if (sp2 == smatrixc) ep2 = c_len;
			// Clear scan steps
			FillCantusMap(acc[cpv], smap, 0, smatrixc, min_cc);
			// Can skip flags - full scan must remove all flags
		}
		// For sliding windows algorithm evaluate whole melody
		if (method == mSWA) {
			ep2 = c_len;
			// Cannot skip flags - need them for penalty if cannot remove all flags
			skip_flags = 0;
			// Clear scan steps of current window
			FillCantusMap(acc[cpv], smap, sp1, sp2, min_cc);
		}
		// Minimum element
		ep1 = max(0, GetMinSmap() - 1);
		// Minimal position in array to cycle
		pp = sp2 - 1;
		p = smap[pp];
	}
	else {
		// For single cantus scan - cannot skip flags - must show all
		skip_flags = 0;
	}
}

void CGenCP1::MultiCPInit() {
	MakeNewCP();
	// First pause
	for (int i = 0; i < fn; ++i) acc[cpv][i] = acc[cpv][fn];
	sp1 = fn; // Start of search window
	sp2 = sp1 + s_len; // End of search window
	if (sp2 > c_len - 1) sp2 = c_len - 1;
	// Record window
	wid = 0;
	wpos1[wid] = sp1;
	wpos2[wid] = sp2;
	// Add last note if this is last window
	ep1 = max(0, sp1 - 1);
	ep2 = sp2; // End of evaluation window
	if (ep2 == c_len - 1) ep2 = c_len;
	p = sp2 - 1; // Minimal position in array to cycle
}

void CGenCP1::ScanCPInit() {
	// Get cantus size
	if (task != tGen) c_len = scpoint[0].size();
	ScanInit();
	// Resize global vectors
	for (int i = 0; i < av_cnt; ++i) {
		ac[i].resize(c_len); // cantus (diatonic)
		acc[i].resize(c_len); // cantus (chromatic)
		acc_old[i].resize(c_len); // Cantus diatonic saved for SWA
		apc[i].resize(c_len);
		apcc[i].resize(c_len);
		aleap[i].resize(c_len);
		asmooth[i].resize(c_len);
		aslur[i].resize(c_len);
	}
	ivl.resize(c_len);
	ivlc.resize(c_len);
	civl.resize(c_len);
	civlc.resize(c_len);
	tivl.resize(c_len);
	motion.resize(c_len);
	beat.resize(c_len);
	sus.resize(c_len);
}

int CGenCP1::SendCP() {
	int step0 = step;
	int pause_len = 0;
	CString st, info, rpst;
	int pos = 0, plen;
	int v, x1;
	int real_len2 = real_len*npm;
	Sleep(sleep_ms);
	for (int av = 0; av < av_cnt; ++av) {
		CreateLinks(ac[av]);
		MakeMacc(acc[av]);
		pos = step;
		if (cpv) {
			v = svoice + av;
		}
		else {
			v = svoice + 1 - av;
		}
		if (av == cpv) {
			plen = cc_len[0] * fn;
			FillPause(pos, plen, v);
			pos += plen;
			x1 = fn;
		}
		else {
			x1 = 0;
		}
		// Copy cantus to output
		if (step + real_len2 >= t_allocated) ResizeVectors(t_allocated * 2);
		for (int x = x1; x < c_len; ++x) {
			SendLyrics(pos, v, av, x);
			for (int i = 0; i < cc_len[x]; ++i) {
				if (av == cpv) {
					// Set color
					color[pos + i][v] = color_noflag;
				}
				SendNotes(pos, i, v, av, x, acc[av]);
				SendNgraph(pos, i, v, x);
				SendComment(pos, v, av, x, i);
			}
			pos += cc_len[x];
		}

		pause_len = SendPause(pos, v);
		InterpolateNgraph(v, step, pos);
		// Merge notes
		MergeNotes(step, pos - 1, v);
	}
	step = pos + pause_len;
	FixLen(step0, step - 1);
	// Count additional variables
	CountOff(step0, step - 1);
	CountTime(step0, step - 1);
	UpdateNoteMinMax(step0, step - 1);
	UpdateTempoMinMax(step0, step - 1);
	++cantus_sent;
	// Create rule penalty string
	for (int x = 0; x < max_flags; ++x) {
		if (!accept[x] && fpenalty[x]) {
			st.Format("%d=%.0f", x, fpenalty[x]);
			if (rpst != "") rpst += ", ";
			rpst += st;
		}
	}
	st.Format("%.0f", rpenalty_cur);
	if (rpst != "") rpst = st + " (" + rpst + ")";
	else rpst = st;
	if (rpenalty_cur == MAX_PENALTY) rpst = "0";
	if (task == tGen) {
		if (!shuffle) {
			Adapt(step0, step - 1);
		}
		// If  window-scan
		st.Format("#%d\nCantus: %s\nRule penalty: %s", cantus_sent, cantus_high?"high":"low", rpst);
		AddMelody(step0, step - 1, svoice + 1, st);
	}
	else if (task == tEval) {
		if (m_algo_id == 101) {
			// If RSWA
			st.Format("#%d\nCantus: %s\nRule penalty: %s", cantus_sent, cantus_high ? "high" : "low", rpst);
		}
		else {
			if (key_eval == "") {
				// If SWA
				st.Format("#%d (from MIDI file %s)\nCantus: %s\nRule penalty: %s\nDistance penalty: %.0f", cantus_id+1, midi_file, cantus_high ? "high" : "low", rpst, dpenalty_cur);
			}
			else {
				// If evaluating
				st.Format("#%d (from MIDI file %s)\nCantus: %s\nRule penalty: %s\nKey selection: %s", cantus_id+1, midi_file, cantus_high ? "high" : "low", rpst, key_eval);
			}
		}
		AddMelody(step0, step - 1, svoice+1, st);
	}
	// Send
	t_generated = step;
	if (task == tGen) {
		if (!shuffle) {
			// Add line
			linecolor[t_sent] = Color(255, 0, 0, 0);
			t_sent = t_generated;
		}
	}
	st.Format("Sent: %ld (ignored %ld)", cantus_sent, cantus_ignored);
	SetStatusText(0, st);
	// Check limit
	if (t_generated >= t_cnt) {
		WriteLog(3, "Reached t_cnt steps. Generation stopped");
		return 1;
	}
	return 0;
}

void CGenCP1::ReseedCP()
{
	CString st;
	MultiCPInit();
	// Allow two seed cycles for each accept
	seed_cycle = 0;
	++reseed_count;
	st.Format("Reseed: %d", reseed_count);
	SetStatusText(4, st);
}

int CGenCP1::FailAlteredInt2(int i, int c1, int c2, int flag) {
	if ((apcc[0][i] == c1 && apcc[1][i] == c2) || (apcc[0][i] == c2 && apcc[1][i] == c1)) FLAG2(flag, i);
	return 0;
}

// Fail vertical altered intervals
int CGenCP1::FailAlteredInt() {
	for (int i = 0; i < ep2; ++i) {
		if (FailAlteredInt2(i, 9, 8, 170)) return 1;
		if (FailAlteredInt2(i, 11, 10, 171)) return 1;
		if (FailAlteredInt2(i, 11, 8, 172)) return 1;
		if (FailAlteredInt2(i, 9, 3, 173)) return 1;
		if (FailAlteredInt2(i, 11, 3, 174)) return 1;
	}
	return 0;
}

int CGenCP1::FailCrossInt2(int i, int i_1, int c1, int c2, int flag) {
	if ((apcc[cfv][i_1] == c1 && apcc[cpv][i] == c2) || (apcc[cfv][i_1] == c2 && apcc[cpv][i] == c1)) FLAG2(flag, i)
	else if ((apcc[cpv][i_1] == c1 && apcc[cfv][i] == c2) || (apcc[cpv][i_1] == c2 && apcc[cfv][i] == c1)) FLAG2(flag, i_1);
	return 0;
}

// Fail cross relation altered intervals
int CGenCP1::FailCrossInt() {
	int i, i_1;
	for (int x = 1; x < fli_size; ++x) {
		i = fli2[x];
		i_1 = fli2[x - 1];
		if (FailCrossInt2(i, i_1, 9, 8, 164)) return 1;
		if (FailCrossInt2(i, i_1, 11, 10, 165)) return 1;
		if (FailCrossInt2(i, i_1, 11, 8, 166)) return 1;
		if (FailCrossInt2(i, i_1, 9, 3, 167)) return 1;
		if (FailCrossInt2(i, i_1, 11, 3, 168)) return 1;
		if (FailCrossInt2(i, i_1, 11, 5, 29)) return 1;
		if (FailCrossInt2(i, i_1, 2, 8, 29)) return 1;
	}
	return 0;
}

void CGenCP1::GetVIntervals() {
	// Calculate intervals
	for (int i = 0; i < ep2; ++i) {
		ivl[i] = ac[1][i] - ac[0][i];
		ivlc[i] = ivl[i] % 7;
		civl[i] = acc[1][i] - acc[0][i];
		civlc[i] = civl[i] % 12;
		//if (civlc[i] == 1 || civlc[i] == 2 || civlc[i] == 5 || civlc[i] == 6 || civlc[i] == 10 || civlc[i] == 11) tivl[i] = iDis;
		if (civlc[i] == 3 || civlc[i] == 4 || civlc[i] == 8 || civlc[i] == 9) tivl[i] = iIco;
		else if (civlc[i] == 7 || civlc[i] == 0) tivl[i] = iPco;
		else tivl[i] = iDis;
	}
}

int CGenCP1::FailVMotion() {
	int mtemp;
	int scontra = 0;
	int sdirect = 0;
	for (int i = 0; i < ep2; ++i) {
		if (i < ep2 - 1) {
			motion[i] = mStay;
			if (acc[cfv][i + 1] != acc[cfv][i] || acc[cpv][i + 1] != acc[cpv][i]) {
				mtemp = (acc[cfv][i + 1] - acc[cfv][i])*(acc[cpv][i + 1] - acc[cpv][i]);
				if (mtemp > 0) {
					motion[i] = mDirect;
					++sdirect;
				}
				else if (mtemp < 0) {
					motion[i] = mContrary;
					++scontra;
				}
				else motion[i] = mOblique;
			}
		}
	}
	// Check how many contrary if full melody analyzed
	if (ep2 == c_len) {
		if (scontra + sdirect) {
			int pcontra = (scontra * 100) / (scontra + sdirect);
			if (pcontra < contrary_min2) FLAG2(46, 0)
			else if (pcontra < contrary_min) FLAG2(35, 0);
		}
	}
	return 0;
}

int CGenCP1::FailVIntervals() {
	int pico_count = 0;
	// Check first step
	if (tivl[0] == iDis) FLAG2(83, 0);
	for (ls = 1; ls < fli_size; ++ls) {
		s = fli[ls];
		s2 = fli2[ls];
		// Unison
		if (!civl[s]) {
			// Inside
			if (ls>1 && ls<fli_size-1) FLAG2(91, fli2[ls-1]);
		}
		// Discord
		if (tivl[s] == iDis) {
			// Downbeat
			if (!beat[ls]) FLAG2(83, s)
			// Upbeat
			else {
				// Check if movement to discord is smooth
				if (asmooth[cpv][fli2[ls-1]]) FLAG2(169, s)
				// If movement to discord is leaping
				else FLAG2(187, s);
			}
		}
		else {
			// Check if previous interval is discord
			if (tivl[fli2[ls - 1]] == iDis) {
				// Check if movement from discord is not smooth
				if (!asmooth[cpv][fli2[ls - 1]]) FLAG2(88, fli2[ls-1]);
			}
		}
		// Perfect consonance
		if (tivl[s] == iPco) {
			// Prohibit parallel 
			if (civl[s] == civl[fli2[ls - 1]]) FLAG2(84, s)
			// Prohibit combinatory
			else if (civlc[s] == civlc[fli2[ls - 1]]) FLAG2(85, s)
			// Prohibit different
			else if (tivl[fli2[ls-1]] == iPco) FLAG2(86, s)
			// All other cases if previous interval is not pco
			else {
				// Direct movement to pco
				if (motion[fli2[ls - 1]] == mDirect) {
					// Last movement with stepwize
					if (s2 == c_len-1 && (abs(acc[cpv][s]-acc[cpv][s-1]) < 3 || abs(acc[cfv][s]-acc[cfv][s-1]) < 3))
						FLAG2(33, s)
					// Other cases
					else FLAG2(87, s);
				}
				// Prohibit downbeats and culminations only if not last step
				if (ls < fli_size - 1) {
					if (beat[ls]) {
						// Prohibit culmination
						if (acc[cpv][s] == nmax || acc[cfv][s] == nmax) FLAG2(81, s);
					}
					else {
						// Prohibit downbeat culmination
						if (acc[cpv][s] == nmax || acc[cfv][s] == nmax) FLAG2(82, s)
						// Prohibit downbeat
						else FLAG2(80, s);
					}
				}
			}
		}
		// Long parallel ico
		if (tivl[s] == iIco && ivl[s] == ivl[fli2[ls - 1]]) {
			++pico_count;
			// Two same ico transitions means three intervals already
			if (pico_count == ico_chain-1) {
				FLAG2(89, s)
			}
			else if (pico_count >= ico_chain2) {
				FLAG2(96, s)
			}
		}
		else pico_count = 0;
	}
	return 0;
}

void CGenCP1::CalcDpenaltyCP() {
	dpenalty_cur = 0;
	for (int z = 0; z < c_len; z++) {
		int dif = abs(acc_old[cpv][z] - acc[cpv][z]);
		if (dif) dpenalty_cur += step_penalty + pitch_penalty * dif;
	}
}

void CGenCP1::SaveCP() {
	// If rpenalty is same as min, calculate dpenalty
	if (optimize_dpenalty) {
		if (rpenalty_cur == rpenalty_min) {
			CalcDpenaltyCP();
			// Do not save cantus if it has higher dpenalty
			if (dpenalty_cur > dpenalty_min) return;
			// Do not save cantus if it is same as source
			if (!dpenalty_cur) return;
			dpenalty_min = dpenalty_cur;
		}
		// If rpenalty lowered, clear dpenalty
		else {
			dpenalty_min = MAX_PENALTY;
			dpenalty_cur = MAX_PENALTY;
		}
		dpenalty.push_back(dpenalty_cur);
	}
	clib.push_back(acc[cpv]);
	rpenalty.push_back(rpenalty_cur);
	rpenalty_min = rpenalty_cur;
}

void CGenCP1::SaveCPIfRp() {
	// Is penalty not greater than minimum of all previous?
	if (rpenalty_cur <= rpenalty_min) {
		// If rpenalty 0, we can skip_flags (if allowed)
		if (!skip_flags && rpenalty_cur == 0)
			skip_flags = !calculate_blocking && !calculate_correlation && !calculate_stat;
		// Insert only if cc is unique
		if (clib_vs.Insert(acc[cpv]))
			SaveCP();
		// Save flags for SWA stuck flags
		if (rpenalty_cur) best_flags = flags;
	}
}

// Detect repeating notes. Step2 excluding
int CGenCP1::FailSlurs(vector<int> &cc, int step1, int step2) {
  // Number of sequential slurs 
	int scount = 0;
	// Number of slurs in window
	int scount2 = 0;
	for (int i = step1; i < step2; ++i) {
		if (cc[i] == cc[i + 1]) {
		  // Check simultaneous slurs
			//if (acc[cfv][i] == acc[cfv][i + 1]) {
				//FLAG2(98, i);
			//}
			// Check slurs sequence
			++scount;
			if (scount > 1) FLAG2(97, i);
			// Check slurs in window
			++scount2;
			// Subtract old slur
			if ((i >= slurs_window) && (cc[i - slurs_window] == cc[i - slurs_window + 1])) --scount2;
			if (scount2 == 1) FLAG2(93, i)
			else if (scount2 == 2) FLAG2(94, i)
			else if (scount2 > 2) FLAG2(95, i);
		}
		else scount = 0;
	}
	return 0;
}

// Count limits
int CGenCP1::FailCPInterval() {
	int bsteps = 0;
	int step = 0;
	for (int i = 0; i < fli_size; ++i) {
		step = fli2[i];
		// Check between
		if (acc[1][step] - acc[0][step] > max_between) {
			++bsteps;
			// Flag very far burst
			if (acc[1][step] - acc[0][step] > burst_between) FLAG2(11, step);
			if (bsteps > burst_steps) {
				// Flag long burst only on first overrun
				if (bsteps == burst_steps + 1) FLAG2(11, step)
					// Next overruns are sent to fpenalty
				else fpenalty[37] += bsteps - burst_steps;
			}
		}
		else bsteps = 0;
	}
	return 0;
}

// Find situations when one voice goes over previous note of another voice
int CGenCP1::FailOverlap() {
	if (cantus_high) {
		for (int i = ep1; i < ep2; ++i) {
			if (i > 0 && acc[cpv][i] >= acc[cfv][i - 1]) FLAG2(24, i)
			else if (i < c_len - 1 && acc[cpv][i] >= acc[cfv][i + 1]) FLAG2(24, i);
		}
	}
	else {
		for (int i = ep1; i < ep2; ++i) {
			if (i > 0 && acc[cpv][i] <= acc[cfv][i - 1]) FLAG2(24, i)
			else if (i < c_len - 1 && acc[cpv][i] <= acc[cfv][i + 1]) FLAG2(24, i);
		}
	}
	return 0;
}

// Create random cantus and optimize it using SWA
void CGenCP1::RandomSWACP()
{
	CString st;
	VSet<int> vs; // Unique checker
								// Disable debug flags
	calculate_blocking = 0;
	calculate_correlation = 0;
	calculate_stat = 0;
	// Create single cantus
	cpoint.resize(1);
	cpoint[0].resize(2);
	cpoint[0][cpv] = acc[cfv];
	cpoint[0][cfv] = acc[cfv];
	scpoint = cpoint[0];
	ScanCPInit();
	// Set random_seed to initiate random cantus
	random_seed = 1;
	// Set random_range to limit scanning to one of possible fast-scan ranges
	random_range = 1;
	// Prohibit limits recalculation during SWA
	swa_inrange = 1;
	for (int i = 0; i < INT_MAX; ++i) {
		if (need_exit) break;
		// Create random cantus
		MakeNewCP();
		scpoint[cpv] = acc[cpv];
		// Set scan matrix to scan all
		smatrixc = c_len - fn;
		smatrix.clear();
		smatrix.resize(c_len, 0);
		// Do not scan first pause
		for (int x = fn; x < c_len; ++x) {
			smatrix[x] = 1;
		}
		// Optimize cpoint
		rpenalty_cur = MAX_PENALTY;
		SWACP(0, 0);
		// Show cantus if it is perfect
		if (rpenalty_min <= rpenalty_accepted) {
			if (vs.Insert(acc[cpv])) {
				int step = t_generated;
				// Add line
				linecolor[t_generated] = Color(255, 0, 0, 0);
				scpoint = acc;
				ScanCP(tEval, 0);
				Adapt(step, t_generated - 1);
				t_sent = t_generated;
			}
			else {
				++cantus_ignored;
			}
		}
		st.Format("Random SWACP: %d", i);
		SetStatusText(3, st);
		st.Format("Sent: %ld (ignored %ld)", cantus_sent, cantus_ignored);
		SetStatusText(0, st);
		//SendCantus(0, 0);
		// Check limit
		if (t_generated >= t_cnt) {
			WriteLog(3, "Reached t_cnt steps. Generation stopped");
			return;
		}
	}
	ShowStuck();
}

// Do not calculate dpenalty (dp = 0). Calculate dpenalty (dp = 1).
void CGenCP1::SWACP(int i, int dp) {
	CString st;
	int time_start = CGLib::time();
	s_len = 1;
	// Save source rpenalty
	float rpenalty_source = rpenalty_cur;
	long cnum = 0;
	// Save cantus only if its penalty is less or equal to source rpenalty
	rpenalty_min = rpenalty_cur;
	dpenalty_min = MAX_PENALTY;
	acc = cpoint[i];
	int a;
	for (a = 0; a < approximations; a++) {
		// Save previous minimum penalty
		int rpenalty_min_old = rpenalty_min;
		int dpenalty_min_old = dpenalty_min;
		// Clear before scan
		clib.clear();
		clib_vs.clear();
		rpenalty.clear();
		dpenalty.clear();
		dpenalty_min = MAX_PENALTY;
		clib.push_back(acc[cpv]);
		clib_vs.Insert(acc[cpv]);
		rpenalty.push_back(rpenalty_min_old);
		dpenalty.push_back(dpenalty_min_old);
		// Sliding Windows Approximation
		scpoint = acc;
		ScanCP(tCor, -1);
		dpenalty_min = MAX_PENALTY;
		cnum = clib.size();
		if (cnum) {
			if (dp) {
				// Count dpenalty for results, where rpenalty is minimal
				dpenalty.resize(cnum);
				for (int x = 0; x < cnum; x++) if (rpenalty[x] <= rpenalty_min) {
					dpenalty[x] = 0;
					for (int z = 0; z < c_len; z++) {
						int dif = abs(cpoint[i][1][z] - clib[x][z]);
						if (dif) dpenalty[x] += step_penalty + pitch_penalty * dif;
					}
					if (dpenalty[x] && dpenalty[x] < dpenalty_min) dpenalty_min = dpenalty[x];
					//st.Format("rp %.0f, dp %0.f: ", rpenalty[x], dpenalty[x]);
					//AppendLineToFile("temp.log", st);
					//LogCantus(clib[x]);
				}
			}
			// Get all best corrections
			cids.clear();
			for (int x = 0; x < cnum; x++) if (rpenalty[x] <= rpenalty_min && (!dp || dpenalty[x] <= dpenalty_min)) {
				cids.push_back(x);
			}
			if (cids.size()) {
				// Get random cid
				int cid = randbw(0, cids.size() - 1);
				// Get random cantus to continue
				acc[cpv] = clib[cids[cid]];
			}
		}
		// Send log
		if (debug_level > 1) {
			CString est;
			est.Format("SWA%d #%d: rp %.0f from %.0f, dp %.0f, cnum %ld", s_len, a, rpenalty_min, rpenalty_source, dpenalty_min, cnum);
			WriteLog(3, est);
		}
		if (acc[cfv].size() > 60) {
			st.Format("SWA%d attempt: %d", s_len, a);
			SetStatusText(4, st);
		}
		if (dp) {
			// Abort SWA if dpenalty and rpenalty not decreasing
			if (rpenalty_min >= rpenalty_min_old && dpenalty_min >= dpenalty_min_old) {
				if (s_len >= swa_steps)	break;
				++s_len;
			}
		}
		else {
			// Abort SWA if rpenalty zero or not decreasing
			if (!rpenalty_min) break;
			if (rpenalty_min >= rpenalty_min_old) {
				if (s_len >= swa_steps) {
					// Record SWA stuck flags
					for (int x = 0; x < max_flags; ++x) {
						if (best_flags[x]) ++ssf[x];
					}
					break;
				}
				else ++s_len;
			}
		}
	}
	// Log
	int time_stop = CGLib::time();
	CString est;
	CString sst = GetStuck();
	est.Format("Finished SWA%d #%d: rp %.0f from %.0f, dp %.0f, cnum %ld (in %d ms): %s",
		s_len, a, rpenalty_min, rpenalty_source, dpenalty_min, cnum, time_stop - time_start, sst);
	WriteLog(3, est);
}

int CGenCP1::FailLastIntervals(vector<int> &pc, int ep2) {
	// Prohibit last note not tonic
	if (ep2 > c_len - 1) {
		if (pc[c_len - 1] != 0) FLAG2(50, c_len - 1);
		// Scan 2nd to last measure
		int start = mli[mli.size() - 2];
		int b_found = 0;
		int g_found = 0;
		int d_found = 0;
		for (int s = start; s < start + npm; ++s) {
			for (int v = 0; v < av_cnt; ++v) {
				if (apc[v][s] == 6) b_found = 1;
				if (apc[v][s] == 4) g_found = 1;
				if (apc[v][s] == 1) d_found = 1;
			}
		}
		if (!b_found) FLAG2(47, start);
		if (!g_found && !d_found) FLAG2(75, start);
	}
	return 0;
}

void CGenCP1::GetNoteTypes() {
	int s = 0;
	int l;
	for (ls = 0; ls < fli_size; ++ls) {
		if (ls > 0) s = fli2[ls-1]+1;
		l = llen[ls];
		// Get beat
		if (s % npm) {
			if (npm>2 && s % (npm / 2)) {
				if (npm>4 && s % (npm / 4)) {
					beat[ls] = 3;
				}
				else beat[ls] = 2;
			}
			else beat[ls] = 1;
		}
		else beat[ls] = 0;
		// Get suspension
		sus[ls] = 0;
		if (l > 1) {
			for (int i = 1; i < l; ++i) {
				if (acc[cfv][s + i] != acc[cfv][s + i - 1]) {
					sus[ls] = 1;
					break;
				}
			}
		}
	}
}

void CGenCP1::GetMeasures() {
	mli.clear();
	for (int i = 0; i < ep2; ++i) {
		// Find measures
		if (i % npm == 0) {
			mli.push_back(i);
		}
	}
}

void CGenCP1::ScanCP(int t, int v) {
	CString st, st2;
	int finished = 0;
	// Load master parameters
	task = t;
	svoice = v;

	ScanCPInit();
	if (task == tGen) MultiCPInit();
	else SingleCPInit();
	if (FailWindowsLimit()) return;
	// Need to clear flags, because if skip_flags, they can contain previous prohibited flags
	fill(flags.begin(), flags.end(), 0);
	// Analyze combination
check:
	while (true) {
		// First pause
		for (int i = 0; i < fn; ++i) acc[cpv][i] = acc[cpv][fn];
		//LogCantus(acc[cpv]);
		GetMelodyInterval(acc[cpv], 0, ep2, nmin, nmax);
		// Limit melody interval
		if (task == tGen) {
			if (nmax - nmin > max_interval) goto skip;
			if (c_len == ep2 && nmax - nmin < min_interval) goto skip;
			if (cantus_high) {
				if (cf_nmax - nmin > sum_interval) goto skip;
			}
			else {
				if (nmax - cf_nmin > sum_interval) goto skip;
			}
			ClearFlags(0, ep2);
		}
		else {
			ClearFlags(0, ep2);
			if (nmax - nmin > max_interval) FLAG(37, 0);
			if (cantus_high) {
				if (cf_nmax - nmin > sum_interval) FLAG(7, 0);
			}
			else {
				if (nmax - cf_nmin > sum_interval) FLAG(7, 0);
			}
			if (c_len == ep2 && nmax - nmin < min_interval) FLAG(38, 0);
		}
		if (FailSlurs(acc[cpv], 0, ep2 - 1)) goto skip;
		++accepted3;
		if (need_exit && task != tEval) break;
		// Show status
		if (FailDiatonic(ac[cpv], acc[cpv], 0, ep2, minor_cur)) goto skip;
		GetPitchClass(ac[cpv], acc[cpv], apc[cpv], apcc[cpv], 0, ep2);
		if (minor_cur) {
			if (FailMinor(apcc[cpv], acc[cpv])) goto skip;
			if (FailGisTrail(apcc[cpv])) goto skip;
		}
		//if (MatchVectors(acc[cpv], test_cc, 0, 2)) 
		//WriteLog(1, "Found");
		CreateLinks(acc[cpv]);
		if (FailCPInterval()) goto skip;
		GetMeasures();
		if (FailTonic(acc[cpv], apc[cpv])) goto skip;
		if (FailLastIntervals(apc[cpv], ep2)) goto skip;
		if (FailNoteSeq(apc[cpv])) goto skip;
		if (FailIntervals(ac[cpv], acc[cpv], apc[cpv], apcc[cpv])) goto skip;
		if (FailLeapSmooth(ac[cpv], acc[cpv], ep2, aleap[cpv], asmooth[cpv], aslur[cpv])) goto skip;
		if (FailOutstandingRepeat(ac[cpv], acc[cpv], aleap[cpv], repeat_steps2, 2, 76)) goto skip;
		if (FailOutstandingRepeat(ac[cpv], acc[cpv], aleap[cpv], repeat_steps3, 3, 36)) goto skip;
		if (FailLongRepeat(acc[cpv], aleap[cpv], repeat_steps5, 5, 72)) goto skip;
		if (FailLongRepeat(acc[cpv], aleap[cpv], repeat_steps7, 7, 73)) goto skip;
		// Calculate diatonic limits
		nmind = CC_C(nmin, tonic_cur, minor_cur);
		nmaxd = CC_C(nmax, tonic_cur, minor_cur);
		if (FailGlobalFill(ac[cpv], ep2, nstat2)) goto skip;
		if (FailLocalRange(acc[cpv], notes_lrange, min_lrange, 98)) goto skip;
		if (FailLocalRange(acc[cpv], notes_lrange2, min_lrange2, 198)) goto skip;
		GetNoteTypes();
		if (FailAlteredInt()) goto skip;
		if (FailCrossInt()) goto skip;
		GetVIntervals();
		if (FailVMotion()) goto skip;
		if (FailVIntervals()) goto skip;
		if (FailOverlap()) goto skip;
		if (FailStagnation(acc[cpv], nstat)) goto skip;
		if (FailMultiCulm(acc[cpv], aslur[cpv])) goto skip;
		if (FailFirstNotes(apc[cpv])) goto skip;
		if (FailLeap(ac[cpv], ep2, aleap[cpv], asmooth[cpv], nstat2, nstat3)) goto skip;
		//if (FailMelodyHarm(apc[cpv], 0, ep2)) goto skip;
		MakeMacc(acc[cpv]);
		if (FailLocalMacc(notes_arange, min_arange, 15)) goto skip;
		if (FailLocalMacc(notes_arange2, min_arange2, 16)) goto skip;

		SaveBestRejected(acc[cpv]);
		// If we are window-scanning
		if ((task == tGen || task == tCor) && method == mScan) {
			++accepted2;
			CalcFlagStat();
			if (FailFlagBlock()) goto skip;
			if (FailAccept()) goto skip;
			++accepted4[wid];
			//LogCantus(acc[cpv]);
			// If this is not last window, go to next window
			if (ep2 < c_len) {
				NextWindow();
				goto check;
			}
			// Check random_choose
			if (random_choose < 100) if (rand2() >= (float)RAND_MAX*random_choose / 100.0) goto skip;
		}
		// Calculate rules penalty if we evaluate or correct cantus without full scan
		else {
			CalcRpenalty(acc[cpv]);
		}
		// Accept cantus
		++accepted;
		TimeBestRejected();
		if (method == mScan && task == tCor) {
			SaveCP();
		}
		else if (method == mSWA && task == tCor) {
			SaveCPIfRp();
		}
		else {
			// Put rule debug lines here:
			//if (FailVIntervals()) goto skip;
			if (task == tGen && accept_reseed) {
				if (clib_vs.Insert(acc[cpv])) {
					if (SendCP()) break;
					ReseedCP();
					// Start evaluating without scan
					goto check;
				}
				else {
					++cantus_ignored;
				}
			}
			else {
				if (SendCP()) break;
			}
			// Exit if this is evaluation
			if (task == tEval) return;
		}
	skip:
		ScanLeft(acc[cpv], finished);
		if (finished) {
			// Clear flag to prevent coming here again
			finished = 0;
			// Sliding Windows Approximation
			if (method == mSWA) {
				if (NextSWA(acc[cpv], acc_old[cpv])) break;
				goto check;
			}
			// Finish if this is last variant in first window and not SWA
			else if ((p == 0) || (wid == 0)) {
				// If we started from random seed, allow one more full cycle
				if (random_seed) {
					if (seed_cycle) {
						// Infinitely cycle through ranges
						if (random_range && accept_reseed) {
							ReseedCP();
							// Start evaluating without scan
							goto check;
						}
						break;
					}
					// Dont show log if we are reseeding after each accept
					if (!accept_reseed) WriteLog(3, "Random seed allows one more full cycle: restarting");
					++seed_cycle;
				}
				else break;
			}
			BackWindow(acc[cpv]);
			// Goto next variant calculation
			goto skip;
		} // if (finished)
		ScanRight(acc[cpv]);
	}
	if (accepted3 > 100000) ShowScanStatus(acc[cpv]);
	WriteFlagCor();
	ShowFlagStat();
	ShowFlagBlock();
}

void CGenCP1::Generate() {
	CString st;
	if (InitCP()) return;
	LoadCantus(midi_file);
	if (cantus.size() < 1) return;
	// Choose cantus to use
	cantus_id = randbw(0, cantus.size() - 1);
	if (cantus_id2) {
		if (cantus_id2 <= cantus.size()) {
			cantus_id = cantus_id2 - 1;
		}
		else {
			CString est;
			est.Format("Warning: cantus_id in configuration file (%d) is greater than number of canti loaded (%d). Selecting highest cantus.", 
				cantus_id2, cantus.size());
			WriteLog(1, est);
			cantus_id = cantus.size() - 1;
		}
	}
	c_len = cantus[cantus_id].size();
	// Get key
	GetCantusKey(cantus[cantus_id]);
	// Get cantus interval
	GetMelodyInterval(cantus[cantus_id], 0, cantus[cantus_id].size(), cf_nmin, cf_nmax);
	if (tonic_cur == -1) return;
	CalcCcIncrement();
	// Show imported melody
	cc_len = cantus_len[cantus_id];
	cc_tempo = cantus_tempo[cantus_id];
	real_len = accumulate(cantus_len[cantus_id].begin(), cantus_len[cantus_id].end(), 0);
	dpenalty_cur = 0;
	// Create pause
	FillPause(0, floor(real_len/8+1)*8, 1);
	// Select rule set
	SelectRuleSet(cf_rule_set);
	ScanCantus(tEval, 0, &(cantus[cantus_id]));
	// Show cantus id
	st.Format("Cantus %d. ", cantus_id + 1);
	comment[0][0] = st + comment[0][0];
	// Go forward
	Adapt(0, step - 1);
	t_generated = step;
	t_sent = t_generated;
	// Choose level
	if (cantus_high) {
		cpv = 0;
		cfv = 1;
	}
	else {
		cpv = 1;
		cfv = 0;
	}
	// Load first voice
	vector<int> cc_len_old = cc_len;
	vector<float> cc_tempo_old = cc_tempo;
	vector<int> anflagsc_old = anflagsc[cfv];
	vector<vector<int>> anflags_old = anflags[cfv];
	c_len = m_c.size() * npm - (npm - 1); 
	ac[cfv].clear();
	acc[cfv].clear();
	apc[cfv].clear();
	apcc[cfv].clear();
	anflags[cfv].clear();
	anflagsc[cfv].clear();
	// Create empty arrays
	anflags[cfv].resize(m_c.size()*npm);
	anflagsc[cfv].resize(m_c.size()*npm);
	int npm2 = npm;
	for (int i = 0; i < m_c.size(); ++i) {
		// Last measure should have one note
		if (i == m_c.size() - 1) npm2 = 1;
		for (int x = 0; x < npm2; ++x) {
			ac[cfv].push_back(m_c[i]);
			acc[cfv].push_back(m_cc[i]);
			apc[cfv].push_back(m_pc[i]);
			apcc[cfv].push_back(m_pcc[i]);
			cc_len.push_back(cc_len_old[i]*npm/npm2);
			cc_tempo.push_back(cc_tempo_old[i]);
			if (!x) {
				int y = i*npm + x;
				anflagsc[cfv][y] = anflagsc_old[i];
				anflags[cfv][y] = anflags_old[i];
			}
		}
	}
	// Generate second voice
	rpenalty_cur = MAX_PENALTY;
	if (SelectRuleSet(cp_rule_set)) return;
	if (method == mSWA) {
		RandomSWACP();
	}
	else {
		ScanCP(tGen, 0);
	}
}
