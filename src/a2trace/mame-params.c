#include <string.h>
#include <stddef.h>
#include <stdio.h>
/* Remove the annoying MAME symbols
 * and put back the address instead
 */

struct dasm_data
{
	int addr;
	const char *name;
};

static const struct dasm_data a2_stuff[] =
{
	{ 0x0020, "WNDLFT" }, { 0x0021, "WNDWDTH" }, { 0x0022, "WNDTOP" }, { 0x0023, "WNDBTM" },
	{ 0x0024, "CH" }, { 0x0025, "CV" }, { 0x0026, "GBASL" }, { 0x0027, "GBASH" },
	{ 0x0028, "BASL" }, { 0x0029, "BASH" }, { 0x002b, "BOOTSLOT" }, { 0x002c, "H2" },
	{ 0x002d, "V2" }, { 0x002e, "MASK" }, { 0x0030, "COLOR" }, { 0x0031, "MODE" },
	{ 0x0032, "INVFLG" }, { 0x0033, "PROMPT" }, { 0x0036, "CSWL" }, { 0x0037, "CSWH" },
	{ 0x0038, "KSWL" }, { 0x0039, "KSWH" }, { 0x0045, "ACC" }, { 0x0046, "XREG" },
	{ 0x0047, "YREG" }, { 0x0048, "STATUS" }, { 0x004E, "RNDL" }, { 0x004F, "RNDH" },
	{ 0x0067, "TXTTAB" }, { 0x0069, "VARTAB" }, { 0x006b, "ARYTAB" }, { 0x6d, "STREND" },
	{ 0x006f, "FRETOP" }, { 0x0071, "FRESPC" }, { 0x0073, "MEMSIZ" }, { 0x0075, "CURLIN" },
	{ 0x0077, "OLDLIN" }, { 0x0079, "OLDTEXT" }, { 0x007b, "DATLIN" }, { 0x007d, "DATPTR" },
	{ 0x007f, "INPTR" }, { 0x0081, "VARNAM" }, { 0x0083, "VARPNT" }, { 0x0085, "FORPNT" },
	{ 0x009A, "EXPON" }, { 0x009C, "EXPSGN" }, { 0x009d, "FAC" }, { 0x00A2, "FAC.SIGN" },
	{ 0x00a5, "ARG" }, { 0x00AA, "ARG.SIGN" }, { 0x00af, "PRGEND" }, { 0x00B8, "TXTPTR" },
	{ 0x00C9, "RNDSEED" }, { 0x00D6, "LOCK" }, { 0x00D8, "ERRFLG" }, { 0x00DA, "ERRLIN" },
	{ 0x00DE, "ERRNUM" }, { 0x00E4, "HGR.COLOR" }, { 0x00E6, "HGR.PAGE" }, { 0x00F1, "SPEEDZ" },

	{ 0xc000, "KBD / 80STOREOFF" }, { 0xc001, "80STOREON" }, { 0xc002, "RDMAINRAM" }, {0xc003, "RDCARDRAM" }, {0xc004, "WRMAINRAM" },
	{ 0xc005, "WRCARDRAM" }, { 0xc006, "SETSLOTCXROM" }, { 0xc007, "SETINTCXROM" }, { 0xc008, "SETSTDZP" },
	{ 0xc009, "SETALTZP "}, { 0xc00a, "SETINTC3ROM" }, { 0xc00b, "SETSLOTC3ROM" }, { 0xc00c, "CLR80VID" },
	{ 0xc00d, "SET80VID" }, { 0xc00e, "CLRALTCHAR" }, { 0xc00f, "SETALTCHAR" }, { 0xc010, "KBDSTRB" },
	{ 0xc011, "RDLCBNK2" }, { 0xc012, "RDLCRAM" }, { 0xc013, "RDRAMRD" }, { 0xc014, "RDRAMWRT" },
	{ 0xc015, "RDCXROM" }, { 0xc016, "RDALTZP" }, { 0xc017, "RDC3ROM" }, { 0xc018, "RD80STORE" },
	{ 0xc019, "RDVBL" }, { 0xc01a, "RDTEXT" }, { 0xc01b, "RDMIXED" }, { 0xc01c, "RDPAGE2" },
	{ 0xc01d, "RDHIRES" }, { 0xc01e, "RDALTCHAR" }, { 0xc01f, "RD80VID" }, { 0xc020, "TAPEOUT" },
	{ 0xc021, "MONOCOLOR" }, { 0xc022, "TBCOLOR" }, { 0xc023, "VGCINT" }, { 0xc024, "MOUSEDATA" },
	{ 0xc025, "KEYMODREG" }, { 0xc026, "DATAREG" }, { 0xc027, "KMSTATUS" }, { 0xc028, "ROMBANK" },
	{ 0xc029, "NEWVIDEO"}, { 0xc02b, "LANGSEL" }, { 0xc02c, "CHARROM" }, { 0xc02d, "SLOTROMSEL" },
	{ 0xc02e, "VERTCNT" }, { 0xc02f, "HORIZCNT" }, { 0xc030, "SPKR" }, { 0xc031, "DISKREG" },
	{ 0xc032, "SCANINT" }, { 0xc033, "CLOCKDATA" }, { 0xc034, "CLOCKCTL" }, { 0xc035, "SHADOW" },
	{ 0xc036, "FPIREG/CYAREG" }, { 0xc037, "BMAREG" }, { 0xc038, "SCCBREG" }, { 0xc039, "SCCAREG" },
	{ 0xc03a, "SCCBDATA" }, { 0xc03b, "SCCADATA" }, { 0xc03c, "SOUNDCTL" }, { 0xc03d, "SOUNDDATA" },
	{ 0xc03e, "SOUNDADRL" }, { 0xc03f, "SOUNDADRH" }, { 0xc040, "STROBE/RDXYMSK" }, { 0xc041, "RDVBLMSK" },
	{ 0xc042, "RDX0EDGE" }, { 0xc043, "RDY0EDGE" }, { 0xc044, "MMDELTAX" }, { 0xc045, "MMDELTAY" },
	{ 0xc046, "DIAGTYPE" }, { 0xc047, "CLRVBLINT" }, { 0xc048, "CLRXYINT" }, { 0xc04f, "EMUBYTE" },
	{ 0xc050, "TXTCLR" }, { 0xc051, "TXTSET" },
	{ 0xc052, "MIXCLR" }, { 0xc053, "MIXSET" }, { 0xc054, "TXTPAGE1" }, { 0xc055, "TXTPAGE2" },
	{ 0xc056, "LORES" }, { 0xc057, "HIRES" }, { 0xc058, "CLRAN0" }, { 0xc059, "SETAN0" },
	{ 0xc05a, "CLRAN1" }, { 0xc05b, "SETAN1" }, { 0xc05c, "CLRAN2" }, { 0xc05d, "SETAN2" },
	{ 0xc05e, "DHIRESON" }, { 0xc05f, "DHIRESOFF" }, { 0xc060, "TAPEIN" }, { 0xc061, "RDBTN0" },
	{ 0xc062, "BUTN1" }, { 0xc063, "RD63" }, { 0xc064, "PADDL0" }, { 0xc065, "PADDL1" },
	{ 0xc066, "PADDL2" }, { 0xc067, "PADDL3" }, { 0xc068, "STATEREG" }, { 0xc070, "PTRIG" }, { 0xc073, "BANKSEL" },
	{ 0xc07e, "IOUDISON" }, { 0xc07f, "IOUDISOFF" }, { 0xc081, "ROMIN" }, { 0xc083, "LCBANK2" },
	{ 0xc085, "ROMIN" }, { 0xc087, "LCBANK2" }, { 0xcfff, "DISCC8ROM" },

	{ 0xF800, "F8ROM_PLOT" }, { 0xF80E, "F8ROM_PLOT1" } , { 0xF819, "F8ROM_HLINE" }, { 0xF828, "F8ROM_VLINE" },
	{ 0xF832, "F8ROM_CLRSCR" }, { 0xF836, "F8ROM_CLRTOP" }, { 0xF838, "F8ROM_CLRSC2" }, { 0xF847, "F8ROM_GBASCALC" },
	{ 0xF856, "F8ROM_GBCALC" }, { 0xF85F, "F8ROM_NXTCOL" }, { 0xF864, "F8ROM_SETCOL" }, { 0xF871, "F8ROM_SCRN" },
	{ 0xF882, "F8ROM_INSDS1" }, { 0xF88E, "F8ROM_INSDS2" }, { 0xF8A5, "F8ROM_ERR" }, { 0xF8A9, "F8ROM_GETFMT" },
	{ 0xF8D0, "F8ROM_INSTDSP" }, { 0xF940, "F8ROM_PRNTYX" }, { 0xF941, "F8ROM_PRNTAX" }, { 0xF944, "F8ROM_PRNTX" },
	{ 0xF948, "F8ROM_PRBLNK" }, { 0xF94A, "F8ROM_PRBL2" },  { 0xF84C, "F8ROM_PRBL3" }, { 0xF953, "F8ROM_PCADJ" },
	{ 0xF854, "F8ROM_PCADJ2" }, { 0xF856, "F8ROM_PCADJ3" }, { 0xF85C, "F8ROM_PCADJ4" }, { 0xF962, "F8ROM_FMT1" },
	{ 0xF9A6, "F8ROM_FMT2" }, { 0xF9B4, "F8ROM_CHAR1" }, { 0xF9BA, "F8ROM_CHAR2" }, { 0xF9C0, "F8ROM_MNEML" },
	{ 0xFA00, "F8ROM_MNEMR" }, { 0xFA40, "F8ROM_OLDIRQ" }, { 0xFA4C, "F8ROM_BREAK" }, { 0xFA59, "F8ROM_OLDBRK" },
	{ 0xFA62, "F8ROM_RESET" }, { 0xFAA6, "F8ROM_PWRUP" }, { 0xFABA, "F8ROM_SLOOP" }, { 0xFAD7, "F8ROM_REGDSP" },
	{ 0xFADA, "F8ROM_RGDSP1" }, { 0xFAE4, "F8ROM_RDSP1" }, { 0xFB19, "F8ROM_RTBL" }, { 0xFB1E, "F8ROM_PREAD" },
	{ 0xFB21, "F8ROM_PREAD4" }, { 0xFB25, "F8ROM_PREAD2" }, { 0xFB2F, "F8ROM_INIT" }, { 0xFB39, "F8ROM_SETTXT" },
	{ 0xFB40, "F8ROM_SETGR" }, { 0xFB4B, "F8ROM_SETWND" }, { 0xFB51, "F8ROM_SETWND2" }, { 0xFB5B, "F8ROM_TABV" },
	{ 0xFB60, "F8ROM_APPLEII" }, { 0xFB6F, "F8ROM_SETPWRC" }, { 0xFB78, "F8ROM_VIDWAIT" }, { 0xFB88, "F8ROM_KBDWAIT" },
	{ 0xFBB3, "F8ROM_VERSION" }, { 0xFBBF, "F8ROM_ZIDBYTE2" }, { 0xFBC0, "F8ROM_ZIDBYTE" }, { 0xFBC1, "F8ROM_BASCALC" },
	{ 0xFBD0, "F8ROM_BSCLC2" }, { 0xFBDD, "F8ROM_BELL1" }, { 0xFBE2, "F8ROM_BELL1.2" }, { 0xFBE4, "F8ROM_BELL2" },
	{ 0xFBF0, "F8ROM_STORADV" }, { 0xFBF4, "F8ROM_ADVANCE" }, { 0xFBFD, "F8ROM_VIDOUT" }, { 0xFC10, "F8ROM_BS" },
	{ 0xFC1A, "F8ROM_UP" }, { 0xFC22, "F8ROM_VTAB" }, { 0xFC24, "F8ROM_VTABZ" }, { 0xFC42, "F8ROM_CLREOP" },
	{ 0xFC46, "F8ROM_CLEOP1" }, { 0xFC58, "F8ROM_HOME" }, { 0xFC62, "F8ROM_CR" }, { 0xFC66, "F8ROM_LF" },
	{ 0xFC70, "F8ROM_SCROLL" }, { 0xFC95, "F8ROM_SCRL3" }, { 0xFC9C, "F8ROM_CLREOL" }, { 0xFC9E, "F8ROM_CLREOLZ" },
	{ 0xFCA8, "F8ROM_WAIT" }, { 0xFCB4, "F8ROM_NXTA4" }, { 0xFCBA, "F8ROM_NXTA1" }, { 0xFCC9, "F8ROM_HEADR" },
	{ 0xFCEC, "F8ROM_RDBYTE" }, { 0xFCEE, "F8ROM_RDBYT2" }, { 0xFCFA, "F8ROM_RD2BIT" }, { 0xFD0C, "F8ROM_RDKEY" },
	{ 0xFD18, "F8ROM_RDKEY1" }, { 0xFD1B, "F8ROM_KEYIN" }, { 0xFD2F, "F8ROM_ESC" }, { 0xFD35, "F8ROM_RDCHAR" },
	{ 0xFD3D, "F8ROM_NOTCR" }, { 0xFD62, "F8ROM_CANCEL" }, { 0xFD67, "F8ROM_GETLNZ" }, { 0xFD6A, "F8ROM_GETLN" },
	{ 0xFD6C, "F8ROM_GETLN0" }, { 0xFD6F, "F8ROM_GETLN1" }, { 0xFD8B, "F8ROM_CROUT1" }, { 0xFD8E, "F8ROM_CROUT" },
	{ 0xFD92, "F8ROM_PRA1" }, { 0xFDA3, "F8ROM_XAM8" }, { 0xFDDA, "F8ROM_PRBYTE" }, { 0xFDE3, "F8ROM_PRHEX" },
	{ 0xFDE5, "F8ROM_PRHEXZ" }, { 0xFDED, "F8ROM_COUT" }, { 0xFDF0, "F8ROM_COUT1" }, { 0xFDF6, "F8ROM_COUTZ" },
	{ 0xFE18, "F8ROM_SETMODE" }, { 0xFE1F, "F8ROM_IDROUTINE" }, { 0xFE20, "F8ROM_LT" }, { 0xFE22, "F8ROM_LT2" },
	{ 0xFE2C, "F8ROM_MOVE" }, { 0xFE36, "F8ROM_VFY" }, { 0xFE5E, "F8ROM_LIST" }, { 0xFE63, "F8ROM_LIST2" },
	{ 0xFE75, "F8ROM_A1PC" }, { 0xFE80, "F8ROM_SETINV" }, { 0xFE84, "F8ROM_SETNORM" }, { 0xFE89, "F8ROM_SETKBD" },
	{ 0xFE8B, "F8ROM_INPORT" }, { 0xFE8D, "F8ROM_INPRT" }, { 0xFE93, "F8ROM_SETVID" }, { 0xFE95, "F8ROM_OUTPORT" },
	{ 0xFE97, "F8ROM_OUTPRT" }, { 0xFEB0, "F8ROM_XBASIC" }, { 0xFEB3, "F8ROM_BASCONT" }, { 0xFEB6, "F8ROM_GO" },
	{ 0xFECA, "F8ROM_USR" }, { 0xFECD, "F8ROM_WRITE" }, { 0xFEFD, "F8ROM_READ" }, { 0xFF2D, "F8ROM_PRERR" },
	{ 0xFF3A, "F8ROM_BELL" }, { 0xFF3F, "F8ROM_RESTORE" }, { 0xFF4A, "F8ROM_SAVE" }, { 0xFF58, "F8ROM_IORTS" },
	{ 0xFF59, "F8ROM_OLDRST" }, { 0xFF65, "F8ROM_MON" }, { 0xFF69, "F8ROM_MONZ" }, { 0xFF6C, "F8ROM_MONZ2" },
	{ 0xFF70, "F8ROM_MONZ4" }, { 0xFF8A, "F8ROM_DIG" }, { 0xFFA7, "F8ROM_GETNUM" }, { 0xFFAD, "F8ROM_NXTCHR" },
	{ 0xFFBE, "F8ROM_TOSUB" }, { 0xFFC7, "F8ROM_ZMODE" }, { 0xFFCC, "F8ROM_CHRTBL" }, { 0xFFE3, "F8ROM_SUBTBL" },

	{ 0xffff, "" }
};

static char parambuf[7];
const char * fix_mame_param(const char *param) {
  int low = 0, high = 0, i = 0;
  if (!param)
    return NULL;
  if (param[0] == '$'
   || param[0] == '#')
    return param;

  if (param[0] == '<') {
    low = 1;
    param++;
  }
  if (param[0] == '>') {
    high = 1;
    param++;
  }

  if (!strcmp(param, "($03fe)"))
    return "$befb";

  while (a2_stuff[i].addr != 0xffff) {
    if (!strcmp(param, a2_stuff[i].name)) {
      int val = a2_stuff[i].addr;
      if (low) {
        val = (val & 0xff);
        snprintf(parambuf, sizeof(parambuf), "$%02x", val);
      }
      else if (high) {
        val = (val & 0xff00) >> 8;
        snprintf(parambuf, sizeof(parambuf), "$%02x", val);
      } else {
        snprintf(parambuf, sizeof(parambuf), "$%04x", val);
      }
      return parambuf;
    }
    i++;
  }

  return param;
}
