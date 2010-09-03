/*BHEADER**********************************************************************

  Copyright (c) 1995-2009, Lawrence Livermore National Security,
  LLC. Produced at the Lawrence Livermore National Laboratory. Written
  by the Parflow Team (see the CONTRIBUTORS file)
  <parflow@lists.llnl.gov> CODE-OCEC-08-103. All rights reserved.

  This file is part of Parflow. For details, see
  http://www.llnl.gov/casc/parflow

  Please read the COPYRIGHT file or Our Notice and the LICENSE file
  for the GNU Lesser General Public License.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License (as published
  by the Free Software Foundation) version 2.1 dated February 1999.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
  and conditions of the GNU General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
**********************************************************************EHEADER*/

#include "parflow.h"

// #define SGS

#if 0
#define debug_print(arg)			\
   {						\
      int foo_x = 28;				\
      int foo_y = 83;				\
      int foo_z = 166;				\
      						\
      int foo_lower_x = foo_x - 8;		\
      int foo_lower_y = foo_y - 8;		\
      int foo_lower_z = foo_z - 8;		\
      						\
      int foo_upper_x = foo_x + 8;		\
      int foo_upper_y = foo_y + 8;		\
      int foo_upper_z = foo_z + 8;		\
      							\
      if( (i > foo_lower_x && i < foo_upper_x) &&	\
	  (j > foo_lower_y && j < foo_upper_y) &&	\
	  (k > foo_lower_z && k < foo_upper_z) )	\
      {							\
         arg;						\
      }							\
   };
#else

#define debug_print(arg)			\
   {							\
      if(amps_Rank()==73)				\
      {							\
         arg;						\
      }							\
   };

#endif



/******************************************************************************
 *
 * The functions in this file are for manipulating the Octree structure.
 *
 *****************************************************************************/

/*--------------------------------------------------------------------------
 * Check the octree for isolated cells
 * "Isolated" cells have a boundary patch on every side
 *--------------------------------------------------------------------------*/

int GrGeomCheckOctree(
   GrGeomOctree *grgeom_octree)
{
   int  i;
   int result = 0;

   if(grgeom_octree)
   {
      /* Check if all of my faces are set */
      result = 1;
      for(i = 0; i < 6; i++)
	 result &= GrGeomOctreeHasFace(grgeom_octree,i);
       
      /* Recurse into my children children */
      if (GrGeomOctreeChildren(grgeom_octree) != NULL)
	 for(i = 0; i < GrGeomOctreeNumChildren; i++)
	 {
	    result |= GrGeomCheckOctree(GrGeomOctreeChild(grgeom_octree,i));
	 }
   }

   return result;
}

/*--------------------------------------------------------------------------
 * Fix the octree by removing interior faces that may be present
 *--------------------------------------------------------------------------*/

void  GrGeomFixOctree(
   GrGeomOctree  *grgeom_octree,
   GrGeomOctree **patch_octrees,
   int            num_patches,
   int            level,
   int            num_indices)
{
   GrGeomOctree   *node, *neighbor_node, *patch_node;
   int            *fdir;
   int             i, j, k, p;
   int             face_index, new_level;

   i = 0;
   j = 0;
   k = 0;
   GrGeomOctreeFaceLoop(i, j, k, fdir, node, grgeom_octree, level,
                        0, 0, 0, num_indices, num_indices, num_indices,
   {
      face_index = GrGeomOctreeFaceIndex(fdir[0],fdir[1],fdir[2]);

      neighbor_node = GrGeomOctreeFind(&new_level, grgeom_octree,
				       i + fdir[0],
				       j + fdir[1],
				       k + fdir[2],
				       level);
      if (new_level == level)
      {
	 if (GrGeomOctreeCellIsInside(neighbor_node))
	 {
	    GrGeomOctreeClearFace(node, face_index);

	    for(p = 0; p < num_patches; p++)
	    {
	       patch_node = GrGeomOctreeFind(&new_level,
					     patch_octrees[p],
					     i, j, k, level);
	       if (new_level == level)
	       {
		  GrGeomOctreeClearFace(patch_node, face_index);
	       }
	    }
	 }
      }
   });
}

/*--------------------------------------------------------------------------
 * GrGeomNewOctree
 *--------------------------------------------------------------------------*/

GrGeomOctree *GrGeomNewOctree()
{
   GrGeomOctree *grgeom_octree;
   int           i;

   grgeom_octree = ctalloc(GrGeomOctree, 1);

   GrGeomOctreeSetCell(grgeom_octree, GrGeomOctreeCellEmpty);
   GrGeomOctreeSetCellLeaf(grgeom_octree);

   GrGeomOctreeParent(grgeom_octree) = NULL;

   GrGeomOctreeChildren(grgeom_octree) = NULL;

   for(i = 0; i < GrGeomOctreeNumFaces; i++)
   {
      GrGeomOctreeClearFace(grgeom_octree,i);
   }

   return grgeom_octree;
}


/*--------------------------------------------------------------------------
 * GrGeomNewOctreeChildren
 *--------------------------------------------------------------------------*/

void GrGeomNewOctreeChildren(
   GrGeomOctree *grgeom_octree)
{
   int i;

   if (GrGeomOctreeChildren(grgeom_octree) == NULL)
   {
      GrGeomOctreeChildren(grgeom_octree) = ctalloc(GrGeomOctree *, GrGeomOctreeNumChildren);

      for(i = 0; i < GrGeomOctreeNumChildren; i++)
      {
         GrGeomOctreeChild(grgeom_octree, i) = GrGeomNewOctree();
         GrGeomOctreeParent(GrGeomOctreeChild(grgeom_octree, i)) = grgeom_octree;
      }

      /* make into a non-leaf node */
      GrGeomOctreeClearCellLeaf(grgeom_octree);
   }
}


/*--------------------------------------------------------------------------
 * GrGeomFreeOctree:
 *   Frees the octree node passed in and everything below it.
 *--------------------------------------------------------------------------*/

void GrGeomFreeOctree(
   GrGeomOctree *grgeom_octree)
{
   int  ic;


   if  ( grgeom_octree != NULL )
   {
      if (GrGeomOctreeChildren(grgeom_octree) != NULL)
      {
         for (ic = 0; ic < GrGeomOctreeNumChildren; ic++)
         {
            GrGeomFreeOctree(GrGeomOctreeChild(grgeom_octree, ic));
         }
         tfree(GrGeomOctreeChildren(grgeom_octree));
      }
      tfree(grgeom_octree);
   }
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeFind
 *--------------------------------------------------------------------------*/

GrGeomOctree   *GrGeomOctreeFind(
int            *new_level,
GrGeomOctree   *grgeom_octree_root,
int             ix,
int             iy,
int             iz,
int             level)
{
   GrGeomOctree *new_node;
   unsigned int  xbits, ybits, zbits;
   unsigned int  mask;
   int           num_indices, octant, l;
   int           searching;

   num_indices = ((int) pow(2.0, level)) - 1;

   /* if ix, iy, iz is "out of range", return */
   if ((ix < 0) || (iy < 0) || (iz < 0) ||
       (ix > num_indices) || (iy > num_indices) || (iz > num_indices))
   {
      *new_level = -1;
      return NULL;
   }

   xbits = (unsigned int) ix;
   ybits = (unsigned int) iy;
   zbits = (unsigned int) iz;
   
   new_node = grgeom_octree_root;
   
   searching  = TRUE;
   l = level;
   while (searching)
   {
      /*----------------------------------------------------------
       * If this is the level of interest, stop searching
       *----------------------------------------------------------*/

      if (l == 0)
      {
	 searching  = FALSE;
      }

      /*----------------------------------------------------------
       * Else if this is a leaf node, stop searching
       *----------------------------------------------------------*/

      else if (GrGeomOctreeCellIsLeaf(new_node))
      {
	 searching = FALSE;
      }

      /*----------------------------------------------------------
       * Else, find the next child
       *----------------------------------------------------------*/

      else
      {
	 mask = ((unsigned int) 0x01) << (l - 1);
	 
	 if ((zbits & mask) != ((unsigned int) 0))
	 {
	    if ((ybits & mask) != ((unsigned int) 0))
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRUF;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLUF;
	       }
	    }
	    else
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRDF;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLDF;
	       }
	    }
	 }
	 else
	 {
	    if ((ybits & mask) != ((unsigned int) 0))
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRUB;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLUB;
	       }
	    }
	    else
	    {
	       if ((xbits & mask) != ((unsigned int)  0))
	       {
		  octant = GrGeomOctreeOctantRDB;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLDB;
	       }
	    }
	 }
	 
	 new_node = GrGeomOctreeChild(new_node,octant);
	 l--;
      }
   }

   *new_level = level - l;

   return new_node;
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeAddCell
 *--------------------------------------------------------------------------*/

GrGeomOctree   *GrGeomOctreeAddCell(
GrGeomOctree   *grgeom_octree_root,
unsigned int    cell,
int             ix,
int             iy,
int             iz,
int             level)
{
   GrGeomOctree *current_node;
   unsigned int  xbits, ybits, zbits;
   unsigned int  mask;
   int           octant, l;
   int           searching;

   xbits = (unsigned int) ix;
   ybits = (unsigned int) iy;
   zbits = (unsigned int) iz;
   
   current_node = grgeom_octree_root;
   
   searching  = TRUE;
   l = level;
   while (searching)
   {
      /*----------------------------------------------------------
       * If this is the level of interest, write the node
       *----------------------------------------------------------*/

      if (l == 0)
      {
	 /* don't overwrite "Inside" cells */
         if (! GrGeomOctreeCellIsInside(current_node) )
            GrGeomOctreeSetCell(current_node, cell);

	 searching  = FALSE;
      }

      /*----------------------------------------------------------
       * If searching, find the next child to visit (possibly
       * allocate new children)
       *----------------------------------------------------------*/

      if (searching)
      {
	 mask = ((unsigned int) 0x01) << (l - 1);
	 
	 if ((zbits & mask) != ((unsigned int) 0))
	 {
	    if ((ybits & mask) != ((unsigned int) 0))
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRUF;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLUF;
	       }
	    }
	    else
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRDF;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLDF;
	       }
	    }
	 }
	 else
	 {
	    if ((ybits & mask) != ((unsigned int) 0))
	    {
	       if ((xbits & mask) != ((unsigned int) 0))
	       {
		  octant = GrGeomOctreeOctantRUB;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLUB;
	       }
	    }
	    else
	    {
	       if ((xbits & mask) != ((unsigned int)  0))
	       {
		  octant = GrGeomOctreeOctantRDB;
	       }
	       else
	       {
		  octant = GrGeomOctreeOctantLDB;
	       }
	    }
	 }
	 
	 if (GrGeomOctreeChildren(current_node) == NULL)
	 {
	    GrGeomNewOctreeChildren(current_node);
	 }

	 current_node = GrGeomOctreeChild(current_node,octant);
	 l--;
      }
   }

   return current_node;
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeAddFace
 *--------------------------------------------------------------------------*/

GrGeomOctree   *GrGeomOctreeAddFace(
GrGeomOctree   *grgeom_octree_root,
int             line_direction,
int             cell_index0,
int             cell_index1,
int             face_index,
int             extent_lower,
int             extent_upper,
int             level,
int             normal_in_direction)
{
   GrGeomOctree *current_node=NULL;
   int           ix = 0, iy = 0 , iz = 0, cell_index;
   unsigned int  cell = 0, face = 0;


   if ( line_direction == XDIRECTION )
   {
      iy = cell_index0;
      iz = cell_index1;
   }
   else if ( line_direction == YDIRECTION )
   {
      ix = cell_index0;
      iz = cell_index1;
   }
   else if ( line_direction == ZDIRECTION )
   {
      ix = cell_index0;
      iy = cell_index1;
   } 

   /* Add (if necessary) the lower cell that has this face */
   cell_index = face_index - 1;
   if ((cell_index >= extent_lower) && (cell_index <= extent_upper))
   {
      if ( line_direction == XDIRECTION )
      {
         ix = cell_index;
	 face = GrGeomOctreeFaceR;
      }
      else if ( line_direction == YDIRECTION )
      {
         iy = cell_index;
	 face = GrGeomOctreeFaceU;
      }
      else if ( line_direction == ZDIRECTION )
      {
         iz = cell_index;
	 face = GrGeomOctreeFaceF;
      }

      if (normal_in_direction < 0)
         cell = GrGeomOctreeCellOutside;
      else if (normal_in_direction > 0)
         cell = GrGeomOctreeCellInside;

      current_node = GrGeomOctreeAddCell(grgeom_octree_root,
                                         cell, ix, iy, iz, level);

      GrGeomOctreeSetFace(current_node, face);
   }

   /* Add (if necessary) the upper cell that has this face */
   cell_index = face_index;
   if ((cell_index >= extent_lower) && (cell_index <= extent_upper))
   {
      if ( line_direction == XDIRECTION )
      {
         ix = cell_index;
	 face = GrGeomOctreeFaceL;
      }
      else if ( line_direction == YDIRECTION )
      {
         iy = cell_index;
	 face = GrGeomOctreeFaceD;
      }
      else if ( line_direction == ZDIRECTION )
      {
         iz = cell_index;
	 face = GrGeomOctreeFaceB;
      }

      if (normal_in_direction < 0)
         cell = GrGeomOctreeCellInside;
      else if (normal_in_direction > 0)
         cell = GrGeomOctreeCellOutside;

      current_node = GrGeomOctreeAddCell(grgeom_octree_root,
                                         cell, ix, iy, iz, level);

      GrGeomOctreeSetFace(current_node, face);
   }

   return current_node;
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeFromTIN
 *--------------------------------------------------------------------------*/

void GrGeomOctreeFromTIN(
GrGeomOctree       **solid_octree_ptr,
GrGeomOctree      ***patch_octrees_ptr,
GeomTIN             *solid,
int                **patches,
int                  num_patches,
int                 *num_patch_triangles,
GrGeomExtentArray   *extent_array,
double               xlower,
double               ylower,
double               zlower,
double               xupper,
double               yupper,
double               zupper,
int                  min_level,
int                  max_level)
{
   GrGeomOctree  *solid_octree;
   GrGeomOctree **patch_octrees;

   GeomTriangle  *triangle;
   GeomVertex    *vertex;
   ListMember  ***xy_lines, ***xz_lines, ***yz_lines;
   ListMember    *current_member, *tmp_member;
   GrGeomOctree  *grgeom_octree, *grgeom_child, *patch_octree;
   GrGeomExtents *ea_extents;

   int           *ixl_lines, *iyl_lines, *izl_lines;
   int           *ixu_lines, *iyu_lines, *izu_lines;
   int           *nx_lines, *ny_lines, *nz_lines;
   int            num_indices, num_triangles;
   int            ea_size;
   double         dx_lines, dy_lines, dz_lines;

   double         v0_x, v0_y, v0_z;
   double         v1_x, v1_y, v1_z;
   double         v2_x, v2_y, v2_z;
   double         octree_v0_x, octree_v0_y, octree_v0_z;
   double         octree_v1_x, octree_v1_y, octree_v1_z;
   double         octree_v2_x, octree_v2_y, octree_v2_z;
   double	  tbox_x_lower, tbox_x_upper;
   double	  tbox_y_lower, tbox_y_upper;
   double	  tbox_z_lower, tbox_z_upper;

   int            nx, ny, nz;
   // SGS there is an error here dz, nz are not being initialized correctly in second loop
   double         dx, dy, dz;
   int            inc, tmp_inc;
   int            ix_lower, iy_lower, iz_lower;
   int            ix_upper,   iy_upper,   iz_upper;
   int            index;
   int            intersects, component, state, start_state;

   double         x_lower,  y_lower,  z_lower;
   double         x_center, y_center, z_center;
   double         x_upper,  y_upper,  z_upper;
   double         point;

   int            p, i, j, k, ie, m, n, level, new_level;
   int            iprime, jprime, kprime, ic, t, face_index = 0;

   int          **patch_table, *patch_table_len;
   int           *patch_list, *tmp_patch_list;
   int            patch_list_blocksz = 5;

   int           *edge_tag, *triangle_tag;
   unsigned char *interior_tag;

   /*-------------------------------------------------------------
    * Make sure extent_array is inside the octree index space
    *-------------------------------------------------------------*/

   num_indices = Pow2(max_level);

   ea_extents = GrGeomExtentArrayExtents(extent_array);
   ea_size    = GrGeomExtentArraySize(extent_array);

   for(ie = 0; ie < ea_size; ie++)
   {
      GrGeomExtentsIXLower(ea_extents[ie]) =
	 pfmax(GrGeomExtentsIXLower(ea_extents[ie]), 0);
      GrGeomExtentsIYLower(ea_extents[ie]) =
	 pfmax(GrGeomExtentsIYLower(ea_extents[ie]), 0);
      GrGeomExtentsIZLower(ea_extents[ie]) =
	 pfmax(GrGeomExtentsIZLower(ea_extents[ie]), 0);
      GrGeomExtentsIXUpper(ea_extents[ie]) =
	 pfmin(GrGeomExtentsIXUpper(ea_extents[ie]), (num_indices-1));
      GrGeomExtentsIYUpper(ea_extents[ie]) =
	 pfmin(GrGeomExtentsIYUpper(ea_extents[ie]), (num_indices-1));
      GrGeomExtentsIZUpper(ea_extents[ie]) =
	 pfmin(GrGeomExtentsIZUpper(ea_extents[ie]), (num_indices-1));
   }

   /*-------------------------------------------------------------
    * Initialize xyz_lines arrays
    *-------------------------------------------------------------*/

   dx_lines = (xupper - xlower) / pow(2.0, ((double) (max_level + 1)));
   dy_lines = (yupper - ylower) / pow(2.0, ((double) (max_level + 1)));
   dz_lines = (zupper - zlower) / pow(2.0, ((double) (max_level + 1)));

   ixl_lines = talloc(int, ea_size);
   iyl_lines = talloc(int, ea_size);
   izl_lines = talloc(int, ea_size);

   ixu_lines = talloc(int, ea_size);
   iyu_lines = talloc(int, ea_size);
   izu_lines = talloc(int, ea_size);

   nx_lines = talloc(int, ea_size);
   ny_lines = talloc(int, ea_size);
   nz_lines = talloc(int, ea_size);

   xy_lines = talloc(ListMember **, ea_size);
   xz_lines = talloc(ListMember **, ea_size);
   yz_lines = talloc(ListMember **, ea_size);
      
   for(ie = 0; ie < ea_size; ie++)
   {
      ixl_lines[ie] = 2*GrGeomExtentsIXLower(ea_extents[ie]) + 1;
      iyl_lines[ie] = 2*GrGeomExtentsIYLower(ea_extents[ie]) + 1;
      izl_lines[ie] = 2*GrGeomExtentsIZLower(ea_extents[ie]) + 1;

      ixu_lines[ie] = 2*GrGeomExtentsIXUpper(ea_extents[ie]) + 1;
      iyu_lines[ie] = 2*GrGeomExtentsIYUpper(ea_extents[ie]) + 1;
      izu_lines[ie] = 2*GrGeomExtentsIZUpper(ea_extents[ie]) + 1;

      nx_lines[ie] = ixu_lines[ie] - ixl_lines[ie] + 1;
      ny_lines[ie] = iyu_lines[ie] - iyl_lines[ie] + 1;
      nz_lines[ie] = izu_lines[ie] - izl_lines[ie] + 1;

      xy_lines[ie] = talloc(ListMember *, nx_lines[ie] * ny_lines[ie]);
      xz_lines[ie] = talloc(ListMember *, nx_lines[ie] * nz_lines[ie]);
      yz_lines[ie] = talloc(ListMember *, ny_lines[ie] * nz_lines[ie]);
      
      index = 0;
      for(j = 0; j < ny_lines[ie]; j++)
      {
	 for(i = 0; i < nx_lines[ie]; i++)
	 {
	    xy_lines[ie][index] = NULL;
	    index++;
	 }
      }
      index = 0;
      for(k = 0; k < nz_lines[ie]; k++)
      {
	 for(i = 0; i < nx_lines[ie]; i++)
	 {
	    xz_lines[ie][index] = NULL;
	    index++;
	 }
      }
      index = 0;
      for(k = 0; k < nz_lines[ie]; k++)
      {
	 for(j = 0; j < ny_lines[ie]; j++)
	 {
	    yz_lines[ie][index] = NULL;
	    index++;
	 }
      }
   }

   /*-------------------------------------------------------------
    * Set up the triangle-to-patch mapping (patch_table)
    *-------------------------------------------------------------*/

   num_triangles = GeomTINNumTriangles(solid);

   patch_table_len = talloc(int,   num_triangles);
   patch_table     = talloc(int *, num_triangles);

   for(n = 0; n < num_triangles; n++)
   {
      patch_table_len[n] = 0;
      patch_table[n] = NULL;
   }

   for(p = 0; p < num_patches; p++)
   {
      for(n = 0; n < num_patch_triangles[p]; n++)
      {
         t = patches[p][n];
	 patch_list = patch_table[t];
	 if (!(patch_table_len[t] % patch_list_blocksz))
	 {
	    tmp_patch_list = patch_list;
	    patch_list = ctalloc(int, patch_table_len[t] + patch_list_blocksz);
	    for (m = 0; m < patch_table_len[t]; m++)
	       patch_list[m] = tmp_patch_list[m];
	    tfree(tmp_patch_list);
	 }
	 patch_list[patch_table_len[t]] = p;
	 patch_table[t] = patch_list;
         patch_table_len[t]++;
      }
   }

   /*-------------------------------------------------------------
    * Find all unique line intersections
    *-------------------------------------------------------------*/

   for(n = 0; n < num_triangles; n++)
   {
      /* Get the triangle */
      triangle = GeomTINTriangle(solid, n);

      /* Get bounds on the cover for the coordinates of vertex 0 */
      vertex = GeomTINVertex(solid, GeomTriangleV0(triangle));
      v0_x = GeomVertexX(vertex);
      v0_y = GeomVertexY(vertex);
      v0_z = GeomVertexZ(vertex);
      octree_v0_x = (v0_x - xlower) / dx_lines;
      octree_v0_y = (v0_y - ylower) / dy_lines;
      octree_v0_z = (v0_z - zlower) / dz_lines;

      /* Get bounds on the cover for the coordinates of vertex 1 */
      vertex = GeomTINVertex(solid, GeomTriangleV1(triangle));
      v1_x = GeomVertexX(vertex);
      v1_y = GeomVertexY(vertex);
      v1_z = GeomVertexZ(vertex);
      octree_v1_x = (v1_x - xlower) / dx_lines;
      octree_v1_y = (v1_y - ylower) / dy_lines;
      octree_v1_z = (v1_z - zlower) / dz_lines;

      /* Get bounds on the cover for the coordinates of vertex 2 */
      vertex = GeomTINVertex(solid, GeomTriangleV2(triangle));
      v2_x = GeomVertexX(vertex);
      v2_y = GeomVertexY(vertex);
      v2_z = GeomVertexZ(vertex);
      octree_v2_x = (v2_x - xlower) / dx_lines;
      octree_v2_y = (v2_y - ylower) / dy_lines;
      octree_v2_z = (v2_z - zlower) / dz_lines;

      /* Get overall bounds for the vertices */
      tbox_x_lower = pfmin(pfmin(octree_v0_x, octree_v1_x), octree_v2_x);
      tbox_y_lower = pfmin(pfmin(octree_v0_y, octree_v1_y), octree_v2_y);
      tbox_z_lower = pfmin(pfmin(octree_v0_z, octree_v1_z), octree_v2_z);
      tbox_x_upper = pfmax(pfmax(octree_v0_x, octree_v1_x), octree_v2_x);
      tbox_y_upper = pfmax(pfmax(octree_v0_y, octree_v1_y), octree_v2_y);
      tbox_z_upper = pfmax(pfmax(octree_v0_z, octree_v1_z), octree_v2_z);

      /* This is to avoid integer overflow below */
      tbox_x_lower = pfmin(tbox_x_lower, 2*num_indices);
      tbox_y_lower = pfmin(tbox_y_lower, 2*num_indices);
      tbox_z_lower = pfmin(tbox_z_lower, 2*num_indices);
      tbox_x_upper = pfmax(tbox_x_upper, -1);
      tbox_y_upper = pfmax(tbox_y_upper, -1);
      tbox_z_upper = pfmax(tbox_z_upper, -1);

      for(ie = 0; ie < ea_size; ie++)
      {
	 /* Intersect bounds with xyz_lines extents */
	 ix_lower = (int)floor((double) pfmax(tbox_x_lower, ixl_lines[ie]));
	 iy_lower = (int)floor((double) pfmax(tbox_y_lower, iyl_lines[ie]));
	 iz_lower = (int)floor((double) pfmax(tbox_z_lower, izl_lines[ie]));
	 ix_upper = (int)ceil((double) pfmin(tbox_x_upper, ixu_lines[ie]));
	 iy_upper = (int)ceil((double) pfmin(tbox_y_upper, iyu_lines[ie]));
	 iz_upper = (int)ceil((double) pfmin(tbox_z_upper, izu_lines[ie]));

	 for(j = iy_lower; j <= iy_upper; j++)
	 {
	    for(i = ix_lower; i <= ix_upper; i++)
	    {
	       x_center = xlower + i * dx_lines;
	       y_center = ylower + j * dy_lines;
	       IntersectLineWithTriangle(ZDIRECTION, x_center, y_center,
					 v0_x, v0_y, v0_z,
					 v1_x, v1_y, v1_z,
					 v2_x, v2_y, v2_z,
					 &intersects, &point, &component);
	       if (intersects)
	       {
		  iprime = i - ixl_lines[ie];
		  jprime = j - iyl_lines[ie];
		  index = iprime + jprime * nx_lines[ie];
		  tmp_member = ListValueNormalComponentSearch(xy_lines[ie][index], point, component);
		  if (tmp_member == NULL)
		  {
		     current_member = NewListMember(point, component, n);
		     ListInsert(&xy_lines[ie][index], current_member);
		  }
	       }
	    }
	 }
	 
	 for(k = iz_lower; k <= iz_upper; k++)
	 {
	    for(i = ix_lower; i <= ix_upper; i++)
	    {
	       x_center = xlower + i * dx_lines;
	       z_center = zlower + k * dz_lines;
	       IntersectLineWithTriangle(YDIRECTION, x_center, z_center,
					 v0_x, v0_y, v0_z,
					 v1_x, v1_y, v1_z,
					 v2_x, v2_y, v2_z,
					 &intersects, &point, &component);
	       if (intersects)
	       {
		  iprime = i - ixl_lines[ie];
		  kprime = k - izl_lines[ie];
		  index = iprime + kprime * nx_lines[ie];
		  tmp_member = ListValueNormalComponentSearch(xz_lines[ie][index], point, component);
		  if (tmp_member == NULL)
		  {
		     current_member = NewListMember(point, component, n);
		     ListInsert(&xz_lines[ie][index], current_member);
		  }
	       }
	    }
	 }
	 
	 for(k = iz_lower; k <= iz_upper; k++)
	 {
	    for(j = iy_lower; j <= iy_upper; j++)
	    {
	       y_center = ylower + j * dy_lines;
	       z_center = zlower + k * dz_lines;
	       IntersectLineWithTriangle(XDIRECTION, y_center, z_center,
					 v0_x, v0_y, v0_z,
					 v1_x, v1_y, v1_z,
					 v2_x, v2_y, v2_z,
					 &intersects, &point, &component);
	       if (intersects)
	       {
		  jprime = j - iyl_lines[ie];
		  kprime = k - izl_lines[ie];
		  index = jprime + kprime * ny_lines[ie];
		  tmp_member = ListValueNormalComponentSearch(yz_lines[ie][index], point, component);
		  if (tmp_member == NULL)
		  {
		     current_member = NewListMember(point, component, n);
		     ListInsert(&yz_lines[ie][index], current_member);
		  }
	       }
	    }
	 }
      }
   }

   /*-------------------------------------------------------------
    * Create the octree, by first adding the boundary faces
    *-------------------------------------------------------------*/

   solid_octree =  GrGeomNewOctree();
   patch_octrees = ctalloc(GrGeomOctree *, num_patches);
   for (p = 0; p < num_patches; p++)
      patch_octrees[p] = GrGeomNewOctree();

   for(ie = 0; ie < ea_size; ie++)
   {
      for(level = min_level; level <= max_level; level++)
      {
         nx = Pow2(level);
         ny = Pow2(level);
         nz = Pow2(level);

         dx = (xupper - xlower) / nx;
         dy = (yupper - ylower) / ny;
         dz = (zupper - zlower) / nz;

         inc = Pow2(max_level - level);

         ix_lower = GrGeomExtentsIXLower(ea_extents[ie]) / inc;
         ix_upper = GrGeomExtentsIXUpper(ea_extents[ie]) / inc;
         iy_lower = GrGeomExtentsIYLower(ea_extents[ie]) / inc;
         iy_upper = GrGeomExtentsIYUpper(ea_extents[ie]) / inc;
         iz_lower = GrGeomExtentsIZLower(ea_extents[ie]) / inc;
         iz_upper = GrGeomExtentsIZUpper(ea_extents[ie]) / inc;

	 /*-------------------------------------------------------
	  * Add Inside and Outside nodes in z
	  *-------------------------------------------------------*/

         edge_tag     = ctalloc(int, (nz + 1));
         triangle_tag = ctalloc(int, (nz + 1));
         for(j = iy_lower; j <= iy_upper; j++)
         {
            for(i = ix_lower; i <= ix_upper; i++)
            {
               iprime = (1 + 2*i) * inc - ixl_lines[ie];
               jprime = (1 + 2*j) * inc - iyl_lines[ie];
               index = iprime + jprime * nx_lines[ie];

               /* Find initial orientation */
               current_member = xy_lines[ie][index];
               if (current_member != NULL)
               {
                  component = ListMemberNormalComponent(current_member);
               }

               while( ListValueLTPoint(current_member, zlower) )
               {
                  current_member = ListMemberNextListMember(current_member);
                  if (current_member != NULL)
                  {
                     component = ListMemberNormalComponent(current_member);
                  }
               }

               /* Fill in the edge crossing info */
               edge_tag[0] = 0;
               triangle_tag[0] = 0;
               for(k = 0; k < nz;  k++)
               {
                  z_lower  = zlower +     ((double) k)     * dz;
                  z_center = zlower + (((double) k) + 0.5) * dz;
                  z_upper  = zlower + (((double) k) + 1.0) * dz;
                  edge_tag[k+1] = 0;
                  triangle_tag[k+1] = 0;

                  if ( ListValueLEPoint(current_member, z_upper) )
                  {
                     while( ListValueInClosedInterval(current_member,
						      z_lower, z_upper) )
                     {
                        if ( ListValueGTPoint(current_member, z_center) )
                        {
			   face_index = k+1;
                        }
                        else if ( ListValueLTPoint(current_member, z_center) )
                        {
			   face_index = k;
                        }
                        else
                        {
                           if (component == 1)
                           {
			      face_index = k+1;
                           }
                           else if (component == -1)
                           {
			      face_index = k;
                           }
                        }
			edge_tag[face_index] += component;

			/* Want nonzero edge_tag with same sign as component */
			if ((edge_tag[face_index]*component) > 0)
			{
			   triangle_tag[face_index] =
			      ListMemberTriangleID(current_member);
			}
			else
			{
			   triangle_tag[face_index] = 0;
			}
                        current_member = ListMemberNextListMember(current_member);
                        if (current_member != NULL)
                        {
                           component = ListMemberNormalComponent(current_member);
                        }
                     }
                  }
               }

               /* Add faces using the edge crossing info */
               for(k = 0; k < (nz + 1); k++)
               {
                  if (edge_tag[k] != 0)
                  {
                     GrGeomOctreeAddFace(solid_octree, ZDIRECTION,
                                         i, j, k, iz_lower, iz_upper, level,
                                         edge_tag[k]);

                     t = triangle_tag[k];
                     for(p = 0; p < patch_table_len[t]; p++)
                     {
			patch_octree = patch_octrees[patch_table[t][p]];
                        GrGeomOctreeAddFace(patch_octree, ZDIRECTION,
                                            i, j, k, iz_lower, iz_upper, level,
                                            edge_tag[k]);
                     }
                  }
               }
            }
         }
         tfree(triangle_tag);
         tfree(edge_tag);

	 /*-------------------------------------------------------
	  * Add Inside and Outside nodes in y
	  *-------------------------------------------------------*/

         edge_tag     = ctalloc(int, (ny + 1));
         triangle_tag = ctalloc(int, (ny + 1));
         for(k = iz_lower; k <= iz_upper; k++)
         {
            for(i = ix_lower; i <= ix_upper; i++)
            {
               iprime = (1 + 2*i) * inc - ixl_lines[ie];
               kprime = (1 + 2*k) * inc - izl_lines[ie];
               index = iprime + kprime * nx_lines[ie];

               /* Find initial orientation */
               current_member = xz_lines[ie][index];
               if (current_member != NULL)
               {
                  component = ListMemberNormalComponent(current_member);
               }
               while( ListValueLTPoint(current_member, ylower) )
               {
                  current_member = ListMemberNextListMember(current_member);
                  if (current_member != NULL)
                  {
                     component = ListMemberNormalComponent(current_member);
                  }
               }

               /* Fill in the edge crossing info */
               edge_tag[0] = 0;
               triangle_tag[0] = 0;
               for(j = 0; j < ny; j++)
               {
                  y_lower  = ylower +     ((double) j)     * dy;
                  y_center = ylower + (((double) j) + 0.5) * dy;
                  y_upper  = ylower + (((double) j) + 1.0) * dy;

                  edge_tag[j+1] = 0;
                  triangle_tag[j+1] = 0;
                  if ( ListValueLEPoint(current_member, y_upper) )
                  {
                     while( ListValueInClosedInterval(current_member,
						      y_lower, y_upper) )
                     {
                        if ( ListValueGTPoint(current_member, y_center) )
                        {
			   face_index = j+1;
                        }
                        else if ( ListValueLTPoint(current_member, y_center) )
                        {
			   face_index = j;
                        }
                        else
                        {
                           if (component == 1)
                           {
			      face_index = j+1;
                           }
                           else if (component == -1)
                           {
			      face_index = j;
                           }
                        }
			edge_tag[face_index] += component;

			/* Want nonzero edge_tag with same sign as component */
			if ((edge_tag[face_index]*component) > 0)
			{
			   triangle_tag[face_index] = ListMemberTriangleID(current_member);
			}
			else
			{
			   triangle_tag[face_index] = 0;
			}
                        current_member = ListMemberNextListMember(current_member);
                        if (current_member != NULL)
                        {
                           component = ListMemberNormalComponent(current_member);
                        }
                     }
                  }
               }

               /* Add faces using the edge crossing info */
               for(j = 0; j < (ny + 1); j++)
               {
                  if (edge_tag[j] != 0)
                  {
                     GrGeomOctreeAddFace(solid_octree, YDIRECTION,
                                         i, k, j, iy_lower, iy_upper, level,
                                         edge_tag[j]);

                     t = triangle_tag[j];
                     for(p = 0; p < patch_table_len[t]; p++)
                     {
			patch_octree = patch_octrees[patch_table[t][p]];
                        GrGeomOctreeAddFace(patch_octree, YDIRECTION,
                                            i, k, j, iy_lower, iy_upper, level,
                                            edge_tag[j]);
                     }
                  }
               }
            }
         }
         tfree(triangle_tag);
         tfree(edge_tag);

	 /*-------------------------------------------------------
	  * Add Inside and Outside nodes in x
	  *-------------------------------------------------------*/

         edge_tag     = ctalloc(int, (nx + 1));
         triangle_tag = ctalloc(int, (nx + 1));
         for(k = iz_lower; k <= iz_upper; k++)
         {
            for(j = iy_lower; j <= iy_upper; j++)
            {
               jprime = (1 + 2*j) * inc - iyl_lines[ie];
               kprime = (1 + 2*k) * inc - izl_lines[ie];
               index = jprime + kprime * ny_lines[ie];

               /* Find initial orientation */
               current_member = yz_lines[ie][index];
               if (current_member != NULL)
               {
                  component = ListMemberNormalComponent(current_member);
               }
               while( ListValueLTPoint(current_member, xlower) )
               {
                  current_member = ListMemberNextListMember(current_member);
                  if (current_member != NULL)
                  {
                     component = ListMemberNormalComponent(current_member);
                  }
               }

               /* Fill in the edge crossing info */
               edge_tag[0] = 0;
               triangle_tag[0] = 0;
               for(i = 0; i < nx; i++)
               {
                  x_lower  = xlower +     ((double) i)     * dx;
                  x_center = xlower + (((double) i) + 0.5) * dx;
                  x_upper  = xlower + (((double) i) + 1.0) * dx;

                  edge_tag[i+1] = 0;
                  triangle_tag[i+1] = 0;
                  if ( ListValueLEPoint(current_member, x_upper) )
                  {
                     while( ListValueInClosedInterval(current_member,
						      x_lower, x_upper) )
                     {
                        if ( ListValueGTPoint(current_member, x_center) )
                        {
			   face_index = i+1;
                        }
                        else if ( ListValueLTPoint(current_member, x_center) )
                        {
			   face_index = i;
                        }
                        else
                        {
                           if (component == 1)
                           {
			      face_index = i+1;
                           }
                           else if (component == -1)
                           {
			      face_index = i;
                           }
                        }
			edge_tag[face_index] += component;

			/* Want nonzero edge_tag with same sign as component */
			if ((edge_tag[face_index]*component) > 0)
			{
			   triangle_tag[face_index] = ListMemberTriangleID(current_member);
			}
			else
			{
			   triangle_tag[face_index] = 0;
			}
                        current_member = ListMemberNextListMember(current_member);
                        if (current_member != NULL)
                        {
                           component = ListMemberNormalComponent(current_member);
                        }
                     }
                  }
               }

               /* Add faces using the edge crossing info */
               for(i = 0; i < (nx + 1); i++)
               {
                  if (edge_tag[i] != 0)
                  {
                     GrGeomOctreeAddFace(solid_octree, XDIRECTION,
                                         j, k, i, ix_lower, ix_upper, level,
                                         edge_tag[i]);

                     t = triangle_tag[i];
                     for(p = 0; p < patch_table_len[t]; p++)
                     {
			patch_octree = patch_octrees[patch_table[t][p]];
                        GrGeomOctreeAddFace(patch_octree, XDIRECTION,
                                            j, k, i, ix_lower, ix_upper, level,
                                            edge_tag[i]);
                     }
                  }
               }
            }
         }
         tfree(triangle_tag);
         tfree(edge_tag);
      }
   }

   /*-------------------------------------------------------------
    * Fix up any stray edges that are interior
    *-------------------------------------------------------------*/
   for(level = min_level; level <= max_level; level++)
   {
      GrGeomFixOctree(solid_octree, patch_octrees, num_patches, level, num_indices);
   }

   /*-------------------------------------------------------------
    * Continue creating the octree, by adding the interior cells.
    *-------------------------------------------------------------*/

   for(ie = 0; ie < ea_size; ie++)
   {
      for(level = min_level; level <= max_level; level++)
      {
         nz = Pow2(level);
         dz = (zupper - zlower) / nz;

         inc = Pow2(max_level - level);

         ix_lower = GrGeomExtentsIXLower(ea_extents[ie]) / inc;
         ix_upper = GrGeomExtentsIXUpper(ea_extents[ie]) / inc;
         iy_lower = GrGeomExtentsIYLower(ea_extents[ie]) / inc;
         iy_upper = GrGeomExtentsIYUpper(ea_extents[ie]) / inc;
         iz_lower = GrGeomExtentsIZLower(ea_extents[ie]) / inc;
         iz_upper = GrGeomExtentsIZUpper(ea_extents[ie]) / inc;

        /*-------------------------------------------------------------
         * Add `Fill' nodes to solid_octree (in z)
         *-------------------------------------------------------------*/

         interior_tag = ctalloc(unsigned char, nz);
         edge_tag     = ctalloc(int, (nz + 1));
         for(j = iy_lower; j <= iy_upper; j++)
         {
            for(i = ix_lower; i <= ix_upper; i++)
            {
               iprime = (1 + 2*i) * inc - ixl_lines[ie];
               jprime = (1 + 2*j) * inc - iyl_lines[ie];
               index = iprime + jprime * nx_lines[ie];

               current_member = xy_lines[ie][index];
               if (current_member != NULL)
               {
                  component = ListMemberNormalComponent(current_member);
               }

               z_lower  = zlower + ((double) iz_lower) * dz;

               start_state = OUTSIDE;
               while( ListValueLTPoint(current_member, z_lower) )
               {
                  if ((start_state == OUTSIDE) && (component < 0))
                  {
                     start_state = INSIDE;
                  }
                  if ((start_state == INSIDE) && (component > 0))
                  {
                     start_state = OUTSIDE;
                  }

                  current_member = ListMemberNextListMember(current_member);
                  if (current_member != NULL)
                  {
                     component = ListMemberNormalComponent(current_member);
                  }
               }


               /* Fill in the edge crossing info */
               edge_tag[iz_lower] = 0;
               for(k = iz_lower; k <= iz_upper;  k++)
               {
                  z_lower  = zlower +     ((double) k)     * dz;
                  z_center = zlower + (((double) k) + 0.5) * dz;
                  z_upper  = zlower + (((double) k) + 1.0) * dz;
                  edge_tag[k+1] = 0;

                  if ( ListValueLEPoint(current_member, z_upper) )
                  {
                     while( ListValueInClosedInterval(current_member,
						      z_lower, z_upper) )
                     {
                        if ( ListValueGTPoint(current_member, z_center) )
                        {
			   face_index = k+1;
                        }
                        else if ( ListValueLTPoint(current_member, z_center) )
                        {
			   face_index = k;
                        }
                        else
                        {
                           if (component == 1)
                           {
			      face_index = k+1;
                           }
                           else if (component == -1)
                           {
			      face_index = k;
                           }
                        }
			edge_tag[face_index] += component;
                        current_member = ListMemberNextListMember(current_member);
                        if (current_member != NULL)
                        {
                           component = ListMemberNormalComponent(current_member);
                        }
                     }
                  }
               }

               /* Fix up the edge crossing info */
               for(k = iz_lower; k <= iz_upper;  k++)
               {
                 if (edge_tag[k] != 0)
                 {
                    grgeom_octree = GrGeomOctreeFind(&new_level, solid_octree, i, j, k, level);
                    if (GrGeomOctreeHasNoFace(grgeom_octree,GrGeomOctreeFaceB))
                    {
                       edge_tag[k] = 0;
                    }
                 }
               }
               if (edge_tag[iz_upper+1] != 0)
               {
                 grgeom_octree = GrGeomOctreeFind(&new_level, solid_octree, i, j, iz_upper, level);
                 if (GrGeomOctreeHasNoFace(grgeom_octree,GrGeomOctreeFaceF))
                 {
                    edge_tag[iz_upper+1] = 0;
                 }
               }

               /* Fill in the interior info using the edge crossing info */
               state = start_state;
               for(k = iz_lower; k <= iz_upper;  k++)
               {
                  interior_tag[k] = 0;
                  if (state == INSIDE)
                  {
                     if (edge_tag[k] != 0)
                     {
                        state = OUTSIDE;
                     }
                     else if (edge_tag[k+1] == 0)
                     {
                        interior_tag[k] = GrGeomOctreeCellFull;
                     }
                  }
                  else
                  {
                     if (edge_tag[k] != 0)
                     {
                        state = INSIDE;
                     }
                  }
               }

               /* Add `Full' nodes to the octree */
               k = iz_lower;
               while (k <= iz_upper)
               {
                  if (interior_tag[k])
                  {
                     /* try to find the (i, j, k, level) octree node */
                     grgeom_octree = GrGeomOctreeFind(&new_level, solid_octree, i, j, k, level);

                     /* Only change non-`Full' cells */
                     if (!GrGeomOctreeCellIsFull(grgeom_octree))
                     {
                        if ((new_level == level) || (new_level < min_level))
                        {
                           /* Only change `Empty' cells */
                           if (GrGeomOctreeCellIsEmpty(grgeom_octree))
                           {
                               GrGeomOctreeSetCell(grgeom_octree, GrGeomOctreeCellFull);
                           }
                        }
                        else
                        {
                           /* Create children and set */
                           GrGeomNewOctreeChildren(grgeom_octree);
                           for (ic = 0; ic < GrGeomOctreeNumChildren; ic++)
                           {
                              grgeom_child = GrGeomOctreeChild(grgeom_octree, ic);
                              GrGeomOctreeSetCell(grgeom_child, GrGeomOctreeCellFull);
                           }
			}
		     }

                     /* compute the new k index */
		     tmp_inc = Pow2(level - new_level);
		     k = (((int) (k / tmp_inc)) + 1) * tmp_inc;
                  }
                  else
                  {
                     k++;
                  }
               }
            }
         }
         tfree(edge_tag);
         tfree(interior_tag);
      }
   }

   /*-------------------------------------------------------------
    * Check the octree for isolated cells 
    *-------------------------------------------------------------*/

   if(GrGeomCheckOctree(solid_octree))
   {
      amps_Printf("Warning: isolated cell exists, need to fix the input.\n");
   }

   /*-------------------------------------------------------------
    * Free up stuff
    *-------------------------------------------------------------*/

   for(ie = 0; ie < ea_size; ie++)
   {
      index = 0;
      for(j = 0; j < ny_lines[ie]; j++)
      {
	 for(i = 0; i < nx_lines[ie]; i++)
	 {
	    ListFree(&xy_lines[ie][index]);
	    index++;
	 }
      }
      
      index = 0;
      for(k = 0; k < nz_lines[ie]; k++)
      {
	 for(i = 0; i < nx_lines[ie]; i++)
	 {
	    ListFree(&xz_lines[ie][index]);
	    index++;
	 }
      }
      
      index = 0;
      for(k = 0; k < nz_lines[ie]; k++)
      {
	 for(j = 0; j < ny_lines[ie]; j++)
	 {
	    ListFree(&yz_lines[ie][index]);
	    index++;
	 }
      }
      
      tfree(yz_lines[ie]);
      tfree(xz_lines[ie]);
      tfree(xy_lines[ie]);
   }

   tfree(yz_lines);
   tfree(xz_lines);
   tfree(xy_lines);

   tfree(nx_lines);
   tfree(ny_lines);
   tfree(nz_lines);

   tfree(ixu_lines);
   tfree(iyu_lines);
   tfree(izu_lines);

   tfree(ixl_lines);
   tfree(iyl_lines);
   tfree(izl_lines);

   for (n = 0; n < num_triangles; n++)
      tfree(patch_table[n]);
   tfree(patch_table);
   tfree(patch_table_len);

   /*-------------------------------------------------------------
    * Return the octrees
    *-------------------------------------------------------------*/

   *solid_octree_ptr  = solid_octree;
   *patch_octrees_ptr = patch_octrees;
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeFromInd
 *--------------------------------------------------------------------------*/
 
void    GrGeomOctreeFromInd(
GrGeomOctree  **solid_octree_ptr,
Vector         *indicator_field,
int             indicator,
double          xlower,
double          ylower,
double          zlower,
double          xupper,
double          yupper,
double          zupper,
int             octree_bg_level,
int             octree_ix,
int             octree_iy,
int             octree_iz)
{
   Grid           *grid = VectorGrid(indicator_field);

   SubgridArray   *subgrids = GridSubgrids(grid);

   Subgrid        *subgrid;
   Subvector      *subvector;

   GrGeomOctree  *solid_octree;

   GrGeomOctree  *grgeom_octree, *grgeom_child;

   int            l, min_l = 0 , max_l = 0;
   int            inc, tmp_inc;
   int            i_begin, j_begin, k_begin;
   int            i_end,   j_end,   k_end;
   int            index;
   int            state, prev_state;

   int            sg, i, j, k, ic, level, new_level;
   int            fi, iprime, jprime, kprime;

   double        *data;

   int           *ind;
   int           *edge_tag;
   unsigned char *interior_tag;


   /*------------------------------------------------------------------
    * Find the minimum and maximum levels to loop through from subgrids
    *------------------------------------------------------------------*/

   ForSubgridI(sg, subgrids)
   {
      subgrid   = SubgridArraySubgrid(subgrids, sg);

      /* level is gotten from subgrid x-resolution */
      l = SubgridRX(subgrid);

      if (sg == 0)
      {
         min_l = l;
      }
      else
      {
         if (l < min_l)
         {
            min_l = l;
         }
      }

      if (sg == 0)
      {
         max_l = l;
      }
      else
      {
         if (l > max_l)
         {
            max_l = l;
         }
      }
   }

#ifdef SGS
   printf("SGS:%d levels %d, %d\n",
	  amps_Rank(),
	  min_l, max_l);
#endif

   /*-------------------------------------------------------------
    * Create the octree
    *-------------------------------------------------------------*/

   solid_octree = GrGeomNewOctree();

   for(l = min_l; l <= max_l; l++)
   {
      ForSubgridI(sg, subgrids)
      {
         if (SubgridRX(SubgridArraySubgrid(subgrids, sg)) == l)
         {
            /* Get information about this subgrid */
            subgrid   = SubgridArraySubgrid(subgrids, sg);

            const int ix = SubgridIX(subgrid);
            const int iy = SubgridIY(subgrid);
            const int iz = SubgridIZ(subgrid);

            const int nx = SubgridNX(subgrid);
            const int ny = SubgridNY(subgrid);
            const int nz = SubgridNZ(subgrid);

            const int rx = SubgridRX(subgrid);
            const int ry = SubgridRY(subgrid);
            const int rz = SubgridRZ(subgrid);

#ifdef SGS
	    printf("SGS:%d subgrid (%d, %d, %d) (%d, %d, %d) (%d, %d, %d)\n",
		   amps_Rank(),
		   ix, iy, iz,
		   nx, ny, nz,
		   rx, ry, rz);
#endif

            /* Compute information about the subgrid and ghost layer */
            const int ix_all = ix - 2;
            const int iy_all = iy - 2;
            const int iz_all = iz - 2;

            const int nx_all = nx + 4;
            const int ny_all = ny + 4;
            const int nz_all = nz + 4;

#ifdef SGS
	    printf("SGS:%d all (%d, %d, %d) (%d, %d, %d)\n",
		   amps_Rank(),
		   ix_all, iy_all, iz_all,
		   nx_all, ny_all, nz_all);
#endif

            /* Get information about this subvector */
            subvector = VectorSubvector(indicator_field, sg);

            const int nx_f = SubvectorNX(subvector);
            const int ny_f = SubvectorNY(subvector);
            const int nz_f = SubvectorNZ(subvector);

#ifdef SGS
	    printf("SGS:%d _f (%d, %d, %d)\n",
		   amps_Rank(),
		   nx_f, ny_f, nz_f);
#endif

            data = SubvectorElt(subvector,ix_all,iy_all,iz_all);

            /* Set up the indicator array - referenced from (0,0,0) */
            ind = ctalloc(int, nx_all * ny_all * nz_all);
            fi = 0;
            BoxLoopI1(i,j,k,
                      ix_all,iy_all,iz_all,nx_all,ny_all,nz_all,
                      fi,nx_f,ny_f,nz_f,1,1,1,
            {
               iprime = i - ix_all;
               jprime = j - iy_all;
               kprime = k - iz_all;

               index = nx_all * (ny_all * kprime + jprime) + iprime;

               if (data[fi] == indicator)
               {
                  ind[index] = INSIDE;
               }
               else
               {
                  ind[index] = OUTSIDE;
               }

// SGS2
#ifdef SGS
	       debug_print(
		     printf("SGS:%d : [%i, %i, %i] ind[index] = %i, data[fi] = %e\n", 
			    amps_Rank(),
			    i,j,k,
			    ind[index],
			    data[fi]
			));
#endif
            });

            /* Get the level of the octree this subgrid */
            /*  lies on in relation to  the background  */
            level = octree_bg_level + l;
            inc = Pow2(l);

            /* Find the beginning and ending indices - accounting for bounds */
            i_begin = IndexSpaceX(pfmax(RealSpaceX(ix_all, rx), xlower), rx);
            i_end   = IndexSpaceX(pfmin(RealSpaceX((ix_all + (nx_all - 1)), rx), xupper), rx);
            j_begin = IndexSpaceY(pfmax(RealSpaceY(iy_all, ry), ylower), ry);
            j_end   = IndexSpaceY(pfmin(RealSpaceY((iy_all + (ny_all - 1)), ry), yupper), ry);
            k_begin = IndexSpaceZ(pfmax(RealSpaceZ(iz_all, rz), zlower), rz);
            k_end   = IndexSpaceZ(pfmin(RealSpaceZ((iz_all + (nz_all - 1)), rz), zupper), rz);

#ifdef SGS
	    printf("SGS:%d: k_begin comp %g, %g : (%d, %d)\n", amps_Rank(),
		   RealSpaceZ(iz_all, rz), zlower, 
		   IndexSpaceZ(RealSpaceZ(iz_all, rz), rz),
		   IndexSpaceZ(zlower, rz));
	    printf("SGS:%d: begin (%d, %d, %d) (%d, %d, %d)\n", amps_Rank(),
		   i_begin, j_begin, k_begin, i_end, j_end, k_end);


	    printf("SGS:%d: begin (%e, %e, %e) (%e, %e, %e)\n", amps_Rank(),
		   xlower, ylower, zlower,
		   xupper, yupper, zupper);
#endif

            /*-------------------------------------------------------
             * Add Inside and Outside nodes in z
             *-------------------------------------------------------*/

            edge_tag = ctalloc(int, (nz_all));
            for(j = j_begin; j <= j_end; j++)
            {
               for(i = i_begin; i <= i_end; i++)
               {
                  iprime = i - ix_all;
                  jprime = j - iy_all;
                  index = nx_all * (ny_all * 0 + jprime) + iprime;
                  prev_state = ind[index];

                  /* Fill in the edge crossing info */
                  for(k = 0; k < nz_all - 2;  k++)
                  {
                     index = nx_all * (ny_all * (k + 1) + jprime) + iprime;
                     edge_tag[k] = 0;
                     if (ind[index] == INSIDE)
                     {
                        if (prev_state != INSIDE)
                        {
                           edge_tag[k] = -1;
                        }
                        prev_state = INSIDE;
                     }
                     else
                     {
                        if (prev_state != OUTSIDE)
                        {
                           edge_tag[k] = 1;
                        }
                        prev_state = OUTSIDE;
                     }
                  }

                  /* Add faces using the edge crossing info */
                  for(k = 0; k < nz_all - 2; k++)
                  {
                     if (edge_tag[k] != 0)

                     {

#ifdef SGS
			debug_print(amps_Printf("AddFace K : (%d %d) %d %d\n", 
						(i - ((int) (octree_ix / inc))),
						(j - ((int) (octree_iy / inc))),
						((iz_all + k + 1) - ((int) (octree_iz / inc))),
						edge_tag[k]));
#endif

                        GrGeomOctreeAddFace(solid_octree, ZDIRECTION,
                                            (i - ((int) (octree_ix / inc))),
                                            (j - ((int) (octree_iy / inc))),
                                            ((iz_all + k + 1) - ((int) (octree_iz / inc))),
                                            k_begin, k_end,
                                            level,
                                            edge_tag[k]);

                     }
                  }
               }
            }
            tfree(edge_tag);

            /*-------------------------------------------------------
             * Add Inside and Outside nodes in y
             *-------------------------------------------------------*/

            edge_tag = ctalloc(int, ny_all);
            for(k = k_begin; k <= k_end; k++)
            {
               for(i = i_begin; i <= i_end; i++)
               {
                  iprime = i - ix_all;
                  kprime = k - iz_all;

                  index = nx_all * (ny_all * kprime + 0) + iprime;
                  prev_state = ind[index];

                  /* Fill in the edge crossing info */
                  for(j = 0; j < ny_all - 2;  j++)
                  {
                     index = nx_all * (ny_all * kprime + (j + 1)) + iprime;
                     edge_tag[j] = 0;
                     if (ind[index] == INSIDE)
                     {
                        if (prev_state != INSIDE)
                        {
                           edge_tag[j] = -1;
                        }
                        prev_state = INSIDE;
                     }
                     else
                     {
                        if (prev_state != OUTSIDE)
                        {
                           edge_tag[j] = 1;
                        }
                        prev_state = OUTSIDE;
                     }
                  }

                  /* Add faces using the edge crossing info */
                  for(j = 0; j < ny_all - 2; j++)
                  {
                     if (edge_tag[j] != 0)
                     {

#ifdef SGS
			debug_print(amps_Printf("AddFace J : (%d %d) %d %d\n", 
						(i - ((int) (octree_ix / inc))),
						(k - ((int) (octree_iz / inc))),
						(iy_all + j + 1- ((int) (octree_iy / inc))),
						edge_tag[j]));
#endif

                        GrGeomOctreeAddFace(solid_octree, YDIRECTION,
                                            (i - ((int) (octree_ix / inc))),
                                            (k - ((int) (octree_iz / inc))),
                                            (iy_all + j + 1 - ((int) (octree_iy / inc))),
                                            j_begin, j_end,
                                            level,
                                            edge_tag[j]);
                     }
                  }
               }
            }
            tfree(edge_tag);

            /*-------------------------------------------------------
             * Add Inside and Outside nodes in x
             *-------------------------------------------------------*/

            edge_tag = ctalloc(int, nx_all);
            for(k = k_begin; k <= k_end; k++)
            {
               for(j = j_begin; j <= j_end; j++)
               {
                  jprime = j - iy_all;
                  kprime = k - iz_all;

                  index = nx_all * (ny_all * kprime + jprime) + 0;
                  prev_state = ind[index];

                  /* Fill in the edge crossing info */
                  for(i = 0; i < nx_all - 2;  i++)
                  {
                     index = nx_all * (ny_all * kprime + jprime) + (i + 1);
                     edge_tag[i] = 0;
                     if (ind[index] == INSIDE)
                     {
                        if (prev_state != INSIDE)
                        {
                           edge_tag[i] = -1;
                        }
                        prev_state = INSIDE;
                     }
                     else
                     {
                        if (prev_state != OUTSIDE)
                        {
                           edge_tag[i] = 1;
                        }
                        prev_state = OUTSIDE;
                     }
                  }

                  /* Add faces using the edge crossing info */
                  for(i = 0; i < nx_all - 2; i++)
                  {
                     if (edge_tag[i] != 0)
                     {

#ifdef SGS
			debug_print(amps_Printf("AddFace I : (%d %d) %d %d\n", 
						(j - ((int) (octree_iy / inc))),
						(k - ((int) (octree_iz / inc))),
						(ix_all + i + 1 - ((int) (octree_ix / inc))),
						edge_tag[i]));
#endif

                        GrGeomOctreeAddFace(solid_octree, XDIRECTION,
                                            (j - ((int) (octree_iy / inc))),
                                            (k - ((int) (octree_iz / inc))),
                                            (ix_all + i + 1 - ((int) (octree_ix / inc))),
                                            i_begin, i_end,
                                            level,
                                            edge_tag[i]);
                     }
                  }
               }
            }
            tfree(edge_tag);

            /*-------------------------------------------------------
             * Add `Fill' nodes to solid_octree (in z)
             *-------------------------------------------------------*/

	    // SGS TODO why is this written like this, can't 
	    // ind just be used directly, seems to convert to 
	    // edges back to interior which we already have?

#if 1
            interior_tag   = ctalloc(unsigned char, nz_all);
            edge_tag       = ctalloc(int, nz_all);
            for(j = j_begin; j <= j_end; j++)
            {
               for(i = i_begin; i <= i_end; i++)
               {

		  iprime = i - ix_all;
		  jprime = j - iy_all;
		  
		  /* Add `Full' nodes to the octree */
		  k = k_begin;
                  while (k <= k_end)
                  {
		     kprime = k - k_begin;

		     index = nx_all * (ny_all * kprime + jprime) + iprime;

#ifdef SGS
			debug_print( 
			   printf("SGS:%d FullLoop [%i, %i, %i] [%i, %i, %i] ind[%i]=%i\n",
				  amps_Rank(),
				  (i - ((int) (octree_ix / inc))),
				  (j - ((int) (octree_iy / inc))),
				  (k - ((int) (octree_iz / inc))),
				  iprime, jprime, kprime,
				  index,
				  ind[index]
			      )
			   );
#endif


                     if (ind[index] > 0)
                     {
                        /* try to find the (i, j, k, level) octree node */
                        grgeom_octree = GrGeomOctreeFind(&new_level,
                                                         solid_octree,
                                                         (i - ((int) (octree_ix / inc))),
                                                         (j - ((int) (octree_iy / inc))),
                                                         (k - ((int) (octree_iz / inc))),
                                                         level);

#ifdef SGS
			debug_print( 
			   printf("SGS:%d GrGeomOctreeFind [%i, %i, %i] %i isfull = %i\n",
				  amps_Rank(),
				  (i - ((int) (octree_ix / inc))),
				  (j - ((int) (octree_iy / inc))),
				  (k - ((int) (octree_iz / inc))),
				  level,
				  GrGeomOctreeCellIsFull(grgeom_octree));
			   );
#endif


                        /* Only change non-`Full' cells */
                        if (!GrGeomOctreeCellIsFull(grgeom_octree))
                        {
                           if ((new_level == level) || (new_level < min_l))
                           {
                              /* Only change `Empty' cells */
                              if (GrGeomOctreeCellIsEmpty(grgeom_octree))
                              {
                                  GrGeomOctreeSetCell(grgeom_octree, GrGeomOctreeCellFull);
                              }
                           }
                           else
                           {
                              /* Create children and set */
                              GrGeomNewOctreeChildren(grgeom_octree);
                              for (ic = 0; ic < GrGeomOctreeNumChildren; ic++)
                              {
                                 grgeom_child = GrGeomOctreeChild(grgeom_octree, ic);
                                 GrGeomOctreeSetCell(grgeom_child, GrGeomOctreeCellFull);
                              }
                           }
                        }

                        /* compute the new k index */
                        tmp_inc = Pow2(level - new_level);
                        k = (((int) (k / tmp_inc)) + 1) * tmp_inc;
                     }
                     else
                     {
                        k++;
                     }
                  }
               }
            }
#else

            interior_tag   = ctalloc(unsigned char, nz_all);
            edge_tag       = ctalloc(int, nz_all);
            for(j = j_begin; j <= j_end; j++)
            {
               for(i = i_begin; i <= i_end; i++)
               {
                  iprime = i - ix_all;
                  jprime = j - iy_all;

                  index = nx_all * (ny_all * 0 + jprime) + iprime;
                  prev_state = ind[index];

                  /* Fill in the edge crossing info */
                  for(k = 0; k < nz_all - 2;  k++)
                  {
                     index = nx_all * (ny_all * (k + 1) + jprime) + iprime;
                     edge_tag[k] = 0;
                     if (ind[index] == INSIDE)
                     {
                        if (prev_state != INSIDE)
                        {
                           edge_tag[k] = -1;
                        }
                        prev_state = INSIDE;
                     }
                     else
                     {
                        if (prev_state != OUTSIDE)
                        {
                           edge_tag[k] = 1;
                        }
                        prev_state = OUTSIDE;
                     }

#ifdef SGS
		  debug_print( 
		     printf("SGS:%d edge_tag_loop  [%i, %i, %i] [%i, %i] et[]=%i\n",
			    amps_Rank(),
			    i, j, k+k_begin,
			    iprime, jprime,
			    edge_tag[k]);

		     );
#endif

                  }

                  /* Fill in the interior info using the edge crossing info */
                  index = nx_all * (ny_all * 0 + jprime) + iprime;
                  state = ind[index];
                  for(k = 0; k < nz_all - 2;  k++)
                  {
                     interior_tag[k] = 0;
                     if (state == INSIDE)
                     {
                        if (edge_tag[k] != 0)
                        {
                           state = OUTSIDE;
                        }
                        else if (edge_tag[k+1] == 0)
                        {
                           interior_tag[k] = GrGeomOctreeCellFull;
                        }
                     }
                     else
                     {
                        if (edge_tag[k] != 0)
                        {
                           state = INSIDE;
                        }
                     }

#ifdef SGS
		  debug_print( 
		     printf("SGS:%d interior_loop  [%i, %i, %i] [%i, %i] it[]=%i\n",
			    amps_Rank(),
			    i, j, k+k_begin,
			    iprime, jprime,
			    interior_tag[k]);
		     );
#endif


                  }

#ifdef SGS
		  debug_print( 
		     printf("SGS:%d k_begin = %d k_end = %d\n",
			    amps_Rank(),
			    k_begin, k_end);
		     );
#endif

                  /* Add `Full' nodes to the octree */
                  k = k_begin;
                  while (k <= k_end)
                  {
                     if (interior_tag[(k - k_begin)])
                     {
                        /* try to find the (i, j, k, level) octree node */
                        grgeom_octree = GrGeomOctreeFind(&new_level,
                                                         solid_octree,
                                                         (i - ((int) (octree_ix / inc))),
                                                         (j - ((int) (octree_iy / inc))),
                                                         (k - ((int) (octree_iz / inc))),
                                                         level);

#ifdef SGS
			debug_print( 
			   printf("SGS:%d GrGeomOctreeFind [%i, %i, %i] %i isfull = %i\n",
				  amps_Rank(),
				  (i - ((int) (octree_ix / inc))),
				  (j - ((int) (octree_iy / inc))),
				  (k - ((int) (octree_iz / inc))),
				  level,
				  GrGeomOctreeCellIsFull(grgeom_octree));
			   );
#endif


                        /* Only change non-`Full' cells */
                        if (!GrGeomOctreeCellIsFull(grgeom_octree))
                        {
                           if ((new_level == level) || (new_level < min_l))
                           {
                              /* Only change `Empty' cells */
                              if (GrGeomOctreeCellIsEmpty(grgeom_octree))
                              {
                                  GrGeomOctreeSetCell(grgeom_octree, GrGeomOctreeCellFull);
                              }
                           }
                           else
                           {
                              /* Create children and set */
                              GrGeomNewOctreeChildren(grgeom_octree);
                              for (ic = 0; ic < GrGeomOctreeNumChildren; ic++)
                              {
                                 grgeom_child = GrGeomOctreeChild(grgeom_octree, ic);
                                 GrGeomOctreeSetCell(grgeom_child, GrGeomOctreeCellFull);
                              }
                           }
                        }

                        /* compute the new k index */
                        tmp_inc = Pow2(level - new_level);
                        k = (((int) (k / tmp_inc)) + 1) * tmp_inc;
                     }
                     else
                     {
                        k++;
                     }
                  }
               }
            }
            tfree(edge_tag);
            tfree(interior_tag);
#endif

            tfree(ind);
         }
      }
   }


   /*-------------------------------------------------------------
    * Return the octree
    *-------------------------------------------------------------*/

   *solid_octree_ptr  = solid_octree;
}


/*--------------------------------------------------------------------------
 * GrGeomPrintOctreeStruc
 *--------------------------------------------------------------------------*/

void          GrGeomPrintOctreeStruc(
   amps_File     file,
   GrGeomOctree *grgeom_octree)
{
   int i;

   if  ( grgeom_octree != NULL )
   {
      if( GrGeomOctreeCellIsEmpty(grgeom_octree) ) {
	 amps_Fprintf(file, " E ");
      } else {
	 amps_Fprintf(file, " - ");
      }

      if( GrGeomOctreeCellIsOutside(grgeom_octree) ) {
	 amps_Fprintf(file, " O ");
      } else {
	 amps_Fprintf(file, " - ");
      }


      if( GrGeomOctreeCellIsInside(grgeom_octree) ) {
	 amps_Fprintf(file, " I ");
      } else {
	 amps_Fprintf(file, " - ");
      }

      if( GrGeomOctreeCellIsFull(grgeom_octree) ) {
	 amps_Fprintf(file, " F ");
      } else {
	 amps_Fprintf(file, " - ");
      }

      if( GrGeomOctreeCellIsLeaf(grgeom_octree) ) {
	 amps_Fprintf(file, " L ");
      } else {
	 amps_Fprintf(file, " - ");
      }

      if ( GrGeomOctreeParent(grgeom_octree) != NULL )
      {
         amps_Fprintf(file, "P;");
      }
      else
      {
         amps_Fprintf(file, "-;");
      }
      amps_Fprintf(file, " ");

      if (GrGeomOctreeHasChildren(grgeom_octree))
      {
         for(i = 0; i < (GrGeomOctreeNumChildren - 1); i++)
         {
            if ( GrGeomOctreeChild(grgeom_octree,i) != NULL )
            {
               amps_Fprintf(file,  "y,");
            }
            else
            {
               amps_Fprintf(file,  "n,");
            }
         }
         if ( GrGeomOctreeChild(grgeom_octree,(GrGeomOctreeNumChildren-1)) != NULL )
         {
            amps_Fprintf(file,  "y;");
         }
         else
         {
            amps_Fprintf(file,  "n;");
         }
      }
      else
      {
         for(i = 0; i < (GrGeomOctreeNumChildren - 1); i++)
         {
            amps_Fprintf(file,  "*,");
         }
         amps_Fprintf(file,  "*;");
      }
      amps_Fprintf(file, " ");

      for(i = 0; i < (GrGeomOctreeNumFaces - 1); i++)
      {
         if (GrGeomOctreeHasFace(grgeom_octree,i))
         {
            amps_Fprintf(file,  "1,");
         }
         else
         {
            amps_Fprintf(file,  "0,");
         }
      }
      if (GrGeomOctreeHasFace(grgeom_octree,(GrGeomOctreeNumFaces-1)))
      {
         amps_Fprintf(file,  "1\n");
      }
      else
      {
         amps_Fprintf(file,  "0\n");
      }
   }
}


/*--------------------------------------------------------------------------
 * GrGeomPrintOctreeLevel
 *--------------------------------------------------------------------------*/

int           GrGeomPrintOctreeLevel(
amps_File     file,
GrGeomOctree *grgeom_octree,
int           level,
int           current_level)
{
   int  i;

   int  ret, level_printed;


   level_printed = FALSE;
   if  ( current_level == level )
   {
      GrGeomPrintOctreeStruc(file, grgeom_octree);
      level_printed = TRUE;
   }
   else if (GrGeomOctreeChildren(grgeom_octree) != NULL)
   {
      for(i = 0; i < GrGeomOctreeNumChildren; i++)
      {
	 ret =
	    GrGeomPrintOctreeLevel(file, GrGeomOctreeChild(grgeom_octree,i),
				   level, current_level+1);
	 if (ret)
	    level_printed = TRUE;
      }
   }

   return level_printed;
}


/*--------------------------------------------------------------------------
 * GrGeomPrintOctree
 *--------------------------------------------------------------------------*/

void          GrGeomPrintOctree(
char         *filename,
GrGeomOctree *grgeom_octree_root)
{
   amps_File     file;

   int  level;

   int  still_more_levels = TRUE;


   if ((file = amps_Fopen(filename, "w")) == NULL)
      InputError("Error: can't open output file %s\n\n", filename, "");

   level = 0;
   while (still_more_levels)
   {
      amps_Fprintf(file, "\n\nlevel = %d:\n", level);
	 
      still_more_levels =
	 GrGeomPrintOctreeLevel(file, grgeom_octree_root, level, 0);

      level++;
   }

   amps_Fclose(file);
}


/*--------------------------------------------------------------------------
 * GrGeomPrintOctreeCells:
 *   Prints out octree cell info for all levels up to `last_level'.
 *--------------------------------------------------------------------------*/

void          GrGeomPrintOctreeCells(
char         *filename,
GrGeomOctree *octree,
int           last_level)
{
   amps_File     file;

   GrGeomOctree *node;
   int           level, i, j, k, l, cell;


   if ((file = amps_Fopen(filename, "w")) == NULL)
      InputError("Error: can't open output file %s%s\n", filename, "");

   for (level = 0; level <= last_level; level++)
   {
      amps_Fprintf(file, "\n\nlevel = %d:\n", level);
	 
      i = 0; j = 0; k = 0;

      GrGeomOctreeLoop(i, j, k, l, node, octree, level, TRUE,
      {
	 amps_Fprintf(file, "%d %d %d %d: ", i, j, k, l);

	 cell = GrGeomOctreeCell(node) & ~GrGeomOctreeCellLeaf;

	 switch(cell)
	 {
	 case GrGeomOctreeCellEmpty:
	    amps_Fprintf(file, "Empty\n");
	    break;
	 case GrGeomOctreeCellOutside:
	    amps_Fprintf(file, "Outside\n");
	    break;
	 case GrGeomOctreeCellInside:
	    amps_Fprintf(file, "Inside\n");
	    break;
	 case GrGeomOctreeCellFull:
	    amps_Fprintf(file, "Full\n");
	    break;
	 }
      },
      {
	 amps_Fprintf(file, "%d %d %d %d: ", i, j, k, l);

	 cell = GrGeomOctreeCell(node) & ~GrGeomOctreeCellLeaf;

	 switch(cell)
	 {
	 case GrGeomOctreeCellEmpty:
	    amps_Fprintf(file, "Empty\n");
	    break;
	 case GrGeomOctreeCellOutside:
	    amps_Fprintf(file, "Outside\n");
	    break;
	 case GrGeomOctreeCellInside:
	    amps_Fprintf(file, "Inside\n");
	    break;
	 case GrGeomOctreeCellFull:
	    amps_Fprintf(file, "Full\n");
	    break;
	 }
      });
   }

   amps_Fclose(file);
}


/*--------------------------------------------------------------------------
 * GrGeomOctreeFree
 *--------------------------------------------------------------------------*/

void GrGeomOctreeFree(GrGeomOctree *grgeom_octree_root)
{
   int i;

   if  ( grgeom_octree_root != NULL )
   {
      if (GrGeomOctreeChildren(grgeom_octree_root) != NULL)
      {
         for(i = 0; i < GrGeomOctreeNumChildren; i++)
         {
            GrGeomOctreeFree(GrGeomOctreeChild(grgeom_octree_root,i));
         }
      }
      GrGeomFreeOctree(grgeom_octree_root);
      
   }
}

