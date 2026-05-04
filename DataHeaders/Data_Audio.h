#ifndef DATA_AUDIO_H_
#define DATA_AUDIO_H_

/*
 * Trimmed Uzebox patch set for the Wolfenduino Uzebox port.
 *
 * Source file kept alongside this header:
 *	DataHeaders/WolfUzeboxSfx_full.inc
 *
 * The uploaded source contained all 87 Wolf PC-speaker effects.  This build
 * keeps only the effects currently reachable by the C port so the large
 * wall-render LUT package can stay enabled.
 *
 * Runtime distance volume is supplied by Platform_playSoundAt() through
 * TriggerFx().  Therefore these patch streams avoid PC_ENV_VOL commands.
 * Sound-on sections use PC_WAVE,4.  Silent gaps use PC_WAVE,0.  Uzebox wavetable samples are signed,
 * so wave 0 in DataHeaders/sounds.inc must be 0x00, not 0x80.  This preserves the original
 * SFX rhythm without overwriting the runtime distance volume.
 *
 * Include after <uzebox.h>; requires PatchStruct, PC_* commands, PATCH_END,
 * PROGMEM, and NULL.
 *
 * Patch slots 0..3 are reserved for E1M1 music instruments because
 * music-compressed.inc references those patch IDs directly.  Wolf SFX patch
 * IDs begin at WOLF_PATCH_SFX_BASE.
 */

#define WOLF_SFX_ORIGINAL_COUNT 87
#define WOLF_SFX_OPENDOORSND 18	/* original Wolf3D SFX number: Door open */
#define WOLF_SFX_CLOSEDOORSND 19	/* original Wolf3D SFX number: Door close */
#define WOLF_SFX_PUSHWALLSND 46	/* original Wolf3D SFX number: Push wall */
#define WOLF_SFX_ATKKNIFESND 23	/* original Wolf3D SFX number: Knife attack */
#define WOLF_SFX_ATKPISTOLSND 24	/* original Wolf3D SFX number: Pistol attack */
#define WOLF_SFX_ATKMACHINEGUNSND 26	/* original Wolf3D SFX number: Machinegun attack */
#define WOLF_SFX_NAZIFIRESND 58	/* original Wolf3D SFX number: Guard attack */
#define WOLF_SFX_GETAMMOSND 31	/* original Wolf3D SFX number: Ammo pickup */
#define WOLF_SFX_GETMACHINESND 30	/* original Wolf3D SFX number: Machinegun pickup */
#define WOLF_SFX_HEALTH1SND 33	/* original Wolf3D SFX number: Health pickup */
#define WOLF_SFX_HITENEMYSND 27	/* original Wolf3D SFX number: Enemy hit */
#define WOLF_SFX_DEATHSCREAM1SND 29	/* original Wolf3D SFX number: Enemy death */
#define WOLF_SFX_TAKEDAMAGESND 16	/* original Wolf3D SFX number: Player damage */
#define WOLF_SFX_PLAYERDEATHSND 9	/* original Wolf3D SFX number: Player death */
#define WOLF_SFX_DOGDEATHSND 10	/* original Wolf3D SFX number: Dog death */
#define WOLF_SFX_DOGBARKSND 41	/* original Wolf3D SFX number: Dog bark */
#define WOLF_SFX_DOGATTACKSND 68	/* original Wolf3D SFX number: Dog attack */

enum
{
	WOLF_PATCH_E1M1_INSTRUMENT1 = 0,
	WOLF_PATCH_E1M1_INSTRUMENT2,
	WOLF_PATCH_E1M1_INSTRUMENT3,
	WOLF_PATCH_E1M1_PERCUSSION1,
	WOLF_PATCH_SFX_BASE,

	WOLF_PATCH_OPENDOORSND = WOLF_PATCH_SFX_BASE,	/* 4: original Wolf SFX 18, Door open */
	WOLF_PATCH_CLOSEDOORSND,	/* 5: original Wolf SFX 19, Door close */
	WOLF_PATCH_PUSHWALLSND,	/* 6: original Wolf SFX 46, Push wall */
	WOLF_PATCH_ATKKNIFESND,	/* 7: original Wolf SFX 23, Knife attack */
	WOLF_PATCH_ATKPISTOLSND,	/* 8: original Wolf SFX 24, Pistol attack */
	WOLF_PATCH_ATKMACHINEGUNSND,	/* 9: original Wolf SFX 26, Machinegun attack */
	WOLF_PATCH_NAZIFIRESND,	/* 10: original Wolf SFX 58, Guard attack */
	WOLF_PATCH_GETAMMOSND,	/* 11: original Wolf SFX 31, Ammo pickup */
	WOLF_PATCH_GETMACHINESND,	/* 12: original Wolf SFX 30, Machinegun pickup */
	WOLF_PATCH_HEALTH1SND,	/* 13: original Wolf SFX 33, Health pickup */
	WOLF_PATCH_HITENEMYSND,	/* 14: original Wolf SFX 27, Enemy hit */
	WOLF_PATCH_DEATHSCREAM1SND,	/* 15: original Wolf SFX 29, Enemy death */
	WOLF_PATCH_TAKEDAMAGESND,	/* 16: original Wolf SFX 16, Player damage */
	WOLF_PATCH_PLAYERDEATHSND,	/* 17: original Wolf SFX 9, Player death */
	WOLF_PATCH_DOGDEATHSND,	/* 18: original Wolf SFX 10, Dog death */
	WOLF_PATCH_DOGBARKSND,	/* 19: original Wolf SFX 41, Dog bark */
	WOLF_PATCH_DOGATTACKSND,	/* 20: original Wolf SFX 68, Dog attack */

	/*
	 * Channel 5 uses the PCM mixer path.  Enemy-origin sounds are routed
	 * through TriggerFx5(), so they need duplicate patch-table entries whose
	 * PatchStruct type is PCM and whose sample pointer references wave 4.
	 * Keep the normal entries above for player/item/door TriggerFx().
	 */
	WOLF_PATCH_PCM_NAZIFIRESND,
	WOLF_PATCH_PCM_HITENEMYSND,
	WOLF_PATCH_PCM_DEATHSCREAM1SND,
	WOLF_PATCH_PCM_DOGDEATHSND,
	WOLF_PATCH_PCM_DOGBARKSND,
	WOLF_PATCH_PCM_DOGATTACKSND,
	WOLF_PATCH_PCM_ATKMACHINEGUNSND,
	WOLF_PATCH_COUNT
};

/* audio18.raw: OPENDOORSND, payload=34 ticks @ 140 Hz, priority=20, frames=15 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_OPENDOORSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,52,	/* E-5, 164.8 Hz; source 167.9 Hz, var */
	3,PC_PITCH,53,	/* F-5, 174.6 Hz; source 173.9 Hz, var */
	4,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 186.7 Hz, var */
	2,PC_PITCH,55,	/* G-5, 196.0 Hz; source 198.9 Hz, raw 97..101 avg */
	1,PC_PITCH,56,	/* G#-5, 207.7 Hz; source 210.0 Hz, raw 92..97 avg */
	1,PC_PITCH,57,	/* A-5, 220.0 Hz; source 220.3 Hz, raw 89..92 avg */
	1,PC_PITCH,58,	/* A#-5, 233.1 Hz; source 234.8 Hz, raw 83..86 avg */
	1,PC_PITCH,59,	/* B-5, 246.9 Hz; source 246.5 Hz, raw 78..83 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 257.4 Hz, raw 77..78 avg */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio19.raw: CLOSEDOORSND, payload=35 ticks @ 140 Hz, priority=20, frames=15 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_CLOSEDOORSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,59,	/* B-5, 246.9 Hz; source 244.1 Hz, var */
	2,PC_PITCH,58,	/* A#-5, 233.1 Hz; source 233.4 Hz, var */
	2,PC_PITCH,57,	/* A-5, 220.0 Hz; source 223.1 Hz, var */
	2,PC_PITCH,56,	/* G#-5, 207.7 Hz; source 211.4 Hz, var */
	2,PC_PITCH,55,	/* G-5, 196.0 Hz; source 196.2 Hz, var */
	3,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 184.7 Hz, var */
	3,PC_PITCH,53,	/* F-5, 174.6 Hz; source 177.1 Hz, raw 111..113 avg */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio46.raw: PUSHWALLSND, payload=82 ticks @ 140 Hz, priority=50, frames=36 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_PUSHWALLSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	30,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,48,	/* C-5, 130.8 Hz; source 131.7 Hz, raw 151 */
	1,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,48,	/* C-5, 130.8 Hz; source 128.3 Hz, raw 155 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio23.raw: ATKKNIFESND, payload=40 ticks @ 140 Hz, priority=49, frames=18 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_ATKKNIFESND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 371.3 Hz, raw 17..84 avg */
	1,PC_PITCH,86,	/* D-8, 1174.7 Hz; source 1169.8 Hz, raw 17 */
	1,PC_PITCH,68,	/* G#-6, 415.3 Hz; source 409.8 Hz, raw 17..81 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 306.4 Hz, raw 63..71 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 256.8 Hz, raw 71..90 avg */
	1,PC_PITCH,59,	/* B-5, 246.9 Hz; source 243.2 Hz, raw 75..90 avg */
	1,PC_PITCH,57,	/* A-5, 220.0 Hz; source 225.9 Hz, raw 82..104 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 191.2 Hz, raw 104 */
	1,PC_PITCH,57,	/* A-5, 220.0 Hz; source 217.4 Hz, raw 80..104 avg */
	1,PC_PITCH,52,	/* E-5, 164.8 Hz; source 167.0 Hz, raw 104..123 avg */
	1,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 182.0 Hz, raw 104..121 avg */
	1,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 142.4 Hz, raw 121..149 avg */
	1,PC_PITCH,50,	/* D-5, 146.8 Hz; source 144.1 Hz, raw 128..153 avg */
	1,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 137.8 Hz, var */
	2,PC_PITCH,48,	/* C-5, 130.8 Hz; source 130.4 Hz, var */
	3,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio24.raw: ATKPISTOLSND, payload=65 ticks @ 140 Hz, priority=50, frames=28 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_ATKPISTOLSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,76,	/* E-7, 659.3 Hz; source 666.6 Hz, raw 24..37 avg */
	1,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 457.3 Hz, raw 33..155 avg */
	1,PC_PITCH,72,	/* C-7, 523.3 Hz; source 534.7 Hz, var */
	2,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 456.8 Hz, raw 33..157 avg */
	1,PC_PITCH,73,	/* C#-7, 554.4 Hz; source 548.3 Hz, raw 35..37 avg */
	1,PC_PITCH,52,	/* E-5, 164.8 Hz; source 164.7 Hz, raw 50..158 avg */
	1,PC_PITCH,64,	/* E-6, 329.6 Hz; source 323.2 Hz, raw 50..69 avg */
	1,PC_PITCH,59,	/* B-5, 246.9 Hz; source 250.4 Hz, raw 69..83 avg */
	1,PC_PITCH,50,	/* D-5, 146.8 Hz; source 149.0 Hz, var */
	2,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 137.9 Hz, raw 140..147 avg */
	1,PC_PITCH,48,	/* C-5, 130.8 Hz; source 128.6 Hz, var */
	2,PC_PITCH,47,	/* B-4, 123.5 Hz; source 125.0 Hz, raw 157..160 avg */
	1,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,48,	/* C-5, 130.8 Hz; source 132.8 Hz, raw 149..151 avg */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 135.7 Hz, raw 142..154 avg */
	1,PC_PITCH,48,	/* C-5, 130.8 Hz; source 129.1 Hz, raw 154 */
	1,PC_WAVE,0,	/* silence waveform */
	3,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,48,	/* C-5, 130.8 Hz; source 131.7 Hz, raw 151 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio26.raw: ATKMACHINEGUNSND, payload=12 ticks @ 140 Hz, priority=50, frames=6 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_ATKMACHINEGUNSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,56,	/* G#-5, 207.7 Hz; source 206.8 Hz, raw 91..104 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 196.7 Hz, raw 91..107 avg */
	1,PC_PITCH,56,	/* G#-5, 207.7 Hz; source 203.3 Hz, raw 93..104 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 196.9 Hz, raw 101 */
	1,PC_WAVE,0,	/* silence waveform */
	1,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,52,	/* E-5, 164.8 Hz; source 161.7 Hz, raw 123 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio58.raw: NAZIFIRESND, payload=60 ticks @ 140 Hz, priority=50, frames=26 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_NAZIFIRESND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,81,	/* A-7, 880.0 Hz; source 875.2 Hz, var */
	3,PC_PITCH,82,	/* A#-7, 932.3 Hz; source 928.5 Hz, var */
	2,PC_PITCH,83,	/* B-7, 987.8 Hz; source 987.6 Hz, raw 20..21 avg */
	1,PC_PITCH,79,	/* G-7, 784.0 Hz; source 775.6 Hz, raw 21..76 avg */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 188.2 Hz, raw 104..110 avg */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,55,	/* G-5, 196.0 Hz; source 195.0 Hz, raw 102 */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 157.8 Hz, raw 126 */
	1,PC_PITCH,53,	/* F-5, 174.6 Hz; source 175.7 Hz, raw 112..115 avg */
	1,PC_PITCH,52,	/* E-5, 164.8 Hz; source 169.6 Hz, raw 115..118 avg */
	1,PC_PITCH,50,	/* D-5, 146.8 Hz; source 149.4 Hz, raw 133..134 avg */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,50,	/* D-5, 146.8 Hz; source 145.7 Hz, var */
	3,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 141.0 Hz, var */
	2,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio31.raw: GETAMMOSND, payload=63 ticks @ 140 Hz, priority=80, frames=27 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_GETAMMOSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,74,	/* D-7, 587.3 Hz; source 584.9 Hz, raw 34 */
	7,PC_WAVE,0,	/* silence waveform */
	5,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,83,	/* B-7, 987.8 Hz; source 994.3 Hz, raw 20 */
	3,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,83,	/* B-7, 987.8 Hz; source 994.3 Hz, raw 20 */
	4,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,83,	/* B-7, 987.8 Hz; source 994.3 Hz, raw 20 */
	4,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio30.raw: GETMACHINESND, payload=92 ticks @ 140 Hz, priority=80, frames=40 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_GETMACHINESND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,65,	/* F-6, 349.2 Hz; source 347.2 Hz, raw 56..58 avg */
	1,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 367.9 Hz, var */
	2,PC_PITCH,67,	/* G-6, 392.0 Hz; source 382.4 Hz, raw 52 */
	3,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 376.4 Hz, raw 52..55 avg */
	1,PC_PITCH,65,	/* F-6, 349.2 Hz; source 349.1 Hz, raw 55..59 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 314.7 Hz, raw 59..66 avg */
	1,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 272.9 Hz, raw 70..79 avg */
	1,PC_PITCH,58,	/* A#-5, 233.1 Hz; source 239.2 Hz, raw 79..88 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 191.8 Hz, raw 88..111 avg */
	1,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 152.4 Hz, raw 128..133 avg */
	1,PC_WAVE,0,	/* silence waveform */
	3,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 373.6 Hz, var */
	6,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 368.3 Hz, raw 54 */
	5,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,77,	/* F-7, 698.5 Hz; source 708.7 Hz, var */
	7,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio33.raw: HEALTH1SND, payload=69 ticks @ 140 Hz, priority=85, frames=30 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_HEALTH1SND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,65,	/* F-6, 349.2 Hz; source 342.9 Hz, raw 58 */
	1,PC_WAVE,0,	/* silence waveform */
	3,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,67,	/* G-6, 392.0 Hz; source 389.9 Hz, raw 51 */
	2,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,72,	/* C-7, 523.3 Hz; source 523.3 Hz, raw 38 */
	1,PC_WAVE,0,	/* silence waveform */
	3,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,81,	/* A-7, 880.0 Hz; source 903.9 Hz, raw 22 */
	2,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,81,	/* A-7, 880.0 Hz; source 903.9 Hz, raw 22 */
	3,PC_WAVE,0,	/* silence waveform */
	4,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,82,	/* A#-7, 932.3 Hz; source 947.0 Hz, raw 21 */
	3,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio27.raw: HITENEMYSND, payload=30 ticks @ 140 Hz, priority=50, frames=13 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_HITENEMYSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,78,	/* F#-7, 740.0 Hz; source 734.0 Hz, raw 17..130 avg */
	1,PC_PITCH,81,	/* A-7, 880.0 Hz; source 893.7 Hz, raw 17..26 avg */
	1,PC_PITCH,75,	/* D#-7, 622.3 Hz; source 633.9 Hz, raw 26..33 avg */
	1,PC_PITCH,68,	/* G#-6, 415.3 Hz; source 424.6 Hz, raw 38..78 avg */
	1,PC_WAVE,0,	/* silence waveform */
	2,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,85,	/* C#-8, 1108.7 Hz; source 1104.8 Hz, raw 18 */
	1,PC_WAVE,0,	/* silence waveform */
	1,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,84,	/* C-8, 1046.5 Hz; source 1049.6 Hz, raw 18..20 avg */
	1,PC_WAVE,0,	/* silence waveform */
	4,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio29.raw: DEATHSCREAM1SND, payload=59 ticks @ 140 Hz, priority=50, frames=26 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_DEATHSCREAM1SND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,64,	/* E-6, 329.6 Hz; source 321.8 Hz, raw 60..64 avg */
	1,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 366.5 Hz, raw 48..61 avg */
	1,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 280.9 Hz, raw 48..156 avg */
	1,PC_PITCH,75,	/* D#-7, 622.3 Hz; source 612.3 Hz, raw 26..36 avg */
	1,PC_PITCH,77,	/* F-7, 698.5 Hz; source 679.9 Hz, raw 26..39 avg */
	1,PC_PITCH,76,	/* E-7, 659.3 Hz; source 658.3 Hz, raw 24..39 avg */
	1,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 462.8 Hz, raw 31..86 avg */
	1,PC_PITCH,80,	/* G#-7, 830.6 Hz; source 822.7 Hz, raw 15..32 avg */
	1,PC_PITCH,88,	/* E-8, 1318.5 Hz; source 1290.2 Hz, raw 15..16 avg */
	1,PC_PITCH,79,	/* G-7, 784.0 Hz; source 783.6 Hz, raw 16..70 avg */
	1,PC_PITCH,81,	/* A-7, 880.0 Hz; source 865.6 Hz, raw 22..24 avg */
	1,PC_PITCH,79,	/* G-7, 784.0 Hz; source 799.7 Hz, raw 22..29 avg */
	1,PC_PITCH,75,	/* D#-7, 622.3 Hz; source 636.7 Hz, raw 28..74 avg */
	1,PC_PITCH,67,	/* G-6, 392.0 Hz; source 402.4 Hz, raw 27..74 avg */
	1,PC_PITCH,77,	/* F-7, 698.5 Hz; source 693.7 Hz, raw 27..30 avg */
	1,PC_PITCH,76,	/* E-7, 659.3 Hz; source 657.0 Hz, raw 29..31 avg */
	1,PC_PITCH,77,	/* F-7, 698.5 Hz; source 710.7 Hz, raw 27..29 avg */
	1,PC_PITCH,73,	/* C#-7, 554.4 Hz; source 547.5 Hz, var */
	2,PC_PITCH,71,	/* B-6, 493.9 Hz; source 492.6 Hz, var */
	2,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 467.2 Hz, raw 29..95 avg */
	1,PC_PITCH,73,	/* C#-7, 554.4 Hz; source 557.2 Hz, var */
	2,PC_PITCH,71,	/* B-6, 493.9 Hz; source 499.8 Hz, raw 38..42 avg */
	1,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 473.5 Hz, raw 42 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio16.raw: TAKEDAMAGESND, payload=56 ticks @ 140 Hz, priority=90, frames=24 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_TAKEDAMAGESND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,64,	/* E-6, 329.6 Hz; source 330.8 Hz, var */
	2,PC_PITCH,65,	/* F-6, 349.2 Hz; source 353.5 Hz, raw 55..58 avg */
	1,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 373.3 Hz, raw 52..54 avg */
	1,PC_PITCH,67,	/* G-6, 392.0 Hz; source 393.9 Hz, var */
	2,PC_PITCH,64,	/* E-6, 329.6 Hz; source 326.0 Hz, raw 57..69 avg */
	1,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 272.4 Hz, var */
	2,PC_PITCH,59,	/* B-5, 246.9 Hz; source 253.9 Hz, raw 72..80 avg */
	1,PC_PITCH,62,	/* D-6, 293.7 Hz; source 288.1 Hz, var */
	2,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 282.7 Hz, raw 65..75 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 261.2 Hz, var */
	2,PC_PITCH,62,	/* D-6, 293.7 Hz; source 288.4 Hz, raw 66..71 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 310.9 Hz, raw 62..66 avg */
	1,PC_PITCH,64,	/* E-6, 329.6 Hz; source 334.8 Hz, raw 58..62 avg */
	1,PC_PITCH,65,	/* F-6, 349.2 Hz; source 344.0 Hz, raw 55..77 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 269.0 Hz, raw 71..77 avg */
	1,PC_PITCH,62,	/* D-6, 293.7 Hz; source 290.7 Hz, raw 67..71 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 313.0 Hz, raw 61..65 avg */
	1,PC_PITCH,64,	/* E-6, 329.6 Hz; source 335.6 Hz, var */
	2,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* audio09.raw: PLAYERDEATHSND, payload=46 ticks @ 140 Hz, priority=99, frames=20 @ 60 Hz, trailing=0x00 */
static const char wolf_sfx_PLAYERDEATHSND[] PROGMEM ={
	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,81,	/* A-7, 880.0 Hz; source 881.6 Hz, raw 21..27 avg */
	1,PC_PITCH,76,	/* E-7, 659.3 Hz; source 661.6 Hz, raw 27..34 avg */
	1,PC_PITCH,72,	/* C-7, 523.3 Hz; source 532.4 Hz, raw 34..39 avg */
	1,PC_PITCH,78,	/* F#-7, 740.0 Hz; source 734.8 Hz, raw 18..45 avg */
	1,PC_PITCH,75,	/* D#-7, 622.3 Hz; source 623.3 Hz, raw 18..47 avg */
	1,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 473.9 Hz, raw 18..56 avg */
	1,PC_PITCH,78,	/* F#-7, 740.0 Hz; source 744.6 Hz, raw 15..73 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 265.3 Hz, raw 73..77 avg */
	1,PC_PITCH,76,	/* E-7, 659.3 Hz; source 676.1 Hz, raw 16..80 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 193.0 Hz, raw 98..111 avg */
	1,PC_PITCH,74,	/* D-7, 587.3 Hz; source 595.6 Hz, raw 17..132 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 199.8 Hz, raw 92..132 avg */
	1,PC_WAVE,0,	/* silence waveform */
	7,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,59,	/* B-5, 246.9 Hz; source 251.7 Hz, raw 79 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* Added dog SFX: DOGDEATHSND; volume commands converted to PC_WAVE on/off. */
static const char wolf_sfx_DOGDEATHSND[] PROGMEM ={

	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,85,	/* C#-8, 1108.7 Hz; source 1085.2 Hz, raw 17..21 avg */
	1,PC_PITCH,86,	/* D-8, 1174.7 Hz; source 1182.0 Hz, raw 15..21 avg */
	1,PC_PITCH,88,	/* E-8, 1318.5 Hz; source 1325.8 Hz, raw 15 */
	1,PC_PITCH,85,	/* C#-8, 1108.7 Hz; source 1139.2 Hz, raw 16..27 avg */
	1,PC_PITCH,76,	/* E-7, 659.3 Hz; source 666.7 Hz, raw 27..33 avg */
	1,PC_PITCH,72,	/* C-7, 523.3 Hz; source 529.5 Hz, raw 33..40 avg */
	1,PC_PITCH,66,	/* F#-6, 370.0 Hz; source 377.9 Hz, raw 43..160 avg */
	1,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 188.5 Hz, raw 57..160 avg */
	1,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 157.0 Hz, raw 57..159 avg */
	1,PC_PITCH,55,	/* G-5, 196.0 Hz; source 197.2 Hz, var */
	2,PC_PITCH,56,	/* G#-5, 207.7 Hz; source 205.6 Hz, raw 93..98 avg */
	1,PC_PITCH,54,	/* F#-5, 185.0 Hz; source 188.0 Hz, raw 103..111 avg */
	1,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 160.0 Hz, raw 111..158 avg */
	1,PC_PITCH,49,	/* C#-5, 138.6 Hz; source 135.4 Hz, raw 135..158 avg */
	1,PC_PITCH,48,	/* C-5, 130.8 Hz; source 130.3 Hz, var */
	3,PC_PITCH,47,	/* B-4, 123.5 Hz; source 125.1 Hz, raw 159 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* Added dog SFX: DOGBARKSND; volume commands converted to PC_WAVE on/off. */
static const char wolf_sfx_DOGBARKSND[] PROGMEM ={

	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,62,	/* D-6, 293.7 Hz; source 295.9 Hz, raw 60..137 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 305.1 Hz, raw 51..137 avg */
	1,PC_PITCH,68,	/* G#-6, 415.3 Hz; source 404.2 Hz, raw 47..51 avg */
	1,PC_PITCH,63,	/* D#-6, 311.1 Hz; source 315.4 Hz, raw 41..140 avg */
	1,PC_PITCH,72,	/* C-7, 523.3 Hz; source 510.7 Hz, raw 37..41 avg */
	1,PC_PITCH,68,	/* G#-6, 415.3 Hz; source 411.7 Hz, raw 31..142 avg */
	1,PC_PITCH,72,	/* C-7, 523.3 Hz; source 532.7 Hz, raw 22..144 avg */
	1,PC_PITCH,67,	/* G-6, 392.0 Hz; source 385.3 Hz, raw 33..143 avg */
	1,PC_PITCH,73,	/* C#-7, 554.4 Hz; source 568.7 Hz, raw 30..41 avg */
	1,PC_PITCH,62,	/* D-6, 293.7 Hz; source 291.8 Hz, raw 48..143 avg */
	1,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 269.9 Hz, raw 52..144 avg */
	1,PC_PITCH,60,	/* C-6, 261.6 Hz; source 258.2 Hz, raw 56..147 avg */
	1,PC_PITCH,59,	/* B-5, 246.9 Hz; source 242.8 Hz, raw 60..146 avg */
	1,PC_PITCH,61,	/* C#-6, 277.2 Hz; source 276.8 Hz, raw 67..79 avg */
	1,PC_PITCH,58,	/* A#-5, 233.1 Hz; source 235.7 Hz, raw 79..89 avg */
	1,PC_PITCH,57,	/* A-5, 220.0 Hz; source 215.2 Hz, raw 92..93 avg */
	1,PC_PITCH,53,	/* F-5, 174.6 Hz; source 172.1 Hz, raw 92..153 avg */
	1,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 158.2 Hz, var */
	2,PC_WAVE,0,	/* silence waveform */
	1,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,48,	/* C-5, 130.8 Hz; source 131.7 Hz, raw 151 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

/* Added dog SFX: DOGATTACKSND; volume commands converted to PC_WAVE on/off. */
static const char wolf_sfx_DOGATTACKSND[] PROGMEM ={

	0,PC_WAVE,4,	/* waveform */
	0,PC_ENV_SPEED,0,	/* hold volume */
	0,PC_WAVE,4,	/* sound waveform */
	0,PC_PITCH,50,	/* D-5, 146.8 Hz; source 150.1 Hz, var */
	2,PC_PITCH,72,	/* C-7, 523.3 Hz; source 514.4 Hz, raw 20..131 avg */
	1,PC_PITCH,52,	/* E-5, 164.8 Hz; source 162.1 Hz, raw 118..125 avg */
	1,PC_PITCH,53,	/* F-5, 174.6 Hz; source 170.0 Hz, raw 106..137 avg */
	1,PC_PITCH,70,	/* A#-6, 466.2 Hz; source 455.3 Hz, raw 25..137 avg */
	1,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 154.9 Hz, raw 87..141 avg */
	1,PC_PITCH,53,	/* F-5, 174.6 Hz; source 170.4 Hz, raw 87..136 avg */
	1,PC_PITCH,50,	/* D-5, 146.8 Hz; source 148.1 Hz, var */
	2,PC_PITCH,51,	/* D#-5, 155.6 Hz; source 156.6 Hz, raw 127 */
	1,PC_NOTE_CUT,0,	/* end */
	0,PATCH_END,
};

#include "MusicPatches.inc"
#include "music-compressed.inc"

extern const char waves[];
#define WOLF_PATCH_PCM_WAVE4 (waves + (256 * 4))

static const struct PatchStruct patches[] PROGMEM = {
	{0,NULL,e1m1_instrument1,0,0},	/* 0: E1M1 instrument 1 */
	{0,NULL,e1m1_instrument2,0,0},	/* 1: E1M1 instrument 2 */
	{0,NULL,e1m1_instrument3,0,0},	/* 2: E1M1 instrument 3 */
	{0,NULL,e1m1_precussion1,0,0},	/* 3: E1M1 percussion */
	{0,NULL,wolf_sfx_OPENDOORSND,0,0},	/* 4: OPENDOORSND, original Wolf SFX 18 */
	{0,NULL,wolf_sfx_CLOSEDOORSND,0,0},	/* 5: CLOSEDOORSND, original Wolf SFX 19 */
	{0,NULL,wolf_sfx_PUSHWALLSND,0,0},	/* 6: PUSHWALLSND, original Wolf SFX 46 */
	{0,NULL,wolf_sfx_ATKKNIFESND,0,0},	/* 7: ATKKNIFESND, original Wolf SFX 23 */
	{0,NULL,wolf_sfx_ATKPISTOLSND,0,0},	/* 8: ATKPISTOLSND, original Wolf SFX 24 */
	{0,NULL,wolf_sfx_ATKMACHINEGUNSND,0,0},	/* 9: ATKMACHINEGUNSND, original Wolf SFX 26 */
	{0,NULL,wolf_sfx_NAZIFIRESND,0,0},	/* 10: NAZIFIRESND, original Wolf SFX 58 */
	{0,NULL,wolf_sfx_GETAMMOSND,0,0},	/* 11: GETAMMOSND, original Wolf SFX 31 */
	{0,NULL,wolf_sfx_GETMACHINESND,0,0},	/* 12: GETMACHINESND, original Wolf SFX 30 */
	{0,NULL,wolf_sfx_HEALTH1SND,0,0},	/* 13: HEALTH1SND, original Wolf SFX 33 */
	{0,NULL,wolf_sfx_HITENEMYSND,0,0},	/* 14: HITENEMYSND, original Wolf SFX 27 */
	{0,NULL,wolf_sfx_DEATHSCREAM1SND,0,0},	/* 15: DEATHSCREAM1SND, original Wolf SFX 29 */
	{0,NULL,wolf_sfx_TAKEDAMAGESND,0,0},	/* 16: TAKEDAMAGESND, original Wolf SFX 16 */
	{0,NULL,wolf_sfx_PLAYERDEATHSND,0,0},	/* 17: PLAYERDEATHSND, original Wolf SFX 9 */
	{0,NULL,wolf_sfx_DOGDEATHSND,0,0},	/* 18: DOGDEATHSND, original Wolf SFX 10 */
	{0,NULL,wolf_sfx_DOGBARKSND,0,0},	/* 19: DOGBARKSND, original Wolf SFX 41 */
	{0,NULL,wolf_sfx_DOGATTACKSND,0,0},	/* 20: DOGATTACKSND, original Wolf SFX 68 */

	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_NAZIFIRESND,0,256},	/* 21: PCM enemy guard/SS/boss alert/fire */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_HITENEMYSND,0,256},	/* 22: PCM enemy hit */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_DEATHSCREAM1SND,0,256},	/* 23: PCM enemy death */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_DOGDEATHSND,0,256},	/* 24: PCM dog death */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_DOGBARKSND,0,256},	/* 25: PCM dog bark */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_DOGATTACKSND,0,256},	/* 26: PCM dog attack */
	{2,WOLF_PATCH_PCM_WAVE4,wolf_sfx_ATKMACHINEGUNSND,0,256},	/* 27: PCM SS/boss attack */
};

#endif
