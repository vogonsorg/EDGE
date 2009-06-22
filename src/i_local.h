//----------------------------------------------------------------------------
//  EDGE System Local definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2005-2009  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#ifndef __SYSTEM_LOCAL_H__
#define __SYSTEM_LOCAL_H__

// I_CTRL
void I_CentreMouse();

// I_MUSIC
extern bool musicpaused;
void I_PostMusicError(const char *message);

// UI
void I_MessageBox(const char* _msg, const char * _title);
// Used for outputing error messages/alerts on exit.

#endif /* __SYSTEM_LOCAL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
