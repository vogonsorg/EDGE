//------------------------------------------------------------------------
//  OBJECT OPERATIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 Andr� Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <algorithm>

#include "r_misc.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "e_things.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"

#include "editloop.h"
#include "r_grid.h"
#include "x_hover.h"



/*
   get the number of objects of a given type minus one
*/
obj_no_t GetMaxObjectNum (int objtype)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			return NumThings - 1;
		case OBJ_LINEDEFS:
			return NumLineDefs - 1;
		case OBJ_SIDEDEFS:
			return NumSideDefs - 1;
		case OBJ_VERTICES:
			return NumVertices - 1;
		case OBJ_SECTORS:
			return NumSectors - 1;
	}
	return -1;
}


static int RawCreateThing()
{
	int objnum = NumThings;

	NumThings++;

	if (objnum > 0)
		Things = (TPtr) ResizeMemory (Things,
				(unsigned long) NumThings * sizeof (struct Thing));
	else
		Things = (TPtr) GetMemory (sizeof (struct Thing));

	memset(&Things[objnum], 0, sizeof (struct Thing));

	return objnum;
}


static int RawCreateLineDef()
{
	int objnum = NumLineDefs;

	NumLineDefs++;

	if (objnum > 0)
		LineDefs = (LDPtr) ResizeMemory (LineDefs,
				(unsigned long) NumLineDefs * sizeof (struct LineDef));
	else
		LineDefs = (LDPtr) GetMemory (sizeof (struct LineDef));

	memset(&LineDefs[objnum], 0, sizeof (struct LineDef));

	return objnum;
}


static int RawCreateSideDef()
{
	int objnum = NumSideDefs;

	NumSideDefs++;

	if (objnum > 0)
		SideDefs = (SDPtr) ResizeMemory (SideDefs,
				(unsigned long) NumSideDefs * sizeof (struct SideDef));
	else
		SideDefs = (SDPtr) GetMemory (sizeof (struct SideDef));

	memset(&SideDefs[objnum], 0, sizeof (struct SideDef));

	return objnum;
}


static int RawCreateVertex()
{
	int objnum = NumVertices;

	NumVertices++;

	if (objnum > 0)
		Vertices = (VPtr) ResizeMemory (Vertices,
				(unsigned long) NumVertices * sizeof (struct Vertex));
	else
		Vertices = (VPtr) GetMemory (sizeof (struct Vertex));

	memset(&Vertices[objnum], 0, sizeof (struct Vertex));

	return objnum;
}


static int RawCreateSector()
{
	int objnum = NumSectors;

	NumSectors++;

	if (objnum > 0)
		Sectors = (SPtr) ResizeMemory (Sectors,
				(unsigned long) NumSectors * sizeof (struct Sector));
	else
		Sectors = (SPtr) GetMemory (sizeof (struct Sector));

	memset(&Sectors[objnum], 0, sizeof (struct Sector));

	return objnum;
}



static void RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	// Delete the thing
	NumThings--;

	if (NumThings > 0)
	{
		for (int n = objnum; n < NumThings; n++)
			Things[n] = Things[n + 1];
	}
	else
	{
		FreeMemory (Things);
		Things = NULL;
	}
}


static void RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	NumVertices--;

	if (NumVertices > 0)
	{
		for (int n = objnum; n < NumVertices; n++)
			Vertices[n] = Vertices[n + 1];
	}
	else
	{
		FreeMemory (Vertices);
		Vertices = NULL;
	}
}


static void RawDeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	NumSectors--;

	if (NumSectors > 0)
	{
		for (int n = objnum; n < NumSectors; n++)
			Sectors[n] = Sectors[n + 1];
	}
	else
	{
		FreeMemory (Sectors);
		Sectors = NULL;
	}
}


static void RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	NumLineDefs--;

	if (NumLineDefs > 0)
	{
		for (int n = objnum; n < NumLineDefs; n++)
			LineDefs[n] = LineDefs[n + 1];
	}
	else
	{
		FreeMemory (LineDefs);
		LineDefs = NULL;
	}
}


static void RawDeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	NumSideDefs--;

	if (NumSideDefs > 0)
	{
		for (int n = objnum; n < NumSideDefs; n++)
			SideDefs[n] = SideDefs[n + 1];
	}
	else
	{
		FreeMemory (SideDefs);
		SideDefs = NULL;
	}
}


/*
   delete a group of objects
*/
void DeleteObjects(selection_c *list)
{
	int objtype = list->what_type();


	// we need to process the object numbers from highest to lowest,
	// because each deletion invalidates all higher-numbered refs
	// in the selection.  Our selection iterator cannot give us
	// what we need, hence put them into a vector for sorting.

	std::vector<int> all_ids;

	selection_iterator_c it;
	for (list->begin(&it); !it.at_end(); ++it)
		all_ids.push_back(*it);

	std::sort(all_ids.begin(), all_ids.end());


	// the passed in selection is now invalid.  Hence we clear it,
	// but we also re-use it for sidedefs which must be deleted
	// (due to their sectors going away).

	list->change_type(OBJ_SIDEDEFS);

	SYS_ASSERT(list->empty());


	for (int i = (int)all_ids.size()-1; i >= 0; i--)
	{
		int objnum = all_ids[i];

		switch (objtype)
		{
			case OBJ_THINGS:
				RawDeleteThing(objnum);
				break;

			case OBJ_LINEDEFS:
				RawDeleteLineDef(objnum);
				break;

			case OBJ_VERTICES:
				RawDeleteVertex(objnum);

				// delete the linedefs bound to this vertex and
				// fix the references

				for (int n = NumLineDefs-1; n >= 0; n--)
				{
					LineDef *L = &LineDefs[n];

					if (L->start == objnum || L->end == objnum)
						RawDeleteLineDef(n);
					else
					{
						if (L->start > objnum)
							L->start--;
						if (L->end > objnum)
							L->end--;
					}
				}
				break;

			case OBJ_SIDEDEFS:
				RawDeleteSideDef(objnum);

				// fix the linedefs references

				for (int n = NumLineDefs-1; n >= 0; n--)
				{
					LineDef *L = &LineDefs[n];

					if (L->side_R == objnum)
						L->side_R = -1;
					else if (L->side_R > objnum)
						L->side_R--;

					if (L->side_L == objnum)
						L->side_L = -1;
					else if (L->side_L > objnum)
						L->side_L--;
				}
				break;
			
			case OBJ_SECTORS:
				RawDeleteSector(objnum);

				// delete the sidedefs bound to this sector and
				// fix the references

				for (int n = NumSideDefs-1; n >= 0; n--)
				{
					SideDef *S = &SideDefs[n];

					if (S->sector == objnum)
					{
						list->set(n);
						S->sector = -1;
					}
					else if (S->sector > objnum)
						S->sector--;
				}
				break;

			default:
				nf_bug ("DeleteObjects: bad objtype %d", (int) objtype);
		}
	}


	if (list->notempty())
	{
		DeleteObjects(list);
	}

	list->change_type(objtype);

	MadeChanges = 1;
}


void DeleteObject(const Objid& obj)
{
	// things and linedefs are easy, since nothing else references them
	if (obj.type == OBJ_THINGS)
	{
		RawDeleteVertex(obj.num);
		return;
	}
	if (obj.type == OBJ_LINEDEFS)
	{
		RawDeleteLineDef(obj.num);
		return;
	}

	selection_c list(obj.type, obj.num);

	DeleteObjects(&list);
}


/*
 *  InsertObject
 *
 *  Insert a new object of type <objtype> at map coordinates
 *  (<xpos>, <ypos>).
 *
 *  If <copyfrom> is a valid object number, the other properties
 *  of the new object are set from the properties of that object,
 *  with the exception of sidedef numbers, which are forced
 *  to OBJ_NO_NONE.
 *
 *  The object is inserted at the exact coordinates given.
 *  No snapping to grid is done.
 */
int InsertObject(obj_type_t objtype, obj_no_t copyfrom, int xpos, int ypos)
{
	MadeChanges = 1;

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int objnum = RawCreateThing();

			Thing *T = &Things[objnum];

			T->x = xpos;
			T->y = ypos;

			if (is_obj(copyfrom))
			{
				T->type    = Things[copyfrom].type;
				T->angle   = Things[copyfrom].angle;
				T->options = Things[copyfrom].options;
			}
			else
			{
				T->type = default_thing;
				T->angle = 0;
				T->options = 0x07;
			}

			return objnum;
		}

		case OBJ_VERTICES:
		{
			int objnum = RawCreateVertex();

			Vertex *V = &Vertices[objnum];

			V->x = xpos;
			V->y = ypos;

			if (xpos < MapMinX) MapMinX = xpos;
			if (ypos < MapMinY) MapMinY = ypos;
			if (xpos > MapMaxX) MapMaxX = xpos;
			if (ypos > MapMaxY) MapMaxY = ypos;

			MadeMapChanges = 1;

			return objnum;
		}

		case OBJ_LINEDEFS:
		{
			int objnum = RawCreateLineDef();

			LineDef *L = &LineDefs[objnum];

			if (is_obj (copyfrom))
			{
				L->start = LineDefs[copyfrom].start;
				L->end   = LineDefs[copyfrom].end;
				L->flags = LineDefs[copyfrom].flags;
				L->type  = LineDefs[copyfrom].type;
				L->tag   = LineDefs[copyfrom].tag;
			}
			else
			{
				L->start = 0;
				L->end   = NumVertices - 1;
				L->flags = 1;
			}

			L->side_R = OBJ_NO_NONE;
			L->side_L = OBJ_NO_NONE;

			return objnum;
		}

		case OBJ_SIDEDEFS:
		{
			int objnum = RawCreateSideDef();

			SideDef *S = &SideDefs[objnum];

			if (is_obj(copyfrom))
			{
				S->x_offset = SideDefs[copyfrom].x_offset;
				S->y_offset = SideDefs[copyfrom].y_offset;
				S->sector   = SideDefs[copyfrom].sector;

				strncpy(S->upper_tex, SideDefs[copyfrom].upper_tex, WAD_TEX_NAME);
				strncpy(S->lower_tex, SideDefs[copyfrom].lower_tex, WAD_TEX_NAME);
				strncpy(S->mid_tex,   SideDefs[copyfrom].mid_tex,   WAD_TEX_NAME);
			}
			else
			{
				S->sector = NumSectors - 1;

				strcpy(S->upper_tex, "-");
				strcpy(S->lower_tex, "-");
				strcpy(S->mid_tex, default_middle_texture);
			}
			
			MadeMapChanges = 1;

			return objnum;
		}

		case OBJ_SECTORS:
		{
			int objnum = RawCreateSector();

			Sector *S = &Sectors[objnum];

			if (is_obj(copyfrom))
			{
				S->floorh = Sectors[copyfrom].floorh;
				S->ceilh  = Sectors[copyfrom].ceilh;
				S->light  = Sectors[copyfrom].light;
				S->type   = Sectors[copyfrom].type;
				S->tag    = Sectors[copyfrom].tag;

				strncpy(S->floor_tex, Sectors[copyfrom].floor_tex, WAD_FLAT_NAME);
				strncpy(S->ceil_tex,  Sectors[copyfrom].ceil_tex, WAD_FLAT_NAME);
			}
			else
			{
				S->floorh = default_floor_height;
				S->ceilh  = default_ceiling_height;
				S->light  = default_light_level;

				strncpy(S->floor_tex, default_floor_texture, WAD_FLAT_NAME);
				strncpy(S->ceil_tex,  default_ceiling_texture, WAD_FLAT_NAME);
			}

			return objnum;
		}

		default:
			nf_bug ("InsertObject: bad objtype %d", (int) objtype);
	}

	return OBJ_NO_NONE;  /* NOT REACHED */
}



/*
   check if a (part of a) LineDef is inside a given block
*/
bool IsLineDefInside (int ldnum, int x0, int y0, int x1, int y1) /* SWAP - needs Vertices & LineDefs */
{
	int lx0 = Vertices[LineDefs[ldnum].start].x;
	int ly0 = Vertices[LineDefs[ldnum].start].y;
	int lx1 = Vertices[LineDefs[ldnum].end].x;
	int ly1 = Vertices[LineDefs[ldnum].end].y;
	int i;

	/* do you like mathematics? */
	if (lx0 >= x0 && lx0 <= x1 && ly0 >= y0 && ly0 <= y1)
		return 1; /* the linedef start is entirely inside the square */
	if (lx1 >= x0 && lx1 <= x1 && ly1 >= y0 && ly1 <= y1)
		return 1; /* the linedef end is entirely inside the square */
	if ((ly0 > y0) != (ly1 > y0))
	{
		i = lx0 + (int) ((long) (y0 - ly0) * (long) (lx1 - lx0) / (long) (ly1 - ly0));
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the y0 side (left) */
	}
	if ((ly0 > y1) != (ly1 > y1))
	{
		i = lx0 + (int) ((long) (y1 - ly0) * (long) (lx1 - lx0) / (long) (ly1 - ly0));
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the y1 side (right) */
	}
	if ((lx0 > x0) != (lx1 > x0))
	{
		i = ly0 + (int) ((long) (x0 - lx0) * (long) (ly1 - ly0) / (long) (lx1 - lx0));
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the x0 side (down) */
	}
	if ((lx0 > x1) != (lx1 > x1))
	{
		i = ly0 + (int) ((long) (x1 - lx0) * (long) (ly1 - ly0) / (long) (lx1 - lx0));
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the x1 side (up) */
	}
	return false;
}



/*
   get the sector number of the sidedef opposite to this sidedef
   (returns -1 if it cannot be found)
   */
int GetOppositeSector (int ld1, bool firstside)
{
	int x0, y0, dx0, dy0;
	int x1, y1, dx1, dy1;
	int x2, y2, dx2, dy2;
	int ld2, dist;
	int bestld, bestdist, bestmdist;

	/* get the coords for this LineDef */

	x0  = Vertices[LineDefs[ld1].start].x;
	y0  = Vertices[LineDefs[ld1].start].y;
	dx0 = Vertices[LineDefs[ld1].end].x - x0;
	dy0 = Vertices[LineDefs[ld1].end].y - y0;

	/* find the normal vector for this LineDef */
	x1  = (dx0 + x0 + x0) / 2;
	y1  = (dy0 + y0 + y0) / 2;
	if (firstside)
	{
		dx1 = dy0;
		dy1 = -dx0;
	}
	else
	{
		dx1 = -dy0;
		dy1 = dx0;
	}

	bestld = -1;
	/* use a parallel to an axis instead of the normal vector (faster method) */
	if (abs (dy1) > abs (dx1))
	{
		if (dy1 > 0)
		{
			/* get the nearest LineDef in that direction (increasing Y's: North) */
			bestdist = 32767;
			bestmdist = 32767;
			for (ld2 = 0; ld2 < NumLineDefs; ld2++)
				if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].x > x1)
							!= (Vertices[LineDefs[ld2].end].x > x1)))
				{
					x2  = Vertices[LineDefs[ld2].start].x;
					y2  = Vertices[LineDefs[ld2].start].y;
					dx2 = Vertices[LineDefs[ld2].end].x - x2;
					dy2 = Vertices[LineDefs[ld2].end].y - y2;
					dist = y2 + (int) ((long) (x1 - x2) * (long) dy2 / (long) dx2);
					if (dist > y1 && (dist < bestdist
								|| (dist == bestdist && (y2 + dy2 / 2) < bestmdist)))
					{
						bestld = ld2;
						bestdist = dist;
						bestmdist = y2 + dy2 / 2;
					}
				}
		}
		else
		{
			/* get the nearest LineDef in that direction (decreasing Y's: South) */
			bestdist = -32767;
			bestmdist = -32767;
			for (ld2 = 0; ld2 < NumLineDefs; ld2++)
				if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].x > x1)
							!= (Vertices[LineDefs[ld2].end].x > x1)))
				{
					x2  = Vertices[LineDefs[ld2].start].x;
					y2  = Vertices[LineDefs[ld2].start].y;
					dx2 = Vertices[LineDefs[ld2].end].x - x2;
					dy2 = Vertices[LineDefs[ld2].end].y - y2;
					dist = y2 + (int) ((long) (x1 - x2) * (long) dy2 / (long) dx2);
					if (dist < y1 && (dist > bestdist
								|| (dist == bestdist && (y2 + dy2 / 2) > bestmdist)))
					{
						bestld = ld2;
						bestdist = dist;
						bestmdist = y2 + dy2 / 2;
					}
				}
		}
	}
	else
	{
		if (dx1 > 0)
		{
			/* get the nearest LineDef in that direction (increasing X's: East) */
			bestdist = 32767;
			bestmdist = 32767;
			for (ld2 = 0; ld2 < NumLineDefs; ld2++)
				if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].y > y1)
							!= (Vertices[LineDefs[ld2].end].y > y1)))
				{
					x2  = Vertices[LineDefs[ld2].start].x;
					y2  = Vertices[LineDefs[ld2].start].y;
					dx2 = Vertices[LineDefs[ld2].end].x - x2;
					dy2 = Vertices[LineDefs[ld2].end].y - y2;
					dist = x2 + (int) ((long) (y1 - y2) * (long) dx2 / (long) dy2);
					if (dist > x1 && (dist < bestdist
								|| (dist == bestdist && (x2 + dx2 / 2) < bestmdist)))
					{
						bestld = ld2;
						bestdist = dist;
						bestmdist = x2 + dx2 / 2;
					}
				}
		}
		else
		{
			/* get the nearest LineDef in that direction (decreasing X's: West) */
			bestdist = -32767;
			bestmdist = -32767;
			for (ld2 = 0; ld2 < NumLineDefs; ld2++)
				if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].y > y1)
							!= (Vertices[LineDefs[ld2].end].y > y1)))
				{
					x2  = Vertices[LineDefs[ld2].start].x;
					y2  = Vertices[LineDefs[ld2].start].y;
					dx2 = Vertices[LineDefs[ld2].end].x - x2;
					dy2 = Vertices[LineDefs[ld2].end].y - y2;
					dist = x2 + (int) ((long) (y1 - y2) * (long) dx2 / (long) dy2);
					if (dist < x1 && (dist > bestdist
								|| (dist == bestdist && (x2 + dx2 / 2) > bestmdist)))
					{
						bestld = ld2;
						bestdist = dist;
						bestmdist = x2 + dx2 / 2;
					}
				}
		}
	}

	/* no intersection: the LineDef was pointing outwards! */
	if (bestld < 0)
		return -1;

	/* now look if this LineDef has a SideDef bound to one sector */
	if (abs (dy1) > abs (dx1))
	{
		if ((Vertices[LineDefs[bestld].start].x
					< Vertices[LineDefs[bestld].end].x) == (dy1 > 0))
			x0 = LineDefs[bestld].side_R;
		else
			x0 = LineDefs[bestld].side_L;
	}
	else
	{
		if ((Vertices[LineDefs[bestld].start].y
					< Vertices[LineDefs[bestld].end].y) != (dx1 > 0))
			x0 = LineDefs[bestld].side_R;
		else
			x0 = LineDefs[bestld].side_L;
	}

	/* there is no SideDef on this side of the LineDef! */
	if (x0 < 0)
		return -1;

	/* OK, we got it -- return the Sector number */

	return SideDefs[x0].sector;
}



/*
   copy a group of objects to a new position
*/
void CopyObjects(selection_c *list)
{
#if 0  // FIXME !!!!! CopyObjects
	int        n, m;
	SelPtr     cur;
	SelPtr     list1, list2;
	SelPtr     ref1, ref2;

	if (list->empty())
		return;

	/* copy the object(s) */
	switch (objtype)
	{
		case OBJ_THINGS:
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_THINGS, cur->objnum, Things[cur->objnum].x,
						Things[cur->objnum].y);
				cur->objnum = NumThings - 1;
			}
			MadeChanges = 1;
			break;

		case OBJ_VERTICES:
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_VERTICES, cur->objnum, Vertices[cur->objnum].x,
						Vertices[cur->objnum].y);
				cur->objnum = NumVertices - 1;
			}
			MadeChanges = 1;
			MadeMapChanges = 1;
			break;

		case OBJ_LINEDEFS:
			list1 = 0;
			list2 = 0;

			// Create the linedefs and maybe the sidedefs
			for (cur = obj; cur; cur = cur->next)
			{
				int old = cur->objnum; // No. of original linedef
				int New;   // No. of duplicate linedef

				InsertObject (OBJ_LINEDEFS, old, 0, 0);
				New = NumLineDefs - 1;

				if (false) ///!!! copy_linedef_reuse_sidedefs)
				{
		/* AYM 1997-07-25: not very orthodox (the New linedef and 
		   the old one use the same sidedefs). but, in the case where
		   you're copying into the same sector, it's much better than
		   having to create the New sidedefs manually. plus it saves
		   space in the .wad and also it makes editing easier (editing
		   one sidedef impacts all linedefs that use it). */
					LineDefs[New].side_R = LineDefs[old].side_R; 
					LineDefs[New].side_L = LineDefs[old].side_L; 
				}
				else
				{
					/* AYM 1998-11-08: duplicate sidedefs too.
					   DEU 5.21 just left the sidedef references to -1. */
					if (is_sidedef (LineDefs[old].side_R))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].side_R, 0, 0);
						LineDefs[New].side_R = NumSideDefs - 1;
					}
					if (is_sidedef (LineDefs[old].side_L))
					{
						InsertObject (OBJ_SIDEDEFS, LineDefs[old].side_L, 0, 0);
						LineDefs[New].side_L = NumSideDefs - 1; 
					}
				}
				cur->objnum = New;
				if (!IsSelected (list1, LineDefs[New].start))
				{
					SelectObject (&list1, LineDefs[New].start);
					SelectObject (&list2, LineDefs[New].start);
				}
				if (!IsSelected (list1, LineDefs[New].end))
				{
					SelectObject (&list1, LineDefs[New].end);
					SelectObject (&list2, LineDefs[New].end);
				}
			}

			// Create the vertices
			CopyObjects (OBJ_VERTICES, list2);


			// Update the references to the vertices
			for (ref1 = list1, ref2 = list2;
					ref1 && ref2;
					ref1 = ref1->next, ref2 = ref2->next)
			{
				for (cur = obj; cur; cur = cur->next)
				{
					if (ref1->objnum == LineDefs[cur->objnum].start)
						LineDefs[cur->objnum].start = ref2->objnum;
					if (ref1->objnum == LineDefs[cur->objnum].end)
						LineDefs[cur->objnum].end = ref2->objnum;
				}
			}
			ForgetSelection (&list1);
			ForgetSelection (&list2);
			break;

		case OBJ_SECTORS:

			list1 = 0;
			list2 = 0;
			// Create the linedefs (and vertices)
			for (cur = obj; cur; cur = cur->next)
			{
				for (n = 0; n < NumLineDefs; n++)
					if  ((((m = LineDefs[n].side_R) >= 0
									&& SideDefs[m].sector == cur->objnum)
								|| ((m = LineDefs[n].side_L) >= 0
									&& SideDefs[m].sector == cur->objnum))
							&& ! IsSelected (list1, n))
					{
						SelectObject (&list1, n);
						SelectObject (&list2, n);
					}
			}
			CopyObjects (OBJ_LINEDEFS, list2);
			/* create the sidedefs */

			for (ref1 = list1, ref2 = list2;
					ref1 && ref2;
					ref1 = ref1->next, ref2 = ref2->next)
			{
				if ((n = LineDefs[ref1->objnum].side_R) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, n, 0, 0);
					n = NumSideDefs - 1;

					LineDefs[ref2->objnum].side_R = n;
				}
				if ((m = LineDefs[ref1->objnum].side_L) >= 0)
				{
					InsertObject (OBJ_SIDEDEFS, m, 0, 0);
					m = NumSideDefs - 1;

					LineDefs[ref2->objnum].side_L = m;
				}
				ref1->objnum = n;
				ref2->objnum = m;
			}
			/* create the Sectors */
			for (cur = obj; cur; cur = cur->next)
			{
				InsertObject (OBJ_SECTORS, cur->objnum, 0, 0);

				for (ref1 = list1, ref2 = list2;
						ref1 && ref2;
						ref1 = ref1->next, ref2 = ref2->next)
				{
					if (ref1->objnum >= 0
							&& SideDefs[ref1->objnum].sector == cur->objnum)
						SideDefs[ref1->objnum].sector = NumSectors - 1;
					if (ref2->objnum >= 0
							&& SideDefs[ref2->objnum].sector == cur->objnum)
						SideDefs[ref2->objnum].sector = NumSectors - 1;
				}
				cur->objnum = NumSectors - 1;
			}
			ForgetSelection (&list1);
			ForgetSelection (&list2);
			break;
	}
#endif
}



/*
 *  MoveObjectsToCoords
 *
 *  Move a group of objects to a new position
 *
 *  You must first call it with obj == NULL and newx and newy
 *  set to the coordinates of the reference point (E.G. the
 *  object being dragged).
 *  Then, every time the object being dragged has changed its
 *  coordinates, call the it again with newx and newy set to
 *  the new position and obj set to the selection.
 *
 *  Returns <>0 iff an object was moved.
 */
bool MoveObjectsToCoords (int objtype, SelPtr obj, int newx, int newy, int grid)
{
#if 0  // FIXME !!!!!
	int        dx, dy;
	SelPtr     cur, vertices;
	static int refx, refy; /* previous position */

	if (grid > 0)
	{
		newx = (newx + grid / 2) & ~(grid - 1);
		newy = (newy + grid / 2) & ~(grid - 1);
	}

	// Only update the reference point ?
	if (! obj)
	{
		refx = newx;
		refy = newy;
		return true;
	}

	/* compute the displacement */
	dx = newx - refx;
	dy = newy - refy;
	/* nothing to do? */
	if (dx == 0 && dy == 0)
		return false;

	/* move the object(s) */
	switch (objtype)
	{
		case OBJ_THINGS:
			for (cur = obj; cur; cur = cur->next)
			{
				Things[cur->objnum].x += dx;
				Things[cur->objnum].y += dy;
			}
			refx = newx;
			refy = newy;
			MadeChanges = 1;
			break;

		case OBJ_VERTICES:
			for (cur = obj; cur; cur = cur->next)
			{
				Vertices[cur->objnum].x += dx;
				Vertices[cur->objnum].y += dy;
			}
			refx = newx;
			refy = newy;
			MadeChanges = 1;
			MadeMapChanges = 1;
			break;

		case OBJ_LINEDEFS:
			vertices = list_vertices_of_linedefs (obj);
			MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
			ForgetSelection (&vertices);
			break;

		case OBJ_SECTORS:

			vertices = list_vertices_of_sectors (obj);
			MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
			ForgetSelection (&vertices);
			break;
	}
	return true;
#endif

	return false;
}



/*
   get the coordinates (approx.) of an object
*/
void GetObjectCoords (int objtype, int objnum, int *xpos, int *ypos)
{
	int  n, v1, v2, sd1, sd2;
	long accx, accy, num;

	switch (objtype)
	{
		case OBJ_THINGS:
			if (! is_thing (objnum))    // Can't happen
			{
				nf_bug ("GetObjectCoords: bad thing# %d", objnum);
				*xpos = 0;
				*ypos = 0;
				return;
			}

			*xpos = Things[objnum].x;
			*ypos = Things[objnum].y;
			break;

		case OBJ_VERTICES:
			if (! is_vertex (objnum))   // Can't happen
			{
				nf_bug ("GetObjectCoords: bad vertex# %d", objnum);
				*xpos = 0;
				*ypos = 0;
				return;
			}

			*xpos = Vertices[objnum].x;
			*ypos = Vertices[objnum].y;
			break;

		case OBJ_LINEDEFS:
			if (! is_linedef (objnum))  // Can't happen
			{
				nf_bug ("GetObjectCoords: bad linedef# %d", objnum);
				*xpos = 0;
				*ypos = 0;
				return;
			}

			v1 = LineDefs[objnum].start;
			v2 = LineDefs[objnum].end;

			*xpos = (Vertices[v1].x + Vertices[v2].x) / 2;
			*ypos = (Vertices[v1].y + Vertices[v2].y) / 2;
			break;

		case OBJ_SIDEDEFS:
			if (! is_sidedef (objnum))  // Can't happen
			{
				nf_bug ("GetObjectCoords: bad sidedef# %d", objnum);
				*xpos = 0;
				*ypos = 0;
				return;
			}

			for (n = 0; n < NumLineDefs; n++)
				if (LineDefs[n].side_R == objnum || LineDefs[n].side_L == objnum)
				{
					v1 = LineDefs[n].start;
					v2 = LineDefs[n].end;

					*xpos = (Vertices[v1].x + Vertices[v2].x) / 2;
					*ypos = (Vertices[v1].y + Vertices[v2].y) / 2;
					return;
				}
			*xpos = (MapMinX + MapMaxX) / 2;
			*ypos = (MapMinY + MapMaxY) / 2;
			// FIXME is the fall through intentional ? -- AYM 2000-11-08

		case OBJ_SECTORS:
			if (! is_sector (objnum))   // Can't happen
			{
				nf_bug ("GetObjectCoords: bad sector# %d", objnum);
				*xpos = 0;
				*ypos = 0;
				return;
			}
			accx = 0L;
			accy = 0L;
			num = 0L;
			for (n = 0; n < NumLineDefs; n++)
			{

				sd1 = LineDefs[n].side_R;
				sd2 = LineDefs[n].side_L;
				v1 = LineDefs[n].start;
				v2 = LineDefs[n].end;

				if ((sd1 >= 0 && SideDefs[sd1].sector == objnum)
						|| (sd2 >= 0 && SideDefs[sd2].sector == objnum))
				{

					/* if the Sector is closed, all Vertices will be counted twice */
					accx += (long) Vertices[v1].x;
					accy += (long) Vertices[v1].y;
					num++;
					accx += (long) Vertices[v2].x;
					accy += (long) Vertices[v2].y;
					num++;
				}
			}
			if (num > 0)
			{
				*xpos = (int) ((accx + num / 2L) / num);
				*ypos = (int) ((accy + num / 2L) / num);
			}
			else
			{
				*xpos = (MapMinX + MapMaxX) / 2;
				*ypos = (MapMinY + MapMaxY) / 2;
			}
			break;

		default:
			nf_bug ("GetObjectCoords: bad objtype %d", objtype);  // Can't happen
			*xpos = 0;
			*ypos = 0;
	}
}



/*
   find a free tag number
   */
int FindFreeTag ()
{
	int  tag, n;
	bool ok;

	tag = 1;
	ok = false;
	while (! ok)
	{
		ok = true;
		for (n = 0; n < NumLineDefs; n++)
			if (LineDefs[n].tag == tag)
			{
				ok = false;
				break;
			}
		if (ok)
			for (n = 0; n < NumSectors; n++)
				if (Sectors[n].tag == tag)
				{
					ok = false;
					break;
				}
		tag++;
	}
	return tag - 1;
}



/*
 *  focus_on_map_coords
 *  Change the view so that the map coordinates (xpos, ypos)
 *  appear under the pointer
 */
void focus_on_map_coords (int x, int y)
{
	grid.orig_x = x - ((edit.map_x) - grid.orig_x);
	grid.orig_y = y - ((edit.map_y) - grid.orig_y);
}


/*
 *  sector_under_pointer
 *  Convenience function
 */
inline int sector_under_pointer ()
{
	Objid o;
	GetCurObject (o, OBJ_SECTORS, edit.map_x, edit.map_y);
	return o.num;
}


/*
  centre the map around the object and zoom in if necessary
*/

void GoToObject (const Objid& objid)
{
	int   xpos, ypos;
	int   xpos2, ypos2;
	int   sd1, sd2;

	GetObjectCoords (objid.type, objid.num, &xpos, &ypos);
	focus_on_map_coords (xpos, ypos);

	/* Special case for sectors: if a sector contains other sectors,
	   or if its shape is such that it does not contain its own
	   geometric centre, zooming in on the centre won't help. So I
	   choose a linedef that borders the sector and focus on a point
	   between the centre of the linedef and the centre of the
	   sector. If that doesn't help, I try another linedef.

	   This algorithm is not perfect but it works rather well with
	   most well-constituted sectors. It does not work so well for
	   unclosed sectors, though (but it's partly GetCurObject()'s
	   fault). */
	if (objid.type == OBJ_SECTORS && sector_under_pointer () != objid.num)
	{
		for (int n = 0; n < NumLineDefs; n++)
		{

			sd1 = LineDefs[n].side_R;
			sd2 = LineDefs[n].side_L;

			if (sd1 >= 0 && SideDefs[sd1].sector == objid.num
					|| sd2 >= 0 && SideDefs[sd2].sector == objid.num)
			{
				GetObjectCoords (OBJ_LINEDEFS, n, &xpos2, &ypos2);
				int d = ComputeDist (abs (xpos - xpos2), abs (ypos - ypos2)) / 7;
				if (d <= 1)
					d = 2;
				xpos = xpos2 + (xpos - xpos2) / d;
				ypos = ypos2 + (ypos - ypos2) / d;
				focus_on_map_coords (xpos, ypos);
				if (sector_under_pointer () == objid.num)
					break;
			}
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab