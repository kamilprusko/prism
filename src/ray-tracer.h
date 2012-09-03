/*
 * Copyright (c) 2010  Kamil Prusko
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkUnsignedCharArray.h"

class RayTracer
{
private:
    vtkPoints            *points;
    vtkCellArray         *lines;
    vtkUnsignedCharArray *colors;
    vtkPolyData          *beam_data;
    vtkPolyDataMapper    *prism_mapper;

public:
    RayTracer ();
    void set_beam_data (vtkPolyData *data);
    void set_prism_mapper (vtkPolyDataMapper *mapper);
    void render (const double orgin[3], const double direction[3], const double color[4]);
    void cast (const double orgin[3], const double direction[3], const double color[4], const vtkIdType orginPointID, const vtkIdType *orginPolyIDs);
    void update ();
};
