/*
* UAE - The Un*x Amiga Emulator
*
* Copyright 2013 Toni Wilen
*
*/

#include "sysdeps.h"

#include "options.h"
#include "gfxboard.h"
#include "xwin.h"

bool gfxboard_set(bool rtg)
{
	bool r;
	struct amigadisplay *ad = &adisplays;
	r = ad->picasso_on;
	if (rtg) {
		ad->picasso_requested_on = 1;
	} else {
		ad->picasso_requested_on = 0;
	}
	set_config_changed();
	return r;
}

const TCHAR *gfxboard_get_configname(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return _T("ZorroII");
	if (type == GFXBOARD_UAE_Z3)
		return _T("ZorroIII");
	return NULL;
}

int gfxboard_get_configtype(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type == GFXBOARD_UAE_Z2)
		return 2;
	if (type == GFXBOARD_UAE_Z3)
		return 3;
	return 0;
}
