// SMPS Definition File Loader
// ---------------------------
// Written by Valley Bell, 2014
#define _CRTDBG_MAP_ALLOC	// note: no effect in Release builds
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>	// for isalpha()
#include <malloc.h>
#include <memory.h>
#include <string.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "stdtype.h"
#include "ini_lib.h"
#include "Engine/smps_structs.h"
#include "Engine/smps_commands.h"

typedef struct _option_list
{
	const char* Text;
	UINT8 Value;
} OPT_LIST;


static UINT8 GetOptionValue(const OPT_LIST* OptList, const char* ValueStr);
//void LoadDriverDefinition(const char* FileName, SMPS_CFG* SmpsCfg);
static void LoadRegisterList(SMPS_CFG* SmpsCfg, UINT8 CstRegCnt, const UINT8* CstRegList);
//void FreeDriverDefinition(SMPS_CFG* SmpsCfg);
static void ApplyCommandFlags(UINT8 FlagCnt, const UINT8* IDList, const CMD_FLAGS* CFBuffer, CMD_LIB* CFLib);
//void LoadCommandDefinition(const char* FileName, SMPS_CFG* SmpsCfg);
//void FreeCommandDefinition(SMPS_CFG* SmpsCfg);
//void LoadDrumDefinition(const char* FileName, DRUM_LIB* DrumDef);
static UINT8 GetShiftFromMask(UINT8 Mask);
//void FreeDrumDefinition(DRUM_LIB* DrumDef);
//void LoadPSGDrumDefinition(const char* FileName, PSG_DRUM_LIB* DrumDef);
//void FreePSGDrumDefinition(PSG_DRUM_LIB* DrumDef);



static const OPT_LIST OPT_PTRFMT[] =
{	{"68K", PTRFMT_68K},
	{"RST", PTRFMT_RST},
	{"Z80", PTRFMT_Z80},
	{NULL, 0}};
static const OPT_LIST OPT_INSMODE[] =
{	{"DEFAULT", INSMODE_DEF},
	{"HARDWARE", INSMODE_HW},
	{"CUSTOM", INSMODE_CST},
	{"INTERLEAVED", INSMODE_INT},
	{NULL, 0}};
static const OPT_LIST OPT_TEMPOMODE[] =
{	{"TIMEOUT", TEMPO_TIMEOUT},
	{"OVERFLOW", TEMPO_OVERFLOW},
	{"OVERFLOW2", TEMPO_OVERFLOW2},
	{"TOUT+OFLW", TEMPO_TOUT_OFLW},
	{"OFLW_MULTI", TEMPO_OFLW_MULT},
	{NULL, 0}};
static const OPT_LIST OPT_FMBASENOTE[] =
{	{"B", FMBASEN_B},
	{"C", FMBASEN_C},
	{NULL, 0}};
static const OPT_LIST OPT_PSGBASENOTE[] =
{	{"C", PSGBASEN_C},
	{"B", PSGBASEN_B},
	{NULL, 0}};
static const OPT_LIST OPT_DELAYFREQ[] =
{	{"RESET", DLYFREQ_RESET},
	{"KEEP", DLYFREQ_KEEP},
	{NULL, 0}};
static const OPT_LIST OPT_NONPREVENT[] =
{	{"HOLD", NONPREV_HOLD},
	{"REST", NONPREV_REST},
	{NULL, 0}};
static const OPT_LIST OPT_MODALGO[] =
{	{"68K", MODULAT_68K},
	{"68K_a", MODULAT_68K_1},
	{"Z80", MODULAT_Z80},
	{NULL, 0}};
static const OPT_LIST OPT_ENVMULT[] =
{	{"PRE", ENVMULT_PRE},
	{"68K", ENVMULT_68K},
	{"Z80", ENVMULT_Z80},
	{NULL, 0}};
static const OPT_LIST OPT_VOLMODE[] =
{	{"ALGO", VOLMODE_ALGO},
	{"BIT7", VOLMODE_BIT7},
	{"ALGOSET", VOLMODE_ALGO | VOLMODE_SETVOL},
	{NULL, 0}};
static const OPT_LIST OPT_DRMCHNMODE[] =
{	{"NORMAL", DCHNMODE_NORMAL},
	{"PS4", DCHNMODE_PS4},
	{"CYMN", DCHNMODE_CYMN},
	{"GAXE3", DCHNMODE_GAXE3},
	{"VRDLX", DCHNMODE_VRDLX},
	{NULL, 0}};
static const OPT_LIST OPT_FREQVALS_FM[] =
{	{"DEF_68K", 1},
	{"DEF_Z80", 2},
	{NULL, 0}};
static const OPT_LIST OPT_FREQVALS_PSG[] =
{	{"DEF_PRE", 1},
	{"DEF_68K", 2},
	{"DEF_Z80_T1", 3},
	{"DEF_Z80_T2", 4},
	{NULL, 0}};
static const OPT_LIST OPT_ENVCMD[] =
{	{"DATA", ENVCMD_DATA},
	{"RESET", ENVCMD_RESET},
	{"HOLD", ENVCMD_HOLD},
	{"LOOP", ENVCMD_LOOP},
	{"STOP", ENVCMD_STOP},
	{"CHG_MULT", ENVCMD_CHGMULT},
	{NULL, 0}};

static const OPT_LIST OPT_DRUMMODE[] =
{	{"Normal", DRMMODE_NORMAL},
	{"Dual", DRMMODE_DUAL},
	{NULL, DRMMODE_DUAL}};
static const OPT_LIST OPT_DRUMTYPE[] =
{	{"FM", DRMTYPE_FM},
	{"PSG", DRMTYPE_PSG},
	{"DAC", DRMTYPE_DAC},
	{NULL, DRMTYPE_NONE}};

static const UINT16 DEF_FMFREQ_VAL[13] =
	{0x25E, 0x284, 0x2AB, 0x2D3, 0x2FE, 0x32D, 0x35C, 0x38F, 0x3C5, 0x3FF, 0x43C, 0x47C, 0x4C0};
#define DEF_FMFREQ_68K_CNT	12
#define DEF_FMFREQ_Z80_CNT	12
#define DEF_FMFREQ_68K_VAL	&DEF_FMFREQ_VAL[0]	// I'd use "static const UINT16*", but VS2010 doesn't
#define DEF_FMFREQ_Z80_VAL	&DEF_FMFREQ_VAL[1]	// like me to use that in the array below

#define DEF_PSGFREQ_PRE_CNT		84
static const UINT16 DEF_PSGFREQ_PRE_VAL[DEF_PSGFREQ_PRE_CNT] =
{	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x3F8, 0x3BF, 0x389,
	0x356, 0x326, 0x2F9, 0x2CE, 0x2A5, 0x280, 0x25C, 0x23A, 0x21A, 0x1FB, 0x1DF, 0x1C4,
	0x1AB, 0x193, 0x17D, 0x167, 0x153, 0x140, 0x12E, 0x11D, 0x10D, 0x0FE, 0x0EF, 0x0E2,
	0x0D6, 0x0C9, 0x0BE, 0x0B4, 0x0A9, 0x0A0, 0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071,
	0x06B, 0x065, 0x05F, 0x05A, 0x055, 0x050, 0x04B, 0x047, 0x043, 0x040, 0x03C, 0x039,
	0x036, 0x033, 0x030, 0x02D, 0x02B, 0x028, 0x026, 0x024, 0x022, 0x020, 0x01F, 0x01D,
	0x01B, 0x01A, 0x018, 0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x000};

#define DEF_PSGFREQ_68K_CNT		70
#define DEF_PSGFREQ_68K_VAL		&DEF_PSGFREQ_PRE_VAL[12]

#define DEF_PSGFREQ_Z80_T1_CNT	69
#define DEF_PSGFREQ_Z80_T1_VAL	DEF_PSGFREQ_68K_VAL

#define DEF_PSGFREQ_Z80_T2_CNT	84
static const UINT16 DEF_PSGFREQ_Z80_T2_VAL[DEF_PSGFREQ_Z80_T2_CNT] =
{	0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3FF, 0x3F7, 0x3BE, 0x388,
	0x356, 0x326, 0x2F9, 0x2CE, 0x2A5, 0x280, 0x25C, 0x23A, 0x21A, 0x1FB, 0x1DF, 0x1C4,
	0x1AB, 0x193, 0x17D, 0x167, 0x153, 0x140, 0x12E, 0x11D, 0x10D, 0x0FE, 0x0EF, 0x0E2,
	0x0D6, 0x0C9, 0x0BE, 0x0B4, 0x0A9, 0x0A0, 0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071,
	0x06B, 0x065, 0x05F, 0x05A, 0x055, 0x050, 0x04B, 0x047, 0x043, 0x040, 0x03C, 0x039,
	0x036, 0x033, 0x030, 0x02D, 0x02B, 0x028, 0x026, 0x024, 0x022, 0x020, 0x01F, 0x01D,
	0x01B, 0x01A, 0x018, 0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x010, 0x000, 0x000};

static const UINT8 DEF_FREQFM_CNT[2] = {DEF_FMFREQ_68K_CNT, DEF_FMFREQ_Z80_CNT};
static const UINT16* DEF_FREQFM_PTR[2] = {DEF_FMFREQ_68K_VAL, DEF_FMFREQ_Z80_VAL};
static const UINT8 DEF_FREQPSG_CNT[4] = {	DEF_PSGFREQ_PRE_CNT, DEF_PSGFREQ_68K_CNT,
											DEF_PSGFREQ_Z80_T1_CNT, DEF_PSGFREQ_Z80_T2_CNT};
static const UINT16* DEF_FREQPSG_PTR[4] = {	DEF_PSGFREQ_PRE_VAL, DEF_PSGFREQ_68K_VAL,
											DEF_PSGFREQ_Z80_T1_VAL, DEF_PSGFREQ_Z80_T2_VAL};


static const UINT8 INSOPS_DEFAULT[] =
{	0xB0,
	0x30, 0x38, 0x34, 0x3C,
	0x50, 0x58, 0x54, 0x5C,
	0x60, 0x68, 0x64, 0x6C,
	0x70, 0x78, 0x74, 0x7C,
	0x80, 0x88, 0x84, 0x8C,
	0x40, 0x48, 0x44, 0x4C};
static const UINT8 INSOPS_HARDWARE[] =
{	0xB0,
	0x30, 0x34, 0x38, 0x3C,
	0x50, 0x54, 0x58, 0x5C,
	0x60, 0x64, 0x68, 0x6C,
	0x70, 0x74, 0x78, 0x7C,
	0x80, 0x84, 0x88, 0x8C,
	0x40, 0x44, 0x48, 0x4C};

static const UINT8 FMCHN_ORDER[7] = {0x16, 0, 1, 2, 4, 5, 6};
static const UINT8 PSGCHN_ORDER[3] = {0x80, 0xA0, 0xC0};

static const OPT_LIST OPT_CFLAGS[] =
{	{"IGNORE", CF_IGNORE},
	{"INVALID", CF_INVALID},
	
	{"PANAFMS", CF_PANAFMS},
	{"DETUNE", CF_DETUNE},
	{"SET_COMM", CF_SET_COMM},
	{"VOLUME", CF_VOLUME},
	{"HOLD", CF_HOLD},
	{"NOTE_STOP", CF_NOTE_STOP},
	{"TRANSPOSE", CF_TRANSPOSE},
	{"INSTRUMENT", CF_INSTRUMENT},
	{"PSG_NOISE", CF_PSG_NOISE},
	{"FM_COMMAND", CF_FM_COMMAND},
	{"SND_CMD", CF_SND_CMD},
	{"PLAY_DAC", CF_PLAY_DAC},
	{"PLAY_PWM", CF_PLAY_PWM},
	{"DAC_BANK", CF_DAC_BANK},
	
	{"MOD_SETUP", CF_MOD_SETUP},
	{"MOD_SET", CF_MOD_SET},
	{"MOD_ENV", CF_MOD_ENV},
	{"FM_VOLENV", CF_FM_VOLENV},
	{"LFO_MOD", CF_LFO_MOD},
	{"ADSR", CF_ADSR},
	
	{"PAN_ANIM", CF_PAN_ANIM},
	{"SET_LFO", CF_SET_LFO},
	{"SET_LFO_SPD", CF_SET_LFO_SPD},
	{"PITCH_SLIDE", CF_PITCH_SLIDE},
	{"RAW_FREQ", CF_RAW_FREQ},
	{"SPC_FM3", CF_SPC_FM3},
	{"SSG_EG", CF_SSG_EG},
	
	{"TEMPO", CF_TEMPO},
	{"TICK_MULT", CF_TICK_MULT},
	{"TIMING", CF_TIMING},
	{"TIMING_MODE", CF_TIMING_MODE},
	
	{"MUS_PAUSE", CF_MUS_PAUSE},
	{"COPY_MEM", CF_COPY_MEM},
	{"FADE_IN", CF_FADE_IN},
	
	{"GOTO", CF_GOTO},
	{"LOOP", CF_LOOP},
	{"GOSUB", CF_GOSUB},
	{"RETURN", CF_RETURN},
	{"LOOP_EXIT", CF_LOOP_EXIT},
	{"COND_JUMP", CF_COND_JUMP},
	{"META_CF", CF_META_CF},
	{"TRK_END", CF_TRK_END},
	
	{"FADE_IN_SONG", CF_FADE_IN_SONG},
	{"SND_OFF", CF_SND_OFF},
	{"NOTE_STOP_REV", CF_NOTE_STOP_REV},
	{"DAC_PS4", CF_DAC_PS4},
	{"DAC_CYMN", CF_DAC_CYMN},
	{"DAC_GAXE3", CF_DAC_GAXE3},
	
	{"VOL_QUICK", CFSMS_VOL_QUICK},
	
	{NULL, CF_INVALID}};

static const OPT_LIST OPT_CFLAGS_SUB[] =
{	{"", 0x00},
	
	{"PAFMS_PAN", CFS_PAFMS_PAN},
	{"PAFMS_AMS", CFS_PAFMS_AMS},
	{"PAFMS_FMS", CFS_PAFMS_FMS},
	{"PAFMS_PAN_C", CFS_PAFMS_PAN_C},
	{"PAFMS_PAN_L", CFS_PAFMS_PAN_L},
	{"PAFMS_PAN_R", CFS_PAFMS_PAN_R},
	
	{"VOL_NN_FM", CFS_VOL_NN_FM},
	{"VOL_NN_PSG", CFS_VOL_NN_PSG},
	{"VOL_NN_FMP", CFS_VOL_NN_FMP},
	{"VOL_NN_FMP1", CFS_VOL_NN_FMP1},
	{"VOL_CN_FM", CFS_VOL_CN_FM},
	{"VOL_CN_PSG", CFS_VOL_CN_PSG},
	{"VOL_CN_FMP", CFS_VOL_CN_FMP},
	{"VOL_CC_FM", CFS_VOL_CC_FM},
	{"VOL_CC_PSG", CFS_VOL_CC_PSG},
	{"VOL_CC_FMP", CFS_VOL_CC_FMP},
	{"VOL_CC_FMP2", CFS_VOL_CC_FMP2},
	{"VOL_ABS", CFS_VOL_ABS},
	{"VOL_ABS_S3K", CFS_VOL_ABS_S3K},
	{"VOL_ABS_HF", CFS_VOL_ABS_HF},
	{"VOL_ABS_HF2", CFS_VOL_ABS_HF2},
	{"VOL_ABS_TMP", CFS_VOL_ABS_TMP},
	{"VOL_SPC_TMP", CFS_VOL_SPC_TMP},
	{"VOL_ABS_PDRM", CFS_VOL_ABS_PDRM},
	{"VOL_CHG_PDRM", CFS_VOL_CHG_PDRM},
	
	{"NSTOP_NORMAL", CFS_NSTOP_NORMAL},
	{"NSTOP_MULT", CFS_NSTOP_MULT},
	
	{"TRNSP_ADD", CFS_TRNSP_ADD},
	{"TRNSP_SET", CFS_TRNSP_SET},
	{"TRNSP_SET_S3K", CFS_TRNSP_SET_S3K},
	{"TRNSP_ADD_ALL", CFS_TRNSP_ADD_ALL},
	{"TRNSP_GADD", CFS_TRNSP_GADD},
	{"TRNSP_GSET", CFS_TRNSP_GSET},
	{"TRNSP_RAND", CFS_TRNSP_RAND},
	
	{"INS_N_FM", CFS_INS_N_FM},
	{"INS_N_PSG", CFS_INS_N_PSG},
	{"INS_C_FM", CFS_INS_C_FM},
	{"INS_C_PSG", CFS_INS_C_PSG},
	{"INS_C_FMP", CFS_INS_C_FMP},
	
	{"PNOIS_SET", CFS_PNOIS_SET},
	{"PNOIS_SET2", CFS_PNOIS_SET2},
	{"PNOIS_SRES", CFS_PNOIS_SRES},
	
	{"FMW_CHN", CFS_FMW_CHN},
	{"FMW_FM1", CFS_FMW_FM1},
	
	{"DACHN_1", 0},
	{"DACHN_2", 1},
	
	{"MODS_ON", CFS_MODS_ON},
	{"MODS_OFF", CFS_MODS_OFF},
	
	{"MENV_GEN", CFS_MENV_GEN},
	{"MENV_FMP", CFS_MENV_FMP},
	{"MENV_GEN2", CFS_MENV_GEN2},
	
	{"ADSR_SETUP", CFS_ADSR_SETUP},
	{"ADSR_MODE", CFS_ADSR_MODE},
	
	{"LFO_NORMAL", CFS_LFO_NORMAL},
	{"LFO_AMSEN", CFS_LFO_AMSEN},
	
	{"SEG_NORMAL", CFS_SEG_NORMAL},
	{"SEG_FULLATK", CFS_SEG_FULLATK},
	
	{"MUSP_Z80", CFS_MUSP_Z80},
	{"MUSP_68K", CFS_MUSP_68K},
	
	{"FDIN_START", CFS_FDIN_START},
	{"FDIN_CANCEL", CFS_FDIN_CANCEL},
	
	{"TEMPO_SET", CFS_TEMPO_SET},
	{"TEMPO_ADD", CFS_TEMPO_ADD},
	
	{"TMULT_CUR", CFS_TMULT_CUR},
	{"TMULT_ALL", CFS_TMULT_ALL},
	
	{"TIME_SET", CFS_TIME_SET},
	{"TIME_ADD", CFS_TIME_ADD},
	{"TIME_SPC", CFS_TIME_SPC},
	
	{"CJMP_NZ", CFS_CJMP_NZ},
	{"CJMP_Z", CFS_CJMP_Z},
	{"CJMP_EQ", CFS_CJMP_EQ},
	{"CJMP_NZ_RESET", CFS_CJMP_NZ_RESET},
	{"CJMP_Z_RESET", CFS_CJMP_Z_RESET},
	
	{"TEND_STD", CFS_TEND_STD},
	{"TEND_MUTE", CFS_TEND_MUTE},
	
	{"NSREV_CYMN", CFS_NSREV_CYMN},
	{"NSREV_RST", CFS_NSREV_RST},
	
	{"PS4_VOLCTRL", CFS_PS4_VOLCTRL},
	{"PS4_VOLUME", CFS_PS4_VOLUME},
	{"PS4_SET_SND", CFS_PS4_SET_SND},
	{"PS4_LOOP", CFS_PS4_LOOP},
	{"PS4_REVERSE", CFS_PS4_REVERSE},
	{"PS4_TRKMODE", CFS_PS4_TRKMODE},
	
	{"CYMN_CHG_CH1", CFS_CYMN_CHG_CH | 0},
	{"CYMN_CHG_CH2", CFS_CYMN_CHG_CH | 1},
	{"CYMN_SET_CH1", CFS_CYMN_SET_CH | 0},
	{"CYMN_SET_CH2", CFS_CYMN_SET_CH | 1},
	
	{"GA3_2NOTE_TEMP", CFS_GA3_2NOTE_TEMP},
	{"GA3_2NOTE_PERM", CFS_GA3_2NOTE_PERM},
	{"GA3_2NOTE_OFF", CFS_GA3_2NOTE_OFF},
	
	{NULL, 0xFF}};


static UINT8 GetOptionValue(const OPT_LIST* OptList, const char* ValueStr)
{
	const OPT_LIST* CurOpt;
	
	if (ValueStr == NULL)
		ValueStr = "";
	/*if (ValueStr == NULL || ValueStr[0] == '\0')
	{
		CurOpt = OptList;
		while(CurOpt->Text != NULL)
			CurOpt ++;
		return CurOpt->Value;	//0x00;
	}*/
	
	CurOpt = OptList;
	while(CurOpt->Text != NULL)
	{
		if (! _stricmp(CurOpt->Text, ValueStr))
			return CurOpt->Value;
		
		CurOpt ++;
	}
	printf("GetOptionValue: Unknown value: %s.\n", ValueStr);
	
	return CurOpt->Value;	//0x00;
}

void LoadDriverDefinition(const char* FileName, SMPS_CFG* SmpsCfg)
{
	FILE* hFile;
	char LineStr[0x100];
	char* LToken;
	char* RToken1;
	char* RToken2;
	UINT8 Group;
	UINT8 RetVal;
	UINT8 CstRegCnt;
	UINT8* CstRegList;
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
	{
		printf("Error opening %s\n", FileName);
		return;
	}
	
	SmpsCfg->FMChnCnt = sizeof(FMCHN_ORDER);
	memcpy(SmpsCfg->FMChnList, FMCHN_ORDER, SmpsCfg->FMChnCnt);
	SmpsCfg->PSGChnCnt = sizeof(PSGCHN_ORDER);
	memcpy(SmpsCfg->PSGChnList, PSGCHN_ORDER, SmpsCfg->PSGChnCnt);
	SmpsCfg->AddChnCnt = 0x00;
	
	Group = 0xFF;
	CstRegList = NULL;
	while(! feof(hFile))
	{
		RetVal = GetTextLine(0x100, LineStr, hFile);
		if (RetVal)
			break;
		RetVal = GetTokenPtrs(LineStr, &LToken, &RToken1);
		if (RetVal)
			continue;
		
		if (*LToken == '[')
		{
			// [Section]
			if (! _stricmp(RToken1, "Main"))
				Group = 0x00;
			else if (! _stricmp(RToken1, "EnvelopeCmds"))
				Group = 0x10;
			else if (! _stricmp(RToken1, "Settings"))
				Group = 0x20;
			else
				Group = 0xFF;
			continue;
		}
		
		RToken2 = TrimToken(RToken1);
		if (Group == 0x00)	// [Main] group
		{
			if (! _stricmp(LToken, "PtrFmt"))
				SmpsCfg->PtrFmt = GetOptionValue(OPT_PTRFMT, RToken1);
			else if (! _stricmp(LToken, "InsMode"))
				SmpsCfg->InsMode = GetOptionValue(OPT_INSMODE, RToken1);
			else if (! _stricmp(LToken, "InsRegs"))
			{
				RevertTokenTrim(RToken1, RToken2);
				if (CstRegList != NULL)
				{
					free(CstRegList);	CstRegList = NULL;
				}
				CstRegCnt = (UINT8)ReadHexData(RToken1, &CstRegList);
			}
			else if (! _stricmp(LToken, "FMChnOrder"))
			{
				UINT32 ChnCount;
				UINT8* ChnIDs;
				UINT8 CurChn;
				
				RevertTokenTrim(RToken1, RToken2);
				ChnCount = ReadHexData(RToken1, &ChnIDs);
				
				for (CurChn = 0; CurChn < ChnCount; CurChn ++)
				{
					// check for valid channel IDs
					if (ChnIDs[CurChn] & 0x80)
					{
						if (ChnIDs[CurChn] & ~0xF0)
							break;
						SmpsCfg->PSGChnCnt = 0x00;
						continue;
					}
					if (ChnIDs[CurChn] & ~0x97)
						break;
				}
				if (! ChnCount || ChnCount > 8 || CurChn < ChnCount)	// invalid channels used?
				{
					SmpsCfg->FMChnCnt = sizeof(FMCHN_ORDER);
					memcpy(SmpsCfg->FMChnList, FMCHN_ORDER, 7);
				}
				else
				{
					SmpsCfg->FMChnCnt = ChnCount;
					memcpy(SmpsCfg->FMChnList, ChnIDs, ChnCount);
				}
				
				free(ChnIDs);
			}
			else if (! _stricmp(LToken, "AddChnOrder"))
			{
				UINT32 ChnCount;
				UINT8* ChnIDs;
				
				RevertTokenTrim(RToken1, RToken2);
				ChnCount = ReadHexData(RToken1, &ChnIDs);
				
				if (ChnCount < 0x10)
				{
					SmpsCfg->AddChnCnt = ChnCount;
					memcpy(SmpsCfg->AddChnList, ChnIDs, ChnCount);
				}
				
				free(ChnIDs);
			}
			else if (! _stricmp(LToken, "TempoMode"))
				SmpsCfg->TempoMode = GetOptionValue(OPT_TEMPOMODE, RToken1);
			else if (! _stricmp(LToken, "FMBaseNote"))
				SmpsCfg->FMBaseNote = GetOptionValue(OPT_FMBASENOTE, RToken1);
			else if (! _stricmp(LToken, "FMBaseOctave"))
				SmpsCfg->FMBaseOct = strtoul(RToken1, NULL, 0) & 7;
			else if (! _stricmp(LToken, "PSGBaseNote"))
				SmpsCfg->PSGBaseNote = GetOptionValue(OPT_PSGBASENOTE, RToken1);
			else if (! _stricmp(LToken, "DelayFreq"))
				SmpsCfg->DelayFreq = GetOptionValue(OPT_DELAYFREQ, RToken1);
			else if (! _stricmp(LToken, "NoteOnPrevent"))
				SmpsCfg->NoteOnPrevent = GetOptionValue(OPT_NONPREVENT, RToken1);
			else if (! _stricmp(LToken, "DetuneOctWrap"))
				SmpsCfg->FMOctWrap = GetBoolValue(RToken1, "True", "False");
			else if (! _stricmp(LToken, "FM6DACOff"))
				SmpsCfg->FM6DACOff = GetBoolValue(RToken1, "True", "False");
			else if (! _stricmp(LToken, "ModAlgo"))
				SmpsCfg->ModAlgo = GetOptionValue(OPT_MODALGO, RToken1);
			else if (! _stricmp(LToken, "EnvMult"))
				SmpsCfg->EnvMult = GetOptionValue(OPT_ENVMULT, RToken1);
			else if (! _stricmp(LToken, "VolMode"))
				SmpsCfg->VolMode = GetOptionValue(OPT_VOLMODE, RToken1);
			else if (! _stricmp(LToken, "FMFreqs") || ! _stricmp(LToken, "PSGFreqs") || ! _stricmp(LToken, "FM3Freqs"))
			{
				UINT8* FreqCntPtr;
				UINT16** FreqDataPtr;
				UINT8 FreqMode;
				UINT8 NewFreqCnt;
				UINT16* NewFreqData;
				
				if (! _stricmp(LToken, "FMFreqs"))
				{
					FreqMode = 0x00;
					FreqCntPtr = &SmpsCfg->FMFreqCnt;
					FreqDataPtr = &SmpsCfg->FMFreqs;
				}
				else if (! _stricmp(LToken, "PSGFreqs"))
				{
					FreqMode = 0x01;
					FreqCntPtr = &SmpsCfg->PSGFreqCnt;
					FreqDataPtr = &SmpsCfg->PSGFreqs;
				}
				else if (! _stricmp(LToken, "FM3Freqs"))
				{
					FreqMode = 0x02;
					FreqCntPtr = &SmpsCfg->FM3FreqCnt;
					FreqDataPtr = &SmpsCfg->FM3Freqs;
				}
				else
				{
					FreqMode = 0xFF;
				}
				
				NewFreqCnt = 0x00;
				NewFreqData = NULL;
				if (FreqMode <= 0x01 && isalpha(*RToken1))
				{
					const UINT16* FreqPtr = NULL;
					
					if (FreqMode == 0x00)
					{
						RetVal = GetOptionValue(OPT_FREQVALS_FM, RToken1);
						if (RetVal)
						{
							RetVal --;
							NewFreqCnt = DEF_FREQFM_CNT[RetVal];
							FreqPtr = DEF_FREQFM_PTR[RetVal];
						}
					}
					else
					{
						RetVal = GetOptionValue(OPT_FREQVALS_PSG, RToken1);
						if (RetVal)
						{
							RetVal --;
							NewFreqCnt = DEF_FREQPSG_CNT[RetVal];
							FreqPtr = DEF_FREQPSG_PTR[RetVal];
						}
					}
					if (FreqPtr != NULL)
					{
						NewFreqData = (UINT16*)malloc(NewFreqCnt * sizeof(UINT16));
						memcpy(NewFreqData, FreqPtr, NewFreqCnt * sizeof(UINT16));
					}
				}
				if (NewFreqData == NULL && FreqMode != 0xFF)
				{
					RevertTokenTrim(RToken1, RToken2);
					NewFreqCnt = ReadMultilineArrayData(hFile, RToken1, &NewFreqData, 0x02);
				}
				
				if (*FreqDataPtr != NULL)
					free(*FreqDataPtr);
				*FreqCntPtr = NewFreqCnt;
				*FreqDataPtr = NewFreqData;
			}
			else if (! _stricmp(LToken, "DrumChMode"))
				SmpsCfg->DrumChnMode = GetOptionValue(OPT_DRMCHNMODE, RToken1);
			else if (! _stricmp(LToken, "DACChns"))
				SmpsCfg->DACDrv.Cfg.Channels = (UINT8)strtoul(RToken1, NULL, 0);
			else if (! _stricmp(LToken, "DACVolDiv"))
				SmpsCfg->DACDrv.Cfg.VolDiv = (UINT8)strtoul(RToken1, NULL, 0);
		}
		else if (Group == 0x10)	// [EnvelopeCmds] group
		{
			UINT8 EnvCmd;
			
			EnvCmd = (UINT8)strtoul(LToken, NULL, 0x10);
			if (EnvCmd & 0x80)
			{
				EnvCmd &= 0x7F;
				SmpsCfg->EnvCmds[EnvCmd] = GetOptionValue(OPT_ENVCMD, RToken1);
			}
		}
		else if (Group == 0x20)	// [Settings] group
		{
			if (! _stricmp(LToken, "DefTimingMode"))
				SmpsCfg->InitCfg.Timing_DefMode = (UINT8)strtoul(RToken1, NULL, 0x10);
			else if (! _stricmp(LToken, "LockTimingMode"))
				SmpsCfg->InitCfg.Timing_Lock = GetBoolValue(RToken1, "True", "False");
			else if (! _stricmp(LToken, "DefTimerA"))
				SmpsCfg->InitCfg.Timing_TimerA = (UINT16)ParseNumber(RToken1, NULL, NULL);
			else if (! _stricmp(LToken, "DefTimerB"))
				SmpsCfg->InitCfg.Timing_TimerB = (UINT8)ParseNumber(RToken1, NULL, NULL);
		}
	}
	
	fclose(hFile);
	
	LoadRegisterList(SmpsCfg, CstRegCnt, CstRegList);
	if (CstRegList != NULL)
		free(CstRegList);
	
	return;
}

static void LoadRegisterList(SMPS_CFG* SmpsCfg, UINT8 CstRegCnt, const UINT8* CstRegList)
{
	UINT8 RegCnt;
	UINT8 CurReg;
	const UINT8* RegPtr;
	UINT8* RegList;
	UINT8 RegTL_Idx;
	
	if (SmpsCfg->InsRegs != NULL)
		free(SmpsCfg->InsRegs);
	
	if (CstRegList == NULL)
		SmpsCfg->InsMode &= 0x0F;
	
	if (SmpsCfg->InsMode & INSMODE_CST)
	{
		RegCnt = CstRegCnt;
		RegPtr = CstRegList;
		RegTL_Idx = 0x00;
		for (CurReg = 0x00; CurReg < RegCnt; CurReg ++)
		{
			if ((RegPtr[CurReg] & 0xF0) == 0x40)
			{
				if (! RegTL_Idx)
				{
					RegTL_Idx = RegPtr[CurReg];
				}
				else
				{
					// 40 44 48 4C -> INSMODE_HW
					// 40 48 44 4C -> leave INSMODE_DEF
					if ((RegPtr[CurReg] ^ RegTL_Idx) & 0x04)
						SmpsCfg->InsMode |= INSMODE_HW;
					break;
				}
			}
		}
	}
	else
	{
		// no custom mode - load predefined register sets
		if ((SmpsCfg->InsMode & 0x0F) == INSMODE_DEF)
		{
			RegCnt = (UINT8)sizeof(INSOPS_DEFAULT);
			RegPtr = INSOPS_DEFAULT;
		}
		else if ((SmpsCfg->InsMode & 0x0F) == INSMODE_HW)
		{
			RegCnt = (UINT8)sizeof(INSOPS_HARDWARE);
			RegPtr = INSOPS_HARDWARE;
		}
		else
		{
			RegCnt = 0x00;
			RegPtr = NULL;
		}
	}
	
	RegTL_Idx = 0xFF;
	for (CurReg = 0x00; CurReg < RegCnt; CurReg ++)
	{
		if (RegPtr[CurReg] == 0x00 || (RegPtr[CurReg] & 0x03))
			break;
		if ((RegPtr[CurReg] & 0xF0) == 0x40 && RegTL_Idx == 0xFF)
			RegTL_Idx = CurReg;
	}
	RegCnt = CurReg;
	if (! RegCnt)
	{
		SmpsCfg->InsRegCnt = 0x00;
		SmpsCfg->InsRegs = NULL;
		SmpsCfg->InsReg_TL = NULL;
		return;
	}
	
	RegList = (UINT8*)malloc(RegCnt + 1);
	memcpy(RegList, RegPtr, RegCnt);
	RegList[RegCnt] = 0x00;
	
	SmpsCfg->InsRegCnt = RegCnt;
	SmpsCfg->InsRegs = RegList;
	if (RegTL_Idx == 0xFF)
		SmpsCfg->InsReg_TL = NULL;
	else
		SmpsCfg->InsReg_TL = &RegList[RegTL_Idx];
	
	return;
}

void FreeDriverDefinition(SMPS_CFG* SmpsCfg)
{
	/*if (SmpsCfg->FMChnList != NULL)
	{
		SmpsCfg->FMChnCnt = 0x00;
		free(SmpsCfg->FMChnList);	SmpsCfg->FMChnList = NULL;
	}*/
	if (SmpsCfg->InsRegs != NULL)
	{
		SmpsCfg->InsRegCnt = 0x00;
		free(SmpsCfg->InsRegs);	SmpsCfg->InsRegs = NULL;
	}
	free(SmpsCfg->FMFreqs);		SmpsCfg->FMFreqs = NULL;
	free(SmpsCfg->PSGFreqs);	SmpsCfg->PSGFreqs = NULL;
	free(SmpsCfg->FM3Freqs);	SmpsCfg->FM3Freqs = NULL;
	
	return;
}


static void ApplyCommandFlags(UINT8 FlagCnt, const UINT8* IDList, const CMD_FLAGS* CFBuffer, CMD_LIB* CFLib)
{
	UINT16 CurCF;
	UINT16 MaxCF;
	CMD_FLAGS* TempCF;
	
	MaxCF = 0xFFFF;
	for (CurCF = 0x00; CurCF < 0x100; CurCF ++)
	{
		if (IDList[CurCF] != 0xFF)
		{
			if (MaxCF == 0xFFFF)
			{
				CFLib->FlagBase = (UINT8)CurCF;
				MaxCF = CurCF;
			}
			else
			{
				if (CurCF < CFLib->FlagBase)
					CFLib->FlagBase = (UINT8)CurCF;
				if (CurCF > MaxCF)
					MaxCF = CurCF;
			}
		}
	}
	MaxCF ++;	// It is more useful to have it mark the CF after the last one.
	CFLib->FlagCount = MaxCF - CFLib->FlagBase;
	CFLib->CmdData = (CMD_FLAGS*)malloc(CFLib->FlagCount * sizeof(CMD_FLAGS));
	
	for (CurCF = CFLib->FlagBase; CurCF < MaxCF; CurCF ++)
	{
		TempCF = &CFLib->CmdData[CurCF - CFLib->FlagBase];
		if (IDList[CurCF] != 0xFF)
		{
			*TempCF = CFBuffer[IDList[CurCF]];
		}
		else
		{
			TempCF->Type = CF_INVALID;
			TempCF->Len = 0x01;
		}
	}
	
	return;
}

void LoadCommandDefinition(const char* FileName, SMPS_CFG* SmpsCfg)
{
	CMD_LIB* CurCLib;
	FILE* hFile;
	char LineStr[0x100];
	char* LToken;
	UINT8 Group;
	UINT8 RetVal;
	UINT8 CurCol;
	char* ColumnPtrs[5];
	UINT8 CF_ID;
	UINT8 CurCF;
	UINT8 CF_IDList[0x100];
	CMD_FLAGS CFBuffer[0x100];
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
	{
		printf("Error opening %s\n", FileName);
		return;
	}
	
	SmpsCfg->CmdList.FlagCount = 0x00;
	SmpsCfg->CmdList.CmdData = NULL;
	SmpsCfg->CmdMetaList.FlagCount = 0x00;
	SmpsCfg->CmdMetaList.CmdData = NULL;
	
	Group = 0xFF;
	CurCLib = NULL;
	CurCF = 0x00;
	while(! feof(hFile))
	{
		RetVal = GetTextLine(0x100, LineStr, hFile);
		if (RetVal)
			break;
		
		RetVal = GetSectionToken(LineStr, &ColumnPtrs[0], &LToken);
		if (! RetVal)
		{
			// [Section]
			if (CurCLib != NULL && CurCF)
				ApplyCommandFlags(CurCF, CF_IDList, CFBuffer, CurCLib);
			if (! _stricmp(LToken, "Main"))
			{
				Group = 0x00;
				CurCLib = &SmpsCfg->CmdList;
			}
			else if (! _stricmp(LToken, "Meta"))
			{
				Group = 0x01;
				CurCLib = &SmpsCfg->CmdMetaList;
			}
			else
			{
				Group = 0xFF;
				CurCLib = NULL;
			}
			CurCF = 0x00;
			memset(CF_IDList, 0xFF, 0x100);
			continue;
		}
		else if (RetVal != 0xFF || Group == 0xFF)
		{
			continue;
		}
		
		ColumnPtrs[0] = LineStr;
		for (CurCol = 1; CurCol < 5; CurCol ++)
		{
			RetVal = GetNextToken_Tab(&ColumnPtrs[CurCol - 1], &ColumnPtrs[CurCol]);
			if (RetVal || ColumnPtrs[CurCol] == NULL)
				break;
		}
		if (CurCol < 5)
		{
			for (; CurCol < 5; CurCol ++)
				ColumnPtrs[CurCol] = NULL;
		}
		else
		{
			TrimToken(ColumnPtrs[CurCol - 1]);
		}
		if (ColumnPtrs[3] == NULL)
			continue;	// need at least the Len column
		
		CF_ID = (UINT8)strtoul(ColumnPtrs[0], NULL, 0x10);
		CF_IDList[CF_ID] = CurCF;
		CFBuffer[CurCF].Type = GetOptionValue(OPT_CFLAGS, ColumnPtrs[1]);
		CFBuffer[CurCF].SubType = GetOptionValue(OPT_CFLAGS_SUB, ColumnPtrs[2]);
		CFBuffer[CurCF].Len = (UINT8)strtoul(ColumnPtrs[3], NULL, 0x10);
		if (ColumnPtrs[4] != NULL)
			CFBuffer[CurCF].JumpOfs = (UINT8)strtoul(ColumnPtrs[4], NULL, 0x10);
		else
			CFBuffer[CurCF].JumpOfs = 0x00;
		CurCF ++;
	}
	if (CurCLib != NULL && CurCF)
		ApplyCommandFlags(CurCF, CF_IDList, CFBuffer, CurCLib);
	
	fclose(hFile);
	
	printf("CFMain: %02hXh commands, CFMeta: %02hXh commands\n",
			SmpsCfg->CmdList.FlagCount, SmpsCfg->CmdMetaList.FlagCount);
	
	return;
}

void FreeCommandDefinition(SMPS_CFG* SmpsCfg)
{
	CMD_LIB* CFLib;
	
	CFLib = &SmpsCfg->CmdList;
	CFLib->FlagCount = 0x00;
	free(CFLib->CmdData);	CFLib->CmdData = NULL;
	
	CFLib = &SmpsCfg->CmdMetaList;
	CFLib->FlagCount = 0x00;
	free(CFLib->CmdData);	CFLib->CmdData = NULL;
	
	return;
}


void LoadDrumDefinition(const char* FileName, DRUM_LIB* DrumDef)
{
	FILE* hFile;
	char LineStr[0x100];
	char* LToken;
	char* RToken;
	UINT8 Group;
	UINT8 RetVal;
#define DDEF_COLUMNS	5
	char* ColumnPtrs[DDEF_COLUMNS];
	UINT16 DrumIDBase;
	DRUM_DATA* TempDrum;
	UINT8 DrumNote;
	UINT8 DrumNoteEnd;
	UINT8 Type;
	UINT8 ChnMask;
	UINT16 DrumID;
	UINT32 PitchOvr;
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
	{
		printf("Error opening %s\n", FileName);
		return;
	}
	
	DrumDef->DrumCount = 0x60;
	DrumDef->DrumData = (DRUM_DATA*)malloc(DrumDef->DrumCount * sizeof(DRUM_DATA));
	memset(DrumDef->DrumData, 0x00, DrumDef->DrumCount * sizeof(DRUM_DATA));
	for (DrumNote = 0x00; DrumNote < DrumDef->DrumCount; DrumNote ++)
		DrumDef->DrumData[DrumNote].Type = DRMTYPE_NONE;
	
	Group = 0xFF;
	DrumIDBase = 0x0000;
	while(! feof(hFile))
	{
		RetVal = GetTextLine(0x100, LineStr, hFile);
		if (RetVal)
			break;
		
		LToken = LineStr;
		RetVal = GetTokenPtrs(LineStr, &LToken, &RToken);
		if (*LToken == '[')
		{
			// [Section]
			if (! _stricmp(RToken, "Main"))
				Group = 0x00;
			else if (! _stricmp(RToken, "Drums"))
				Group = 0x01;
			else
				Group = 0xFF;
			continue;
		}
		else if (Group == 0xFF)
		{
			continue;
		}
		
		if (Group == 0x00)	// [Main] group
		{
			if (RetVal)
				continue;
			
			TrimToken(RToken);
			if (! _stricmp(LToken, "DrumMode"))
				DrumDef->Mode = GetOptionValue(OPT_DRUMMODE, RToken);
			else if (! _stricmp(LToken, "Mask1"))
				DrumDef->Mask1 = (UINT8)strtoul(RToken, NULL, 0x10);
			else if (! _stricmp(LToken, "Mask2"))
				DrumDef->Mask2 = (UINT8)strtoul(RToken, NULL, 0x10);
			else if (! _stricmp(LToken, "DrumIDBase"))
				DrumIDBase = (UINT16)strtol(RToken, NULL, 0x10);	// allow -1
		}
		else if (Group == 0x01)	// [Drums] group
		{
			GetColumns_Tab(LineStr, DDEF_COLUMNS, ColumnPtrs);
			if (ColumnPtrs[2] == NULL)
				continue;	// need at least the Len column
			
			DrumNote = (UINT8)strtoul(ColumnPtrs[0], &RToken, 0x10);
			if (DrumNote < 0x80)
				continue;
			
			DrumNoteEnd = DrumNote;
			if (RToken != NULL && ! strncmp(RToken, "..", 2))
			{
				// handle 81..83
				RToken += 2;
				DrumNoteEnd = (UINT8)strtoul(RToken, NULL, 0x10);
				if (DrumNoteEnd < DrumNote)
					DrumNoteEnd = DrumNote;
			}
			
			Type = GetOptionValue(OPT_DRUMTYPE, ColumnPtrs[1]);
			DrumID = (UINT16)strtoul(ColumnPtrs[2], NULL, 0x10);
			if (DrumID >= DrumIDBase - 1)	// Drum ID -1 is allowed
				DrumID -= DrumIDBase;
			if (ColumnPtrs[3] != NULL)
				PitchOvr = (UINT8)strtoul(ColumnPtrs[3], NULL, 0x10);
			else
				PitchOvr = 0x00;
			if (ColumnPtrs[4] != NULL)
				ChnMask = (UINT8)strtoul(ColumnPtrs[4], NULL, 0x10);
			else
				ChnMask = 0x00;
			
			DrumNote -= 0x80;
			DrumNoteEnd -= 0x80;
			for (; DrumNote <= DrumNoteEnd; DrumNote ++, DrumID ++)
			{
				if (DrumNote >= DrumDef->DrumCount)
					break;
				TempDrum = &DrumDef->DrumData[DrumNote];
				TempDrum->Type = Type;
				TempDrum->DrumID = DrumID;
				TempDrum->PitchOvr = PitchOvr;
				TempDrum->ChnMask = ChnMask;
			}
		}
	}
	
	DrumDef->Shift1 = GetShiftFromMask(DrumDef->Mask1);
	DrumDef->Shift2 = GetShiftFromMask(DrumDef->Mask2);
	
	fclose(hFile);
	
	return;
}

static UINT8 GetShiftFromMask(UINT8 Mask)
{
	UINT8 Shift;
	
	if (! Mask)
		return 0;
	
	Shift = 0;
	while(! (Mask & 0x01))
	{
		Shift ++;
		Mask >>= 1;
	}
	
	return Shift;
}

void FreeDrumDefinition(DRUM_LIB* DrumDef)
{
	DrumDef->DrumCount = 0x00;
	free(DrumDef->DrumData);	DrumDef->DrumData = NULL;
	
	return;
}

void LoadPSGDrumDefinition(const char* FileName, PSG_DRUM_LIB* DrumDef)
{
	FILE* hFile;
	char LineStr[0x100];
	char* LToken;
	char* RToken;
	UINT8 Group;
	UINT8 RetVal;
#define PSG_DDEF_COLUMNS	7
	char* ColumnPtrs[PSG_DDEF_COLUMNS];
	PSG_DRUM_DATA* TempDrum;
	UINT8 DrumNote;
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
	{
		printf("Error opening %s\n", FileName);
		return;
	}
	
	DrumDef->DrumCount = 0x60;
	DrumDef->DrumData = (PSG_DRUM_DATA*)malloc(DrumDef->DrumCount * sizeof(PSG_DRUM_DATA));
	memset(DrumDef->DrumData, 0x00, DrumDef->DrumCount * sizeof(PSG_DRUM_DATA));
	
	Group = 0x01;
	while(! feof(hFile))
	{
		RetVal = GetTextLine(0x100, LineStr, hFile);
		if (RetVal)
			break;
		
		// We inizialize LToken to ensure that we don't crash if the first line is a non-section line.
		// In that case GetTokenPtrs() doesn't set LToken (and returns an 'invalid line' code).
		LToken = LineStr;
		RetVal = GetTokenPtrs(LineStr, &LToken, &RToken);
		if (*LToken == '[')
		{
			// [Section]
			/*if (! _stricmp(RToken, "Main"))
				Group = 0x00;
			else*/ if (! _stricmp(RToken, "Drums"))
				Group = 0x01;
			else
				Group = 0xFF;
			continue;
		}
		else if (Group == 0xFF)
		{
			continue;
		}
		
		if (Group == 0x01)	// [Drums] group
		{
			GetColumns_Tab(LineStr, PSG_DDEF_COLUMNS, ColumnPtrs);
			if (ColumnPtrs[3] == NULL)
				continue;	// need at least the Noise Mode column
			
			DrumNote = (UINT8)strtoul(ColumnPtrs[0], &RToken, 0x10);
			if (DrumNote < 0x80 || DrumNote >= 0x80 + DrumDef->DrumCount)
				continue;
			
			TempDrum = &DrumDef->DrumData[DrumNote & 0x7F];
			// main values for PSG noise channel
			TempDrum->NoiseMode = (UINT8)strtoul(ColumnPtrs[1], NULL, 0x10);
			TempDrum->VolEnv = (UINT8)strtoul(ColumnPtrs[2], NULL, 0x10);
			TempDrum->Volume = (UINT8)strtoul(ColumnPtrs[3], NULL, 0x10);
			// defaults for PSG 3 channel (i.e. not used)
			TempDrum->Ch3Vol = 0xFF;
			TempDrum->Ch3Freq = 0xFFFF;
			TempDrum->Ch3Slide = 0x00;
			// if the Ch3Freq column is present, handle Ch3 columns
			if (ColumnPtrs[5] != NULL)
			{
				TempDrum->Ch3Vol = (UINT8)strtoul(ColumnPtrs[4], NULL, 0x10);
				TempDrum->Ch3Freq = (UINT16)strtoul(ColumnPtrs[5], NULL, 0x10);
				if (ColumnPtrs[6] != NULL)	// Ch3Slide is optional
					TempDrum->Ch3Slide = (UINT8)strtoul(ColumnPtrs[6], NULL, 0x10);
			}
		}
	}
	
	fclose(hFile);
	
	return;
}

void FreePSGDrumDefinition(PSG_DRUM_LIB* DrumDef)
{
	DrumDef->DrumCount = 0x00;
	free(DrumDef->DrumData);	DrumDef->DrumData = NULL;
	
	return;
}