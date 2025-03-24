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
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

#include "GPU_glew.h"
#include "GPU_vertex_array.h"

void GPU_create_vertex_arrays(int n, unsigned int *arrays)
{
#ifndef __APPLE__
	glGenVertexArrays(n, arrays);
#else
	glGenVertexArraysAPPLE(n, arrays);
#endif
}

void GPU_bind_vertex_array(unsigned int array)
{
#ifndef __APPLE__
	glBindVertexArray(array);
#else
	glBindVertexArrayAPPLE(array);
#endif
}

void GPU_unbind_vertex_array(void)
{
#ifndef __APPLE__
	glBindVertexArray(0);
#else
	glBindVertexArrayAPPLE(0);
#endif
}

void GPU_delete_vertex_arrays(int n, const unsigned int *arrays)
{
#ifndef __APPLE__
	glDeleteVertexArrays(n, arrays);
#else
	glDeleteVertexArraysAPPLE(n, arrays);
#endif
}
