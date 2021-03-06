 /*
  * UAE - The Un*x Amiga Emulator
  *
  * ROM file management
  *
  */ 

#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "gui.h"
#include "memory.h"
#include "rommgr.h"
#include "zfile.h"
#include "crc32.h"
#include "fsdb.h"
#include "autoconf.h"
#include "filesys.h"

static struct romlist *rl;
static int romlist_cnt;

static struct romlist *getromlistbyids(const int *ids, const TCHAR *romname);

struct romlist *romlist_getit (void)
{
	return rl;
}

int romlist_count (void)
{
	return romlist_cnt;
}

static TCHAR *romlist_get (const struct romdata *rd)
{
  int i;

  if (!rd)
  	return 0;
  for (i = 0; i < romlist_cnt; i++) {
  	if (rl[i].rd->id == rd->id)
	    return rl[i].path;
  }
  return 0;
}

static struct romlist *romlist_getrl (const struct romdata *rd)
{
  int i;
    
  if (!rd)
  	return 0;
  for (i = 0; i < romlist_cnt; i++) {
  	if (rl[i].rd == rd)
	    return &rl[i];
  }
  return 0;
}

static void romlist_cleanup (void);
void romlist_add (const TCHAR *path, struct romdata *rd)
{
  struct romlist *rl2;

  if (path == NULL || rd == NULL) {
  	romlist_cleanup ();
  	return;
  }
  romlist_cnt++;
  rl = xrealloc (struct romlist, rl, romlist_cnt);
  rl2 = rl + romlist_cnt - 1;
  rl2->path = my_strdup (path);
  rl2->rd = rd;
	struct romdata *rd2 = getromdatabyid (rd->id);
	if (rd2 != rd && rd2) // replace "X" with parent name
		rd->name = rd2->name;
}

struct romdata *getromdatabypath (const TCHAR *path)
{
  int i;
  for (i = 0; i < romlist_cnt; i++) {
  	struct romdata *rd = rl[i].rd;
  	if (rd->configname && path[0] == ':') {
	    if (!_tcscmp(path + 1, rd->configname))
    		return rd;
  	}
		if (my_issamepath(rl[i].path, path))
	    return rl[i].rd;
  }
  return NULL;
}

#define NEXT_ROM_ID 260

#define ALTROM(id,grp,num,size,flags,crc32,a,b,c,d,e) \
{ _T("X"), 0, 0, 0, 0, 0, size, id, 0, 0, flags, (grp << 16) | num, 0, NULL, crc32, a, b, c, d, e },
#define ALTROMPN(id,grp,num,size,flags,pn,crc32,a,b,c,d,e) \
{ _T("X"), 0, 0, 0, 0, 0, size, id, 0, 0, flags, (grp << 16) | num, 0, pn, crc32, a, b, c, d, e },

static struct romdata roms[] = {
	{ _T(" AROS KS ROM (built-in)"), 0, 0, 0, 0, _T("AROS\0"), 524288 * 2, 66, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xffffffff, 0, 0, 0, 0, 0, _T("AROS") },
	{ _T(" ROM Disabled"), 0, 0, 0, 0, _T("NOROM\0"), 0, 87, 0, 0, ROMTYPE_NONE, 0, 0, NULL,
	0xffffffff, 0, 0, 0, 0, 0, _T("NOROM") },
	{ _T(" Enabled"), 0, 0, 0, 0, _T("ENABLED\0"), 0, 142, 0, 0, ROMTYPE_NOT, 0, 0, NULL,
	0xffffffff, 0, 0, 0, 0, 0, _T("ENABLED") },

	{ _T("Cloanto Amiga Forever ROM key"), 0, 0, 0, 0, 0, 2069, 0, 0, 1, ROMTYPE_KEY, 0, 0, NULL,
	0x869ae1b1, 0x801bbab3,0x2e3d3738,0x6dd1636d,0x4f1d6fa7,0xe21d5874 },
	{ _T("Cloanto Amiga Forever 2006 ROM key"), 0, 0, 0, 0, 0, 750, 48, 0, 1, ROMTYPE_KEY, 0, 0, NULL,
	0xb01c4b56, 0xbba8e5cd,0x118b8d92,0xafed5693,0x5eeb9770,0x2a662d8f },
	{ _T("Cloanto Amiga Forever 2010 ROM key"), 0, 0, 0, 0, 0, 1544, 73, 0, 1, ROMTYPE_KEY, 0, 0, NULL,
	0x8c4dd05c, 0x05034f62,0x0b5bb7b2,0x86954ea9,0x164fdb90,0xfb2897a4 },

	{ _T("KS ROM v1.2 (A500,A1000,A2000)"), 1, 2, 33, 180, _T("A500\0A1000\0A2000\0"), 262144, 5, 0, 0, ROMTYPE_KICK, 0, 0, _T("315093-01"),
	0xa6ce1636, 0x11F9E62C,0xF299F721,0x84835B7B,0x2A70A163,0x33FC0D88 },
	{ _T("KS ROM v1.3 (A500,A1000,A2000)"), 1, 3, 34, 5, _T("A500\0A1000\0A2000\0"), 262144, 6, 0, 0, ROMTYPE_KICK, 0, 0, _T("315093-02"),
	0xc4f0f55f, 0x891E9A54,0x7772FE0C,0x6C19B610,0xBAF8BC4E,0xA7FCB785 },
	{ _T("KS ROM v2.04 (A500+)"), 2, 4, 37, 175, _T("A500+\0"), 524288, 7, 0, 0, ROMTYPE_KICK, 0, 0, _T("390979-01"),
	0xc3bdb240, 0xC5839F5C,0xB98A7A89,0x47065C3E,0xD2F14F5F,0x42E334A1 },
	{ _T("KS ROM v2.05 (A600)"), 2, 5, 37, 299, _T("A600\0"), 524288, 8, 0, 0, ROMTYPE_KICK, 0, 0, _T("391388-01"),
	0x83028fb5, 0x87508DE8,0x34DC7EB4,0x7359CEDE,0x72D2E3C8,0xA2E5D8DB },
	{ _T("KS ROM v2.05 (A600HD)"), 2, 5, 37, 300, _T("A600HD\0A600\0"), 524288, 9, 0, 0, ROMTYPE_KICK, 0, 0, _T("391304-01"),
	0x64466c2a, 0xF72D8914,0x8DAC39C6,0x96E30B10,0x859EBC85,0x9226637B },
	{ _T("KS ROM v2.05 (A600HD)"), 2, 5, 37, 350, _T("A600HD\0A600\0"), 524288, 10, 0, 0, ROMTYPE_KICK, 0, 0, _T("391304-02"),
	0x43b0df7b, 0x02843C42,0x53BBD29A,0xBA535B0A,0xA3BD9A85,0x034ECDE4 },

	{ _T("KS ROM v3.0 (A1200)"), 3, 0, 39, 106, _T("A1200\0"), 524288, 11, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x6c9b07d2, 0x70033828,0x182FFFC7,0xED106E53,0x73A8B89D,0xDA76FAA5 },
	ALTROMPN(11, 1, 1, 262144, ROMTYPE_EVEN, _T("391523-01"), 0xc742a412,0x999eb81c,0x65dfd07a,0x71ee1931,0x5d99c7eb,0x858ab186)
	ALTROMPN(11, 1, 2, 262144, ROMTYPE_ODD , _T("391524-01"), 0xd55c6ec6,0x3341108d,0x3a402882,0xb5ef9d3b,0x242cbf3c,0x8ab1a3e9)
	{ _T("KS ROM v3.0 (A4000)"), 3, 0, 39, 106, _T("A4000\0"), 524288, 12, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x9e6ac152, 0xF0B4E9E2,0x9E12218C,0x2D5BD702,0x0E4E7852,0x97D91FD7 },
	ALTROMPN(12, 1, 1, 262144, ROMTYPE_EVEN, _T("391513-02"), 0x36f64dd0,0x196e9f3f,0x9cad934e,0x181c07da,0x33083b1f,0x0a3c702f)
	ALTROMPN(12, 1, 2, 262144, ROMTYPE_ODD , _T("391514-02"), 0x17266a55,0x42fbed34,0x53d1f11c,0xcbde89a9,0x826f2d11,0x75cca5cc)
	{ _T("KS ROM v3.1 (A4000)"), 3, 1, 40, 70, _T("A4000\0"), 524288, 13, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x2b4566f1, 0x81c631dd,0x096bbb31,0xd2af9029,0x9c76b774,0xdb74076c },
  ALTROM(13, 1, 1, 262144, ROMTYPE_EVEN, 0xf9cbecc9,0x138d8cb4,0x3b8312fe,0x16d69070,0xde607469,0xb3d4078e)
  ALTROM(13, 1, 2, 262144, ROMTYPE_ODD , 0xf8248355,0xc2379547,0x9fae3910,0xc185512c,0xa268b82f,0x1ae4fe05)
	{ _T("KS ROM v3.1 (A500,A600,A2000)"), 3, 1, 40, 63, _T("A500\0A600\0A2000\0"), 524288, 14, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xfc24ae0d, 0x3B7F1493,0xB27E2128,0x30F989F2,0x6CA76C02,0x049F09CA },
	{ _T("KS ROM v3.1 (A1200)"), 3, 1, 40, 68, _T("A1200\0"), 524288, 15, 1, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x1483a091, 0xE2154572,0x3FE8374E,0x91342617,0x604F1B3D,0x703094F1 },
	ALTROMPN(15, 1, 1, 262144, ROMTYPE_EVEN, _T("391773-01"), 0x08dbf275,0xb8800f5f,0x90929810,0x9ea69690,0xb1b8523f,0xa22ddb37)
	ALTROMPN(15, 1, 2, 262144, ROMTYPE_ODD , _T("391774-01"), 0x16c07bf8,0x90e331be,0x1970b0e5,0x3f53a9b0,0x390b51b5,0x9b3869c2)
	{ _T("KS ROM v3.1 (A4000)(Cloanto)"), 3, 1, 40, 68, _T("A4000\0"), 524288, 31, 2 | 4, 1, ROMTYPE_KICK, 0, 0, NULL,
	0x43b6dd22, 0xC3C48116,0x0866E60D,0x085E436A,0x24DB3617,0xFF60B5F9 },
	{ _T("KS ROM v3.1 (A4000)"), 3, 1, 40, 68, _T("A4000\0"), 524288, 16, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xd6bae334, 0x5FE04842,0xD04A4897,0x20F0F4BB,0x0E469481,0x99406F49 },
  ALTROM(16, 1, 1, 262144, ROMTYPE_EVEN, 0xb2af34f8,0x24e52b5e,0xfc020495,0x17387ab7,0xb1a1475f,0xc540350e)
  ALTROM(16, 1, 2, 262144, ROMTYPE_ODD , 0xe65636a3,0x313c7cbd,0xa5779e56,0xf19a41d3,0x4e760f51,0x7626d882)
	{ _T("KS ROM v3.X (A4000)(Cloanto)"), 3, 10, 45, 57, _T("A4000\0"), 524288, 46, 2 | 4, 1, ROMTYPE_KICK, 0, 0, NULL,
	0x3ac99edc, 0x3cbfc9e1,0xfe396360,0x157bd161,0xde74fc90,0x1abee7ec },

	{ _T("CD32 KS ROM v3.1"), 3, 1, 40, 60, _T("CD32\0"), 524288, 18, 1, 0, ROMTYPE_KICKCD32, 0, 0, NULL,
	0x1e62d4a5, 0x3525BE88,0x87F79B59,0x29E017B4,0x2380A79E,0xDFEE542D },
	{ _T("CD32 extended ROM"), 3, 1, 40, 60, _T("CD32\0"), 524288, 19, 1, 0, ROMTYPE_EXTCD32, 0, 0, NULL,
	0x87746be2, 0x5BEF3D62,0x8CE59CC0,0x2A66E6E4,0xAE0DA48F,0x60E78F7F },

  /* plain CD32 rom */
	{ _T("CD32 ROM (KS + extended)"), 3, 1, 40, 60, _T("CD32\0"), 2 * 524288, 64, 1, 0, ROMTYPE_KICKCD32 | ROMTYPE_EXTCD32 | ROMTYPE_CD32, 0, 0, NULL,
	0xf5d4f3c8, 0x9fa14825,0xc40a2475,0xa2eba5cf,0x325bd483,0xc447e7c1 },
  /* real CD32 rom dump 391640-03 */
	ALTROMPN(64, 1, 1, 2 * 524288, ROMTYPE_CD32, _T("391640-03"), 0xa4fbc94a, 0x816ce6c5,0x07787585,0x0c7d4345,0x2230a9ba,0x3a2902db )
   
	{ _T("CD32 Full Motion Video Cartridge ROM"), 3, 1, 40, 30, _T("CD32FMV\0"), 262144, 23, 1, 0, ROMTYPE_CD32CART, 0, 0, NULL,
	0xc35c37bf, 0x03ca81c7,0xa7b259cf,0x64bc9582,0x863eca0f,0x6529f435 },
	{ _T("CD32 Full Motion Video Cartridge ROM"), 3, 1, 40, 22, _T("CD32FMV\0"), 262144, 74, 1, 0, ROMTYPE_CD32CART, 0, 0, _T("391777-01"),
	0xf11158eb, 0x94e469a7,0x6030dcb2,0x99ebc752,0x0aaeef9d,0xb54284cf },

	{ _T("The Diagnostic 2.0 (Logica)"), 2, 0, 2, 0, _T("LOGICA\0"), 524288, 72, 0, 0, ROMTYPE_KICK | ROMTYPE_SPECIALKICK, 0, 0, NULL,
	0x8484f426, 0xba10d161,0x66b2e2d6,0x177c979c,0x99edf846,0x2b21651e },

	{ _T("Freezer: Action Replay Mk I v1.00"), 1, 0, 1, 0, _T("AR\0"), 65536, 52, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x2d921771, 0x1EAD9DDA,0x2DAD2914,0x6441F5EF,0x72183750,0x22E01248 },
  ALTROM(52, 1, 1, 32768, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x82d6eb87, 0x7c9bac11,0x28666017,0xeee6f019,0x63fb3890,0x7fbea355)
  ALTROM(52, 1, 2, 32768, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x40ae490c, 0x81d8e432,0x01b73fd9,0x2e204ebd,0x68af8602,0xb62ce397)
	{ _T("Freezer: Action Replay Mk I v1.50"), 1, 50, 1, 50, _T("AR\0"), 65536, 25, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0xf82c4258, 0x843B433B,0x2C56640E,0x045D5FDC,0x854DC6B1,0xA4964E7C },
  ALTROM(25, 1, 1, 32768, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x7fbd6de2, 0xb5f71a5c,0x09d65ecc,0xa8a3bc93,0x93558461,0xca190228)
  ALTROM(25, 1, 2, 32768, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x43018069, 0xad8ff242,0xb2cbf125,0x1fc53a73,0x581cf57a,0xb69cee00)
	{ _T("Freezer: Action Replay Mk II v2.05"), 2, 5, 2, 5, _T("AR\0"), 131072, 26, 0, 0, ROMTYPE_AR2, 0, 1, NULL,
	0x1287301f, 0xF6601DE8,0x888F0050,0x72BF562B,0x9F533BBC,0xAF1B0074 },
	{ _T("Freezer: Action Replay Mk II v2.12"), 2, 12, 2, 12, _T("AR\0"), 131072, 27, 0, 0, ROMTYPE_AR2, 0, 1, NULL,
	0x804d0361, 0x3194A07A,0x0A82D8B5,0xF2B6AEFA,0x3CA581D6,0x8BA8762B },
	{ _T("Freezer: Action Replay Mk II v2.14"), 2, 14, 2, 14, _T("AR\0"), 131072, 28, 0, 0, ROMTYPE_AR2, 0, 1, NULL,
	0x49650e4f, 0x255D6DF6,0x3A4EAB0A,0x838EB1A1,0x6A267B09,0x59DFF634 },
	{ _T("Freezer: Action Replay Mk III v3.09"), 3, 9, 3, 9, _T("AR\0"), 262144, 29, 0, 0, ROMTYPE_AR2, 0, 1, NULL,
	0x0ed9b5aa, 0x0FF3170A,0xBBF0CA64,0xC9DD93D6,0xEC0C7A01,0xB5436824 },
  ALTROM(29, 1, 1, 131072, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x2b84519f, 0x7841873b,0xf009d834,0x1dfa2794,0xb3751bac,0xf86adcc8)
  ALTROM(29, 1, 2, 131072, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x1d35bd56, 0x6464be16,0x26b51949,0x9e76e4e3,0x409e8016,0x515d48b6)
	{ _T("Freezer: Action Replay Mk III v3.17"), 3, 17, 3, 17, _T("AR\0"), 262144, 30, 0, 0, ROMTYPE_AR2, 0, 1, NULL,
	0xc8a16406, 0x5D4987C2,0xE3FFEA8B,0x1B02E314,0x30EF190F,0x2DB76542 },
	{ _T("Freezer: Action Replay 1200"), 0, 0, 0, 0, _T("AR\0"), 262144, 47, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x8d760101, 0x0F6AB834,0x2810094A,0xC0642F62,0xBA42F78B,0xC0B07E6A },

	{ _T("Freezer: Action Cartridge Super IV Professional"), 0, 0, 0, 0, _T("SUPERIV\0"), 0, 62, 0, 0, ROMTYPE_SUPERIV, 0, 1, NULL,
	0xffffffff, 0, 0, 0, 0, 0, _T("SuperIV") },
	{ _T("Freezer: Action Cart. Super IV Pro (+ROM v4.3)"), 4, 3, 4, 3, _T("SUPERIV\0"), 170368, 60, 0, 0, ROMTYPE_SUPERIV, 0, 1, NULL,
	0xe668a0be, 0x633A6E65,0xA93580B8,0xDDB0BE9C,0x9A64D4A1,0x7D4B4801 },
	{ _T("Freezer: X-Power Professional 500 v1.2"), 1, 2, 1, 2, _T("XPOWER\0"), 131072, 65, 0, 0, ROMTYPE_XPOWER, 0, 1, NULL,
	0x9e70c231, 0xa2977a1c,0x41a8ca7d,0x4af4a168,0x726da542,0x179d5963 },
  ALTROM(65, 1, 1, 65536, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xf98742e4,0xe8e683ba,0xd8b38d1f,0x79f3ad83,0xa9e67c6f,0xa91dc96c)
  ALTROM(65, 1, 2, 65536, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xdfb9984b,0x8d6bdd49,0x469ec8e2,0x0143fbb3,0x72e92500,0x99f07910)
	{ _T("Freezer: X-Power Professional 500 v1.3"), 1, 3, 1, 3, _T("XPOWER\0"), 131072, 68, 0, 0, ROMTYPE_XPOWER, 0, 1, NULL,
	0x31e057f0, 0x84650266,0x465d1859,0x7fd71dee,0x00775930,0xb7e450ee },
  ALTROM(68, 1, 1, 65536, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x0b2ce0c7,0x45ad5456,0x89192404,0x956f47ce,0xf66a5274,0x57ace33b)
  ALTROM(68, 1, 2, 65536, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x34580c35,0x8ad42566,0x7364f238,0x978f4381,0x08f8d5ec,0x470e72ea)
	{ _T("Freezer: Nordic Power v1.5"), 1, 5, 1, 5, _T("NPOWER\0"), 65536, 69, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0x83b4b21c, 0xc56ced25,0x506a5aab,0x3fa13813,0x4fc9e5ae,0x0f9d3709 },
  ALTROM(69, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xdd207174,0xae67652d,0x64f5db20,0x0f4b2110,0xee59567f,0xfbd90a1b)
  ALTROM(69, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x8f93d85d,0x73c62d21,0x40c0c092,0x6315b702,0xdd5d0f05,0x3dad7fab)
	{ _T("Freezer: Nordic Power v2.0"), 2, 0, 2, 0, _T("NPOWER\0"), 65536, 67, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0xa4db2906, 0x0aec68f7,0x25470c89,0x6b699ff4,0x6623dec5,0xc777466e },
  ALTROM(67, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xb21be46c,0x50dc607c,0xce976bbd,0x3841eaf0,0x591ddc7e,0xa1939ad2)
  ALTROM(67, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x96057aed,0xdd9209e2,0x1d5edfc1,0xcdb52abe,0x93de0f35,0xc43da696)
	{ _T("Freezer: Nordic Power v3.0"), 3, 0, 3, 0, _T("NPOWER\0"), 65536, 70, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0x72850aef, 0x59c91d1f,0xa8f118f9,0x0bdba05a,0x9ae788d7,0x7a6cc7c9 },
  ALTROM(70, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xf3330e1f,0x3a597db2,0xb7d11b6c,0xb8e13496,0xc215f223,0x88c4ca3c)
	ALTROM(70, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xee58e0f9,0x4148f4cb,0xb42cec33,0x8ca144de,0xd4f54118,0xe0f185dd)
	{ _T("Freezer: Nordic Power v3.2"), 3, 2, 3, 2, _T("NPOWER\0"), 65536, 115, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0x46158b6e, 0xd8c3f5af,0x5f109c61,0x5f6acb38,0x68fe6c06,0x580041b5 },
	ALTROM(115, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x4bfc71de,0x51914de0,0xdc0f055a,0x29ca188d,0xa7f61914,0xfdecbd07)
	ALTROM(115, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x923ec443,0x9f1e5334,0xaa620745,0xf4d0c50e,0x8736543b,0x6d4366c5)
	{ _T("Freezer: Pro Access v2.17"), 2, 17, 2, 17, _T("PROACCESS\0"), 65536, 116, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0xc4c265cd, 0x6a5c0d99,0x69a624dc,0x1b437aec,0x5dbcd4c7,0x2ce9064a },
	ALTROM(116, 1, 1, 32768, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x1909f7e9, 0x5abe9b9d,0xaae328c8,0x134e2b62,0x7b33b698,0xe342afc2)
	ALTROM(116, 1, 2, 32768, ROMTYPE_ODD  | ROMTYPE_8BIT, 0xa3927c72, 0x7adc9352,0x2d112ae9,0x23b9a70d,0x951b1e7a,0xba800ea6)

	{ _T("Freezer: HRTMon v2.37 (built-in)"), 0, 0, 0, 0, _T("HRTMON\0"), 0, 63, 0, 0, ROMTYPE_HRTMON, 0, 1, NULL,
	0xffffffff, 0, 0, 0, 0, 0, _T("HRTMon") },

  { NULL }

};

void romlist_clear (void)
{
  int i;
  int mask = 0;
  struct romdata *parent;
	const TCHAR *pn;

  xfree (rl);
  rl = 0;
  romlist_cnt = 0;
  parent = 0;
  pn = NULL;
  for (i = 0; roms[i].name; i++) {
  	struct romdata *rd = &roms[i];
  	if (rd->group == 0) {
	    parent = rd;
	    mask = rd->type;
	    pn = parent->partnumber;
  	} else {
	    rd->type &= ~ROMTYPE_MASK;
	    rd->type |= mask & ROMTYPE_MASK;
	    if (rd->partnumber && !pn) {
    		TCHAR *newpn;
    		if (parent->partnumber == NULL)
					parent->partnumber = my_strdup (_T(""));
    		newpn = xcalloc (TCHAR, _tcslen (parent->partnumber) + 1 + _tcslen (rd->partnumber) + 1);
    		if (_tcslen (parent->partnumber) > 0) {
  		    _tcscpy (newpn, parent->partnumber);
					_tcscat (newpn, _T("/"));
    		}
    		_tcscat (newpn, rd->partnumber);
				xfree ((char *) parent->partnumber);
    		parent->partnumber = newpn;
	    }
  	}
  }
}

/* remove rom entries that need 2 or more roms but not everything required is present */
static void romlist_cleanup (void)
{
  int i = 0;
  while (roms[i].name) {
  	struct romdata *rd = &roms[i];
  	int grp = rd->group >> 16;
    int ok = 1;
  	int j = i;
  	int k = i;
  	while (rd->name && (rd->group >> 16) == grp && grp > 0) {
	    struct romlist *rl = romlist_getrl (rd);
	    if (!rl)
    		ok = 0;
	    rd++;
	    j++;
  	}
  	if (ok == 0) {
	    while (i < j) {
    		struct romlist *rl2 = romlist_getrl (&roms[i]);
    		if (rl2) {
  		    int cnt = romlist_cnt - (rl2 - rl) - 1;
					write_log (_T("%s '%s' removed from romlist\n"), roms[k].name, rl2->path);
  		    xfree (rl2->path);
  		    if (cnt > 0)
      			memmove (rl2, rl2 + 1, cnt * sizeof (struct romlist));
  		    romlist_cnt--;
    		}
    		i++;
	    }
  	}
  	i++;
  }
}

struct romlist **getromlistbyident (int ver, int rev, int subver, int subrev, const TCHAR *model, int romflags, bool all)
{
	int i, j, ok, out, max;
	struct romdata *rd;
	struct romlist **rdout, *rltmp;
	void *buf;

	for (i = 0; roms[i].name; i++);
	if (all)
		max = i;
	else
		max = romlist_cnt;
	buf = xmalloc (uae_u8, (sizeof (struct romlist*) + sizeof (struct romlist)) * (i + 1));
	rdout = (struct romlist**)buf;
	rltmp = (struct romlist*)((uae_u8*)buf + (i + 1) * sizeof (struct romlist*));
	out = 0;
	for (i = 0; i < max; i++) {
		ok = 0;
		if (!all)
			rd = rl[i].rd;
		else
			rd = &roms[i];
		if (rd->group)
			continue;
		if (model && !_tcsicmp (model, rd->name))
			ok = 2;
		if ((ver < 0 || rd->ver == ver) && (rev < 0 || rd->rev == rev) && (rd->rev != 0 || rd->ver != 0)) {
			if (subver >= 0) {
				if (rd->subver == subver && (subrev < 0 || rd->subrev == subrev) && rd->subver > 0)
					ok = 1;
			} else {
				ok = 1;
			}
		}
		if (!ok)
			continue;
		if (model && ok < 2) {
			const TCHAR *p = rd->model;
			ok = 0;
			while (p && *p) {
				if (!_tcscmp(rd->model, model)) {
					ok = 1;
					break;
				}
				p = p + _tcslen(p) + 1;
			}
		}
		if (romflags && (rd->type & romflags) == 0)
			ok = 0;
		if (ok) {
			if (all) {
				rdout[out++] = rltmp;
				rltmp->path = NULL;
				rltmp->rd = rd;
				rltmp++;
			} else {
				rdout[out++] = &rl[i];
			}
		}
	}
	if (out == 0) {
		xfree (rdout);
		return NULL;
	}
	for (i = 0; i < out; i++) {
		int v1 = rdout[i]->rd->subver * 1000 + rdout[i]->rd->subrev;
		for (j = i + 1; j < out; j++) {
			int v2 = rdout[j]->rd->subver * 1000 + rdout[j]->rd->subrev;
			if (v1 < v2) {
				struct romlist *rltmp = rdout[j];
				rdout[j] = rdout[i];
				rdout[i] = rltmp;
			}
		}
	}
	rdout[out] = NULL;
	return rdout;
}

static int kickstart_checksum_do (uae_u8 *mem, int size)
{
  uae_u32 cksum = 0, prevck = 0;
  int i;
  for (i = 0; i < size; i+=4) {
  	uae_u32 data = mem[i]*65536*256 + mem[i+1]*65536 + mem[i+2]*256 + mem[i+3];
  	cksum += data;
  	if (cksum < prevck)
	    cksum++;
  	prevck = cksum;
  }
  return cksum == 0xffffffff;
}

static int kickstart_checksum_more_do (uae_u8 *mem, int size)
{
	uae_u8 *p = mem + size - 20;
	if (p[0] != ((size >> 24) & 0xff) || p[1] != ((size >> 16) & 0xff)
		|| p[2] != ((size >> 8) & 0xff) || p[3] != ((size >> 0) & 0xff))
		return 0;
	if (size == 524288) {
		if (mem[0] != 0x11 || mem[1] != 0x14)
			return 0;
	} else if (size == 262144) {
		if (mem[0] != 0x11 || mem[1] != 0x11)
			return 0;
	} else {
		return 0;
	}
	return kickstart_checksum_do(mem, size);
}

#define ROM_KEY_NUM 4
struct rom_key {
  uae_u8 *key;
  int size;
};

static struct rom_key keyring[ROM_KEY_NUM];

static void addkey (uae_u8 *key, int size, const TCHAR *name)
{
  int i;

	//write_log (_T("addkey(%08x,%d,'%s')\n"), key, size, name);
  if (key == NULL || size == 0) {
  	xfree (key);
  	return;
  }
  for (i = 0; i < ROM_KEY_NUM; i++) {
  	if (keyring[i].key && keyring[i].size == size && !memcmp (keyring[i].key, key, size)) {
	    xfree (key);
			//write_log (_T("key already in keyring\n"));
	    return;
  	}
  }
  for (i = 0; i < ROM_KEY_NUM; i++) {
  	if (keyring[i].key == NULL)
	    break;
  }
  if (i == ROM_KEY_NUM) {
  	xfree (key);
		//write_log (_T("keyring full\n"));
  	return;
  }
  keyring[i].key = key;
  keyring[i].size = size;
}

static void addkeyfile (const TCHAR *path)
{
  struct zfile *f;
  int keysize;
  uae_u8 *keybuf;

	f = zfile_fopen (path, _T("rb"), ZFD_NORMAL);
  if (!f)
  	return;
  zfile_fseek (f, 0, SEEK_END);
  keysize = zfile_ftell (f);
  if (keysize > 0) {
    zfile_fseek (f, 0, SEEK_SET);
    keybuf = xmalloc (uae_u8, keysize);
    zfile_fread (keybuf, 1, keysize, f);
    addkey (keybuf, keysize, path);
  }
  zfile_fclose (f);
}

void addkeydir (const TCHAR *path)
{
  TCHAR tmp[MAX_DPATH];

  _tcscpy (tmp, path);
  if (zfile_exists (tmp)) {
    int i;
    for (i = _tcslen (tmp) - 1; i > 0; i--) {
	    if (tmp[i] == '\\' || tmp[i] == '/')
        break;
  	}
  	tmp[i] = 0;
  }
	_tcscat (tmp, _T("/"));
	_tcscat (tmp, _T("rom.key"));
  addkeyfile (tmp);
}

static int get_keyring (void)
{
  int i, num = 0;
  for (i = 0; i < ROM_KEY_NUM; i++) {
  	if (keyring[i].key)
	    num++;
  }
  return num;
}

int load_keyring (struct uae_prefs *p, const TCHAR *path)
{
  uae_u8 *keybuf;
  int keysize;
  TCHAR tmp[MAX_DPATH], *d;
  int keyids[] = { 0, 48, 73, -1 };
  int cnt, i;

  free_keyring();
  keybuf = target_load_keyfile(p, path, &keysize, tmp);
  addkey (keybuf, keysize, tmp);
  for (i = 0; keyids[i] >= 0; i++) {
  	struct romdata *rd = getromdatabyid (keyids[i]);
  	TCHAR *s;
  	if (rd) {
	    s = romlist_get (rd);
	    if (s)
    		addkeyfile (s);
  	}
  }

  cnt = 0;
  for (;;) {
  	keybuf = NULL;
  	keysize = 0;
  	tmp[0] = 0;
  	switch (cnt)
  	{
		case 0:
			if (path)
      {
				_tcscpy (tmp, path);
  	    _tcscat (tmp, _T("rom.key"));
  	  }
			break;
		case 1:
	    if (p) {
    		_tcscpy (tmp, p->path_rom.path[0]);
    		_tcscat (tmp, _T("rom.key"));
	    }
    	break;
	  case 2:
	    _tcscpy (tmp, _T("roms/rom.key"));
    	break;
  	case 3:
	    _tcscpy (tmp, start_path_data);
	    _tcscat (tmp, _T("rom.key"));
    	break;
  	case 4:
	    _stprintf (tmp, _T("%s../shared/rom/rom.key"), start_path_data);
    	break;
  	case 5:
	    if (p) {
    		for (i = 0; uae_archive_extensions[i]; i++) {
  		    if (_tcsstr (p->romfile, uae_archive_extensions[i]))
      			break;
    		}
    		if (!uae_archive_extensions[i]) {
  		    _tcscpy (tmp, p->romfile);
  		    d = _tcsrchr (tmp, '/');
  		    if (!d)
      			d = _tcsrchr (tmp, '\\');
  		    if (d)
    			_tcscpy (d + 1, _T("rom.key"));
    		}
	    }
	    break;
	  case 6:
	    return get_keyring ();
  	}
  	cnt++;
  	if (!tmp[0])
      continue;
  	addkeyfile (tmp);
  }
}
void free_keyring (void)
{
  int i;
  for (i = 0; i < ROM_KEY_NUM; i++)
  	xfree (keyring[i].key);
  memset(keyring, 0, sizeof (struct rom_key) * ROM_KEY_NUM);
}

struct romdata *getromdatabyid (int id)
{
  int i = 0;
  while (roms[i].name) {
  	if (id == roms[i].id && roms[i].group == 0)
	    return &roms[i];
  	i++;
  }
  return 0;
}

STATIC_INLINE int notcrc32(uae_u32 crc32)
{
  if (crc32 == 0xffffffff || crc32 == 0x00000000)
  	return 1;
  return 0;
}

struct romdata *getromdatabycrc (uae_u32 crc32, bool allowgroup)
{
  int i = 0;
  while (roms[i].name) {
  	if (roms[i].group == 0 && crc32 == roms[i].crc32 && !notcrc32(crc32))
	    return &roms[i];
  	i++;
	}
	if (allowgroup) {
		i = 0;
		while (roms[i].name) {
			if (roms[i].group && crc32 == roms[i].crc32 && !notcrc32(crc32))
				return &roms[i];
			i++;
		}
  }
  return 0;
}

static int cmpsha1 (const uae_u8 *s1, const struct romdata *rd)
{
  int i;

  for (i = 0; i < SHA1_SIZE / 4; i++) {
  	uae_u32 v1 = (s1[0] << 24) | (s1[1] << 16) | (s1[2] << 8) | (s1[3] << 0);
  	uae_u32 v2 = rd->sha1[i];
  	if (v1 != v2)
	    return -1;
  	s1 += 4;
  }
  return 0;
}

static struct romdata *checkromdata (const uae_u8 *sha1, int size, uae_u32 mask)
{
  int i = 0;
  while (roms[i].name) {
  	if (!notcrc32(roms[i].crc32) && roms[i].size >= size) {
	    if (roms[i].type & mask) {
    		if (!cmpsha1(sha1, &roms[i]))
  		    return &roms[i];
	    }
  	}
  	i++;
  }
  return NULL;
}

int decode_cloanto_rom_do (uae_u8 *mem, int size, int real_size)
{
  int cnt, t, i;

  for (i = ROM_KEY_NUM - 1; i >= 0; i--) {
  	uae_u8 sha1[SHA1_SIZE];
  	struct romdata *rd;
  	int keysize = keyring[i].size;
  	uae_u8 *key = keyring[i].key;
  	if (!key)
	    continue;
    for (t = cnt = 0; cnt < size; cnt++, t = (t + 1) % keysize)  {
      mem[cnt] ^= key[t];
      if (real_size == cnt + 1)
  	    t = keysize - 1;
    }
  	if ((mem[2] == 0x4e && mem[3] == 0xf9) || (mem[0] == 0x11 && (mem[1] == 0x11 || mem[1] == 0x14))) {
	    cloanto_rom = 1;
	    return 1;
  	}
  	get_sha1 (mem, size, sha1);
  	rd = checkromdata (sha1, size, -1);
  	if (rd) {
	    if (rd->cloanto)
    		cloanto_rom = 1;
	    return 1;
  	}
  	if (i == 0)
	    break;
  	for (t = cnt = 0; cnt < size; cnt++, t = (t + 1) % keysize)  {
	    mem[cnt] ^= key[t];
	    if (real_size == cnt + 1)
    		t = keysize - 1;
  	}
  }
  return 0;
}

static int decode_rekick_rom_do (uae_u8 *mem, int size, int real_size)
{
  uae_u32 d1 = 0xdeadfeed, d0;
  int i;

  for (i = 0; i < size / 8; i++) {
  	d0 = ((mem[i * 8 + 0] << 24) | (mem[i * 8 + 1] << 16) | (mem[i * 8 + 2] << 8) | mem[i * 8 + 3]);
  	d1 = d1 ^ d0;
  	mem[i * 8 + 0] = d1 >> 24;
  	mem[i * 8 + 1] = d1 >> 16;
  	mem[i * 8 + 2] = d1 >> 8;
  	mem[i * 8 + 3] = d1;
  	d1 = ((mem[i * 8 + 4] << 24) | (mem[i * 8 + 5] << 16) | (mem[i * 8 + 6] << 8) | mem[i * 8 + 7]);
  	d0 = d0 ^ d1;
  	mem[i * 8 + 4] = d0 >> 24;
  	mem[i * 8 + 5] = d0 >> 16;
  	mem[i * 8 + 6] = d0 >> 8;
  	mem[i * 8 + 7] = d0;
  }
  return 1;
}

int decode_rom (uae_u8 *mem, int size, int mode, int real_size)
{
  if (mode == 1) {
	  if (!decode_cloanto_rom_do (mem, size, real_size)) {
    	notify_user (NUMSG_NOROMKEY);
    	return 0;
    }
    return 1;
  } else if (mode == 2) {
  	decode_rekick_rom_do (mem, size, real_size);
  	return 1;
  }
  return 0;
}

struct romdata *getromdatabydata (uae_u8 *rom, int size)
{
  uae_u8 sha1[SHA1_SIZE];
  uae_u8 tmp[4];
  uae_u8 *tmpbuf = NULL;
  struct romdata *ret = NULL;

  if (size > 11 && !memcmp (rom, "AMIROMTYPE1", 11)) {
  	uae_u8 *tmpbuf = xmalloc (uae_u8, size);
  	int tmpsize = size - 11;
  	memcpy (tmpbuf, rom + 11, tmpsize);
  	decode_rom (tmpbuf, tmpsize, 1, tmpsize);
  	rom = tmpbuf;
  	size = tmpsize;
  }
  get_sha1 (rom, size, sha1);
  ret = checkromdata(sha1, size, -1);
  if (!ret) {
  	get_sha1 (rom, size / 2, sha1);
  	ret = checkromdata (sha1, size / 2, -1);
  	if (!ret) {
			/* ignore AR2/3 IO-port range until we have full dump */
	    memcpy (tmp, rom, 4);
	    memset (rom, 0, 4);
	    get_sha1 (rom, size, sha1);
			ret = checkromdata (sha1, size, ROMTYPE_AR2);
	    memcpy (rom, tmp, 4);
  	}
	}//9 
  xfree (tmpbuf);
  return ret;
}

struct romdata *getromdatabyzfile (struct zfile *f)
{
  int pos, size;
  uae_u8 *p;
  struct romdata *rd;

  pos = zfile_ftell (f);
  zfile_fseek (f, 0, SEEK_END);
  size = zfile_ftell (f);
	if (size > 2048 * 1024)
		return NULL;
  p = xmalloc (uae_u8, size);
  if (!p)
  	return NULL;
  memset (p, 0, size);
  zfile_fseek (f, 0, SEEK_SET);
  zfile_fread (p, 1, size, f);
  zfile_fseek (f, pos, SEEK_SET);        
  rd = getromdatabydata (p, size);
  xfree (p);
  return rd;
}

void getromname	(const struct romdata *rd, TCHAR *name)
{
  name[0] = 0;
  if (!rd)
    return;
  while (rd->group)
  	rd--;
  _tcscat (name, rd->name);
  if ((rd->subrev || rd->subver) && rd->subver != rd->ver)
		_stprintf (name + _tcslen (name), _T(" rev %d.%d"), rd->subver, rd->subrev);
  if (rd->size > 0)
		_stprintf (name + _tcslen (name), _T(" (%dk)"), (rd->size + 1023) / 1024);
  if (rd->partnumber && _tcslen (rd->partnumber) > 0)
		_stprintf (name + _tcslen (name), _T(" [%s]"), rd->partnumber);
}

struct romlist *getromlistbyromdata (const struct romdata *rd)
{
  int ids[2];
  
  ids[0] = rd->id;
	ids[1] = -1;
	return getromlistbyids(ids, NULL);
}

static struct romlist *getromlistbyromtype(uae_u32 romtype, const TCHAR *romname)
{
	int i = 0;
	while (roms[i].name) {
		if (roms[i].type == romtype) {
			struct romdata *rd = &roms[i];
			for (int j = 0; j < romlist_cnt; j++) {
				if (rl[j].rd->id == rd->id) {
					if (romname) {
						if (my_issamepath(rl[j].path, romname))
					    return &rl[j];
					} else {
						return &rl[j];
					}
				}
			}
		}
		i++;
	}
	return NULL;
}

static struct romlist *getromlistbyids (const int *ids, const TCHAR *romname)
{
  struct romdata *rd;
  int i, j;

	i = 0;
	if (romname) {
		while (ids[i] >= 0) {
			rd = getromdatabyid (ids[i]);
			if (rd) {
				for (j = 0; j < romlist_cnt; j++) {
					if (rl[j].rd->id == rd->id) {
						if (my_issamepath(rl[j].path, romname))
							return &rl[j];
					}
				}
			}
			i++;
		}
	}
  i = 0;
  while (ids[i] >= 0) {
  	rd = getromdatabyid (ids[i]);
  	if (rd) {
	    for (j = 0; j < romlist_cnt; j++) {
    		if (rl[j].rd->id == rd->id)
  		    return &rl[j];
	    }
  	}
  	i++;
  }
  return NULL;
}

static void romwarning (const int *ids)
{
	int i, exp;
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	TCHAR tmp3[MAX_DPATH];

	if (ids[0] == -1)
		return;
	exp = 0;
	tmp2[0] = 0;
	i = 0;
	while (ids[i] >= 0) {
		struct romdata *rd = getromdatabyid (ids[i]);
		if (!(rd->type & ROMTYPE_NONE)) {
		  getromname (rd, tmp1);
		  _tcscat (tmp2, _T("- "));
		  _tcscat (tmp2, tmp1);
		  _tcscat (tmp2, _T("\n"));
			if (rd->type & ROMTYPE_CD32CART)
			  exp++;
		}
		i++;
  }
	translate_message (exp ? NUMSG_EXPROMNEED : NUMSG_ROMNEED, tmp3);
	gui_message (tmp3, tmp2);
}

static void byteswap (uae_u8 *buf, int size)
{
  int i;
  for (i = 0; i < size; i += 2) {
  	uae_u8 t = buf[i];
  	buf[i] = buf[i + 1];
  	buf[i + 1] = t;
  }
}
static void wordbyteswap (uae_u8 *buf, int size)
{
  int i;
  for (i = 0; i < size; i += 4) {
  	uae_u8 t;
  	t = buf[i + 0];
  	buf[i + 0] = buf[i + 2];
  	buf[i + 2] = t;
  	t = buf[i + 1];
  	buf[i + 1] = buf[i + 3];
  	buf[i + 3] = t;
  }
}

static void mergecd32 (uae_u8 *dst, uae_u8 *src, int size)
{
  int i, k;
  k = 0;
  for (i = 0; i < size / 2; i += 2) {
  	int j = i + size / 2;
  	dst[k + 1] = src[i + 0];
  	dst[k + 0] = src[i + 1];
  	dst[k + 3] = src[j + 0];
  	dst[k + 2] = src[j + 1];
  	k += 4;
  }
}

static void descramble (const struct romdata *rd, uae_u8 *data, int size, int odd)
{
	int flags = rd->type;

	if (flags & (ROMTYPE_NORDIC | ROMTYPE_XPOWER))
		descramble_nordicpro (data, size, odd);
}

static int read_rom_file (uae_u8 *buf, const struct romdata *rd)
{
  struct zfile *zf;
  struct romlist *rl = romlist_getrl (rd);
  uae_char tmp[11];

  if (!rl || _tcslen (rl->path) == 0)
  	return 0;
	zf = zfile_fopen (rl->path, _T("rb"), ZFD_NORMAL);
  if (!zf)
  	return 0;
  addkeydir (rl->path);
  zfile_fread (tmp, sizeof tmp, 1, zf);
  if (!memcmp (tmp, "AMIROMTYPE1", sizeof tmp)) {
  	zfile_fread (buf, rd->size, 1, zf);
    decode_cloanto_rom_do (buf, rd->size, rd->size);
  } else {
  	memcpy (buf, tmp, sizeof tmp);
  	zfile_fread (buf + sizeof tmp, rd->size - sizeof (tmp), 1, zf);
  }
  zfile_fclose (zf);
  return 1;
}

static struct zfile *read_rom (struct romdata *prd)
{
	struct romdata *rd2 = prd;
	struct romdata *rd = prd;
	struct romdata *rdpair = NULL;
	const TCHAR *name;
  int id = rd->id;
  uae_u32 crc32;
  int size;
  uae_u8 *buf, *buf2;

  /* find parent node */
  for (;;) {
  	if (rd2 == &roms[0])
	    break;
  	if (rd2[-1].id != id)
	    break;
  	rd2--;
  }
	
  size = rd2->size;
  crc32 = rd2->crc32;
  name = rd->name;
  buf = xmalloc (uae_u8, size * 2);
  memset (buf, 0xff, size * 2);
  if (!buf)
  	return NULL;
  buf2 = buf + size;
  while (rd->id == id) {
  	int i, j, add;
  	int ok = 0;
  	uae_u32 flags = rd->type;
    int odd = (flags & ROMTYPE_ODD) ? 1 : 0;

  	add = 0;
  	for (i = 0; i < 2; i++) {
	    memset (buf, 0, size);
	    if (!(flags & (ROMTYPE_EVEN | ROMTYPE_ODD))) {
    		read_rom_file (buf, rd);
    		if (flags & ROMTYPE_CD32) {
  		    memcpy (buf2, buf, size);
  		    mergecd32 (buf, buf2, size);
    		}
    		add = 1;
    		i++;
      } else {
    		int romsize = size / 2;
    		if (i)
		      odd = !odd;
				if (rd->id == rd[1].id)
					rdpair = &rd[1];
				else if (rd != roms)
					rdpair = &rd[-1];
				else
					rdpair = rd;
    		if (flags & ROMTYPE_8BIT) {
		      read_rom_file (buf2, rd);
		      if (flags & ROMTYPE_SCRAMBLED)
	          descramble (rd, buf2, romsize, odd);
		      for (j = 0; j < size; j += 2)
      			buf[j + odd] = buf2[j / 2];
					read_rom_file (buf2, rdpair);
		      if (flags & ROMTYPE_SCRAMBLED)
	          descramble (rd + 1, buf2, romsize, !odd);
		      for (j = 0; j < size; j += 2)
      			buf[j + (1 - odd)] = buf2[j / 2];
    		} else {
		      read_rom_file (buf2, rd);
		      if (flags & ROMTYPE_SCRAMBLED)
      			descramble (rd, buf2, romsize, odd);
		      for (j = 0; j < size; j += 4) {
      			buf[j + 2 * odd + 0] = buf2[j / 2 + 0];
      			buf[j + 2 * odd + 1] = buf2[j / 2 + 1];
		      }
					read_rom_file (buf2, rdpair);
		      if (flags & ROMTYPE_SCRAMBLED)
      			descramble (rd + 1, buf2, romsize, !odd);
		      for (j = 0; j < size; j += 4) {
      			buf[j + 2 * (1 - odd) + 0] = buf2[j / 2 + 0];
      			buf[j + 2 * (1 - odd) + 1] = buf2[j / 2 + 1];
		      }
    		}
        add = 2;
      }

			if (notcrc32(crc32) || get_crc32(buf, size) == crc32) {
    		ok = 1;
		  }
		  if (!ok && (rd->type & ROMTYPE_AR)) {
			  uae_u8 tmp[2];
			  tmp[0] = buf[0];
			  tmp[1] = buf[1];
			  buf[0] = buf[1] = 0;
			  if (get_crc32 (buf, size) == crc32)
				  ok = 1;
			  buf[0] = tmp[0];
			  buf[1] = tmp[1];
		  }
		  if (!ok) {
    		/* perhaps it is byteswapped without byteswap entry? */
    		byteswap (buf, size);
    		if (get_crc32 (buf, size) == crc32)
		      ok = 1;
				if (!ok)
					byteswap(buf, size);
      }
      if (ok) {
    		struct zfile *zf = zfile_fopen_empty (NULL, name, size);
    		if (zf) {
    	    zfile_fwrite (buf, size, 1, zf);
    	    zfile_fseek (zf, 0, SEEK_SET);
    		}
    		xfree (buf);
    		return zf;
      }
  	}
  	rd += add;

  }
  xfree (buf);
  return NULL;
}

struct zfile *rom_fopen (const TCHAR *name, const TCHAR *mode, int mask)
{
	return zfile_fopen (name, mode, mask);
}

static struct zfile *rom_fopen2(const TCHAR *name, const TCHAR *mode, int mask)
{
	struct zfile *f2 = NULL;
	struct zfile *f = rom_fopen(name, mode, mask);
	if (f) {
		int size = zfile_size(f);
		if (size == 524288 * 2 || size == 524288 || size == 262144) {
			uae_u8 *newrom = NULL;
			uae_u8 *tmp1 = xcalloc(uae_u8, 524288 * 2);
			uae_u8 *tmp2 = xcalloc(uae_u8, 524288 * 2);
			for (;;) {
				if (zfile_fread(tmp1, 1, size, f) != size)
					break;
				if (size == 524288 * 2) {
					// Perhaps it is 1M interleaved ROM image?
					mergecd32(tmp2, tmp1, 524288 * 2);
					if (kickstart_checksum_more_do(tmp2, 524288) && kickstart_checksum_more_do(tmp2 + 524288, 524288)) {
						newrom = tmp2;
						break;
					}
					// byteswapped KS ROM?
					byteswap(tmp1, 524288 * 2);
					if (kickstart_checksum_more_do(tmp1, 524288) && kickstart_checksum_more_do(tmp1 + 524288, 524288)) {
						newrom = tmp1;
						break;
					} else {
						byteswap(tmp1, 524288 * 2);
						wordbyteswap(tmp1, 524288 * 2);
						if (kickstart_checksum_more_do(tmp1, 524288) && kickstart_checksum_more_do(tmp1 + 524288, 524288)) {
							newrom = tmp1;
							break;
						}
					}
				} else {
					// byteswapped KS ROM?
					byteswap(tmp1, size);
					if (kickstart_checksum_more_do(tmp1, size)) {
						newrom = tmp1;
						break;
					} else {
						byteswap(tmp1, size);
						wordbyteswap(tmp1, size);
						if (kickstart_checksum_more_do(tmp1, size)) {
							newrom = tmp1;
							break;
						}
					}
				}
				break;
			}
			if (newrom) {
				f2 = zfile_fopen_data(zfile_getname(f), size, newrom);
			}
			xfree(tmp2);
			xfree(tmp1);
		}
	}
	if (f2) {
		zfile_fclose(f);
		f = f2;
	}
	if (f) {
		zfile_fseek(f, 0, SEEK_SET);
	}
	return f;
}

struct zfile *read_rom_name (const TCHAR *filename)
{
  struct zfile *f;

  for (int i = 0; i < romlist_cnt; i++) {
		if (my_issamepath(filename, rl[i].path)) {
	    struct romdata *rd = rl[i].rd;
			f = read_rom (rd);
	    if (f)
    		return f;
  	}
  }
	f = rom_fopen2(filename, _T("rb"), ZFD_NORMAL);
  if (f) {
		uae_u8 tmp[11] = { 0 };
  	zfile_fread (tmp, sizeof tmp, 1, f);
  	if (!memcmp (tmp, "AMIROMTYPE1", sizeof tmp)) {
	    struct zfile *df;
	    int size;
	    uae_u8 *buf;
	    addkeydir (filename);
	    zfile_fseek (f, 0, SEEK_END);
	    size = zfile_ftell (f) - sizeof tmp;
	    zfile_fseek (f, sizeof tmp, SEEK_SET);
	    buf = xmalloc (uae_u8, size);
	    zfile_fread (buf, size, 1, f);
			df = zfile_fopen_empty (f, _T("tmp.rom"), size);
	    decode_cloanto_rom_do (buf, size, size);
	    zfile_fwrite (buf, size, 1, df);
	    zfile_fclose (f);
	    xfree (buf);
	    zfile_fseek (df, 0, SEEK_SET);
	    f = df;
	  } else {
	      zfile_fseek (f, -((int)sizeof tmp), SEEK_CUR);
	  }
  }
  return f;
}

struct zfile *read_rom_name_guess (const TCHAR *filename, TCHAR *out)
{
	int i, j;
	struct zfile *f;
	const TCHAR *name;

	for (i = _tcslen (filename) - 1; i >= 0; i--) {
		if (filename[i] == '/' || filename[i] == '\\')
			break;
	}
	if (i < 0)
		return NULL;
	name = &filename[i];

	for (i = 0; i < romlist_cnt; i++) {
		TCHAR *n = rl[i].path;
		for (j = _tcslen (n) - 1; j >= 0; j--) {
			if (n[j] == '/' || n[j] == '\\')
				break;
		}
		if (j < 0)
			continue;
		if (!_tcsicmp (name, n + j)) {
			struct romdata *rd = rl[i].rd;
			f = read_rom (rd);
			if (f) {
				write_log (_T("ROM %s not found, using %s\n"), filename, rl[i].path);
				_tcscpy(out, rl[i].path);
				return f;
			}
		}
	}
	return NULL;
}

void kickstart_fix_checksum (uae_u8 *mem, int size)
{
  uae_u32 cksum = 0, prevck = 0;
  int i, ch = size == 524288 ? 0x7ffe8 : (size == 262144 ? 0x3ffe8 : 0x3e);

  mem[ch] = 0;
  mem[ch + 1] = 0;
  mem[ch + 2] = 0;
  mem[ch + 3] = 0;
  for (i = 0; i < size; i+=4) {
  	uae_u32 data = (mem[i] << 24) | (mem[i + 1] << 16) | (mem[i + 2] << 8) | mem[i + 3];
  	cksum += data;
  	if (cksum < prevck)
      cksum++;
  	prevck = cksum;
  }
  cksum ^= 0xffffffff;
  mem[ch++] = cksum >> 24;
  mem[ch++] = cksum >> 16;
  mem[ch++] = cksum >> 8;
  mem[ch++] = cksum >> 0;
}

int kickstart_checksum (uae_u8 *mem, int size)
{
  if (!kickstart_checksum_do (mem, size)) {
    notify_user (NUMSG_KSROMCRCERROR);
    return 0;
  }
  return 1;
}

int configure_rom (struct uae_prefs *p, const int *rom, int msg)
{
	struct romdata *rd;
	TCHAR *path = 0;
	int i;

	if (rom[0] < 0)
		return 1;
	i = 0;
	while (rom[i] >= 0) {
		rd = getromdatabyid (rom[i]);
		if (!rd) {
			i++;
			continue;
		}
		path = romlist_get (rd);
		if (path)
			break;
		i++;
	}
	if (!path) {
		if (msg)
			romwarning(rom);
		return 0;
	}
	if (rd->type & (ROMTYPE_KICK | ROMTYPE_KICKCD32))
		_tcscpy (p->romfile, path);
	if ((rd->type & ROMTYPE_EXTCD32) && !(rd->type & ROMTYPE_KICKCD32))
		_tcscpy (p->romextfile, path);
	if (rd->type & ROMTYPE_CD32CART) {
		_tcscpy(p->cartfile, path);
		struct boardromconfig *brc = get_device_rom_new(p, ROMTYPE_CD32CART, 0, NULL);
		if (brc)
			_tcscpy(brc->roms[0].romfile, p->cartfile);
	}
	if (rd->type == ROMTYPE_HRTMON || rd->type == ROMTYPE_XPOWER || rd->type ==  ROMTYPE_NORDIC || rd->type == ROMTYPE_AR || rd->type == ROMTYPE_SUPERIV)
		_tcscpy (p->cartfile, path);
	return 1;
}

const struct expansionromtype *get_device_expansion_rom(int romtype)
{
	for (int i = 0; expansionroms[i].name; i++) {
		const struct expansionromtype *ert = &expansionroms[i];
		if ((ert->romtype & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK))
			return ert;
	}
	return NULL;
}

static void device_rom_defaults(struct uae_prefs *p, struct boardromconfig *brc, int romtype, int devnum)
{
	memset(brc, 0, sizeof(boardromconfig));
	brc->device_type = romtype;
	brc->device_num = devnum;
	for (int i = 0; i < MAX_BOARD_ROMS; i++) {
		brc->roms[i].back = brc;
	}
	int order = 0;
	for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
		if (p->expansionboard[i].device_order > order)
			order = p->expansionboard[i].device_order;
	}
	if (p->fastmem[0].device_order > order)
		order = p->fastmem[0].device_order;
	if (p->z3fastmem[0].device_order > order)
		order = p->z3fastmem[0].device_order;
	if (p->rtgboards[0].device_order > order)
		order = p->rtgboards[0].device_order;
	brc->device_order = order + 1;
}

struct boardromconfig *get_device_rom_new(struct uae_prefs *p, int romtype, int devnum, int *index)
{
	int idx2;
	static struct boardromconfig fake;
	const struct expansionromtype *ert = get_device_expansion_rom(romtype);
	if (!ert) {
		if (index)
			*index = 0;
		return &fake;
	}
	if (index)
		*index = 0;
	struct boardromconfig *brc = get_device_rom(p, romtype, devnum, &idx2);
	if (!brc) {
		for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
			brc = &p->expansionboard[i];
			if (brc->device_type == 0)
				continue;
			int ok = 0;
			for (int j = 0; j < MAX_BOARD_ROMS; j++) {
				if (!brc->roms[j].romfile[0] && !brc->roms[j].romident[0] && !brc->roms[j].board_ram_size)
					ok++;
			}
			if (ok == MAX_BOARD_ROMS)
				memset(brc, 0, sizeof(boardromconfig));
		}
		for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
			brc = &p->expansionboard[i];
			if (brc->device_type == 0) {
				device_rom_defaults(p, brc, romtype, devnum);
				return brc;
			}
		}
		return &fake;
	}
	return brc;
}

void clear_device_rom(struct uae_prefs *p, int romtype, int devnum, bool deleteDevice)
{
	int index;
	struct boardromconfig *brc = get_device_rom(p, romtype, devnum, &index);
	if (!brc)
		return;
	if (deleteDevice) {
		memset(brc, 0, sizeof(struct boardromconfig));
	} else {
		memset(&brc->roms[index], 0, sizeof(struct romconfig));
	}
}

struct boardromconfig *get_device_rom(struct uae_prefs *p, int romtype, int devnum, int *index)
{
	const struct expansionromtype *ert = get_device_expansion_rom(romtype);
	if (!ert) {
		if (index)
		  *index = 0;
		return NULL;
	}
	int parentrom = romtype;
	if (index)
		*index = 0;
	for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
		struct boardromconfig *brc = &p->expansionboard[i];
		if (!brc->device_type)
			continue;
		if ((brc->device_type & ROMTYPE_MASK) == (parentrom & ROMTYPE_MASK) && brc->device_num == devnum)
			return brc;
	}
	return NULL;
}

struct romconfig *get_device_romconfig(struct uae_prefs *p, int romtype, int devnum)
{
	int idx;
	struct boardromconfig *brc = get_device_rom(p, romtype, devnum, &idx);
	if (brc)
		return &brc->roms[idx];
	return NULL;
}

void board_prefs_changed(int romtype, int devnum)
{
	if (romtype != -1) {
		int idx1, idx2;
		struct boardromconfig *brc1 = get_device_rom(&currprefs, romtype, devnum, &idx1);
		struct boardromconfig *brc2 = get_device_rom(&changed_prefs, romtype, devnum, &idx2);
		if (brc1 && brc2) {
			memcpy(brc1, brc2, sizeof(struct boardromconfig));
		} else if (brc1 && !brc2) {
			clear_device_rom(&currprefs, romtype, devnum, true);
		} else if (!brc1 && brc2) {
			brc1 = get_device_rom_new(&currprefs, romtype, devnum, &idx1);
			if (brc1)
				memcpy(brc1, brc2, sizeof(struct boardromconfig));
		}
	} else {
		for (int i = 0; expansionroms[i].name; i++) {
			const struct expansionromtype *ert = &expansionroms[i];
			for (int j = 0; j < MAX_BOARD_ROMS; j++) {
				board_prefs_changed(ert->romtype, j);
			}
		}
	}
}

bool is_board_enabled(struct uae_prefs *p, int romtype, int devnum)
{
	int idx;
	struct boardromconfig *brc = get_device_rom(p, romtype, devnum, &idx);
	if (!brc)
		return false;
	return brc->roms[idx].romfile[0] != 0;
}

static bool isspecialrom(const TCHAR *name)
{
	if (!_tcsicmp(name, _T(":NOROM")))
		return true;
	if (!_tcsicmp(name, _T(":ENABLED")))
		return true;
	return false;
}

static struct zfile *read_device_from_romconfig(struct romconfig *rc, uae_u32 romtype)
{
	struct zfile *z = NULL;
	if (isspecialrom(rc->romfile))
		return z;
	z = read_rom_name (rc->romfile);
	if (z)
		return z;
	if (romtype) {
		struct romlist *rl = getromlistbyromtype(romtype, NULL);
		if (rl) {
			struct romdata *rd = rl->rd;
			z = read_rom(rd);
		}
	}
	return z;
}

bool load_rom_rc(struct romconfig *rc, uae_u32 romtype, int maxfilesize, int fileoffset, uae_u8 *rom, int maxromsize, int flags)
{
	struct zfile *f = read_device_from_romconfig(rc, romtype);
	if (!f)
		return false;
	zfile_fseek(f, fileoffset, SEEK_SET);
	int cnt = 0;
	int pos = 0;
	int bytes = 0;
	bool eof = false;
	while (cnt < maxromsize && cnt < maxfilesize && pos < maxromsize) {
		uae_u8 b = 0xff;
		if (!eof) {
			if (!zfile_fread(&b, 1, 1, f))
				eof = true;
			else
				bytes++;
		}
		if (eof) {
			int bitcnt = 0;
			for (int i = 1; i < maxromsize; i <<= 1) {
				if (cnt & i)
					bitcnt++;
			}
			if (bitcnt == 1)
				break;
		}

		rom[pos] = b;
		pos += 1;
		cnt++;
	}
	if (f)
		write_log(_T("ROM '%s' loaded, %d bytes.\n"), zfile_getname(f), bytes);
	zfile_fclose(f);
	return true;
}
