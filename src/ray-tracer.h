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

#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <cstddef>
#include <functional>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkUnsignedCharArray.h>

/** Per-hit data for optional color hooks (indices are trace-mesh cell ids). */
struct RayHitColorContext
{
    vtkIdType         primary_cell_id;
    const vtkIdType  *related_cell_ids;
    std::size_t       related_cell_ids_capacity;
    double            hit_point[3];
    double            normal[3];
    bool              ray_origin_inside;
    double            n1;
    double            n2;
    const double     *incoming_rgba;
    double            refracted_rgba[4];
    double            reflected_rgba[4];
};

using RayTracerColorCallback = std::function<void (RayHitColorContext &)>;

class RayTracer
{
public:
    /** Policy for recursion depth, beam size, and exclusion-id stack buffers. */
    struct Limits
    {
        static constexpr int max_cast_depth = 64;
        /** Upper bound on vtkPoints used for the beam (replaces an informal ~400 cap). */
        static constexpr int max_beam_points = 512;
        /** Extra slots for grazing / zero-distance ancestor id copies on one bounce. */
        static constexpr int exclusion_id_slack = 8;
        static constexpr std::size_t excluded_face_id_capacity =
            static_cast<std::size_t>(max_cast_depth + exclusion_id_slack);
    };

private:
    vtkPoints               *points;
    vtkCellArray            *lines;
    vtkUnsignedCharArray    *colors;
    vtkPolyData             *beam_data;
    /** Mapper whose input is the sole surface used for ray intersection (prism only in this app). */
    vtkPolyDataMapper       *trace_surface_mapper;
    RayTracerColorCallback   on_refract_color_;
    RayTracerColorCallback   on_reflect_color_;

public:
    RayTracer ();
    void set_beam_data (vtkPolyData *data);
    /** Input pipeline must be the trace surface only—not ground or composite scene geometry. */
    void set_trace_surface_mapper (vtkPolyDataMapper *mapper);
    void set_refract_color_callback (RayTracerColorCallback cb);
    void set_reflect_color_callback (RayTracerColorCallback cb);
    void render (const double orgin[3], const double direction[3], const double color[4]);
    void cast (const double orgin[3],
               const double direction[3],
               const double color[4],
               vtkIdType orgin_point_id,
               const vtkIdType *orgin_poly_ids,
               std::size_t orgin_poly_ids_count,
               int cast_depth);
    void update ();
};

#endif
