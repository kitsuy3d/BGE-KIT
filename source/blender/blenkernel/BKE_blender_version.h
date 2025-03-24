/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef __BKE_BLENDER_VERSION_H__
#define __BKE_BLENDER_VERSION_H__

/** \file BKE_blender_version.h
 *  \ingroup bke
 */

/* these lines are grep'd, watch out for our not-so-awesome regex
 * and keep comment above the defines.
 * Use STRINGIFY() rather than defining with quotes */
#define BLENDER_VERSION         279
#define BLENDER_SUBVERSION      7
/* Several breakages with 270, e.g. constraint deg vs rad */
#define BLENDER_MINVERSION      270
#define BLENDER_MINSUBVERSION   6
/* UPBGE2x: I think it's better not to change upbge versioning a lot, to avoid incompatibility, we use 0.2.6xxx. */
#define UPBGE_VERSION           2
#define UPBGE_SUBVERSION        6016
/* UPBGE 0.2.6xxx: I think it's better not to change RanGE versioning, to avoid incompatibility. */
#define RANGE_VERSION           1
#define RANGE_SUBVERSION        0

/* used by packaging tools */
/* can be left blank, otherwise a,b,c... etc with no quotes */
#define BLENDER_VERSION_CHAR
/* alpha/beta/rc/release, docs use this */
#define BLENDER_VERSION_CYCLE   alpha

extern char versionstr[]; /* from blender.c */
extern char range_versionstr[]; /* from blender.c */
extern char upbge_versionstr[]; /* from blender.c */

#endif  /* __BKE_BLENDER_VERSION_H__ */
