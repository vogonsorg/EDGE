//------------------------------------------------------------------------
//  LEVEL building - Quake1/2 PAK files
//------------------------------------------------------------------------
//
//  Oblige Level Maker (C) 2006-2008 Andrew Apted
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
//------------------------------------------------------------------------

#ifndef __PAK_FILES_H__
#define __PAK_FILES_H__


/* PAK reading */

bool PAK_OpenRead(const char *filename);
void PAK_CloseRead(void);

int  PAK_FindEntry(const char *name);
void PAK_FindMaps(std::vector<int>& entries);

int  PAK_EntryLen(int entry);
bool PAK_ReadData(int entry, int offset, int length, void *buffer);


/* PAK writing */

bool PAK_OpenWrite(const char *filename);
void PAK_CloseWrite(void);

void PAK_NewLump(const char *name);
void PAK_AppendData(const void *data, int length);
void PAK_FinishLump(void);


/* WAD2 reading */

bool WAD2_OpenRead(const char *filename);
void WAD2_CloseRead(void);

int  WAD2_FindEntry(const char *name);
int  WAD2_EntryLen(int entry);
bool WAD2_ReadData(int entry, int offset, int length, void *buffer);


/* WAD2 writing */

bool WAD2_OpenWrite(const char *filename);
void WAD2_CloseWrite(void);

void WAD2_NewLump(const char *name);
void WAD2_AppendData(const void *data, int length);
void WAD2_FinishLump(void);


/* ----- PAK structures ---------------------- */

typedef struct
{
  char magic[4];

  u32_t dir_start;
  u32_t entry_num;
}
raw_pak_header_t;

#define PAK_MAGIC  "PACK"


typedef struct
{
  char name[56];

  u32_t offset;
  u32_t length;
}
raw_pak_entry_t;


/* ----- WAD2 structures ---------------------- */

typedef struct
{
  char magic[4];

  u32_t num_lumps;
  u32_t dir_start;
}
raw_wad2_header_t;

#define WAD2_MAGIC  "WAD2"


typedef struct
{
  u32_t start;
  u32_t length;  // compressed
  u32_t u_len;   // uncompressed

  u8_t  type;
  u8_t  compression;
  u8_t  _pad[2];

  char  name[16];  // must be null terminated
}
raw_wad2_lump_t;


#endif /* __PAK_FILES_H__ */

//--- editor settings ---
// vi:ts=2:sw=2:expandtab