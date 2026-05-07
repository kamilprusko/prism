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

#include "ray-tracer.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include <vtkCellArray.h>
#include <vtkPolyData.h>

#define MAX_DISTANCE     (1.0e15)
#define MIN_RAY_LENGTH   (1.0e-6)

namespace {
constexpr vtkIdType kFallbackMaxVertsPerCell = 16;
}

bool _is_nan (const double var)
{
    volatile double d = var;
    return d != d;
}

double _reflect (const double normal[3],
                 const double incident[3],
                 double       reflection[3],
                 const double n1,
                 const double n2)
{
    (void)n1;
    (void)n2;
    double cos_theta1 = -vtkMath::Dot (incident, normal);
    double transmittance = 1.0;

    reflection[0] = incident[0] + 2.0 * cos_theta1 * normal[0];
    reflection[1] = incident[1] + 2.0 * cos_theta1 * normal[1];
    reflection[2] = incident[2] + 2.0 * cos_theta1 * normal[2];

    vtkMath::Normalize (reflection);

    return transmittance;
}

double _refract (const double normal[3],
                 const double incident[3],
                 double       refraction[3],
                 const double n1,
                 const double n2)
{
    double eta        =  n1 / n2;
    double cos_theta1 = -vtkMath::Dot (incident, normal);
    double cos_theta2 =  sqrt (1.0 - (eta*eta*(1.0 - cos_theta1*cos_theta1)));

    if (_is_nan(cos_theta2))
    {
        return 0.0;
    }

    refraction[0] = eta*incident[0] + (eta*cos_theta1 - cos_theta2) * normal[0];
    refraction[1] = eta*incident[1] + (eta*cos_theta1 - cos_theta2) * normal[1];
    refraction[2] = eta*incident[2] + (eta*cos_theta1 - cos_theta2) * normal[2];

    vtkMath::Normalize (refraction);

    double fresnel_Rs = pow ((n1*cos_theta1 - n2*cos_theta2) / (n1*cos_theta1 + n2*cos_theta2), 2);
    double fresnel_Rp = pow ((n1*cos_theta2 - n2*cos_theta1) / (n1*cos_theta2 + n2*cos_theta1), 2);

    double reflectance = (fresnel_Rs + fresnel_Rp) / 2.0;

    if (n1 > n2)
    {
        (void)reflectance;
    }

    return 1.0 - reflectance;
}

bool _intersect_triangle (const double orig[3],
                          const double direction[3],
                          const double v0[3],
                          const double v1[3],
                          const double v2[3],
                          double      *t,
                          double      *u,
                          double      *v)
{
    double dir[3] = {
        direction[0], direction[1], direction[2]
        };
    vtkMath::Normalize (dir);

    double edge1[3], edge2[3], pvec[3], qvec[3], tvec[3];
    double det, inv_det;

    vtkMath::Subtract (v1, v0, edge1);
    vtkMath::Subtract (v2, v0, edge2);

    vtkMath::Cross (dir, edge2, pvec);

    det = vtkMath::Dot (edge1, pvec);

    if (det > -VTK_DBL_EPSILON && det < VTK_DBL_EPSILON)
        return false;

    inv_det = 1.0 / det;

    vtkMath::Subtract (orig, v0, tvec);

    *u = vtkMath::Dot (tvec, pvec) * inv_det;
    if (*u < 0.0 || *u > 1.0)
        return false;

    vtkMath::Cross (tvec, edge1, qvec);

    *v = vtkMath::Dot (dir, qvec) * inv_det;
    if (*v < 0.0 || *u + *v > 1.0)
        return false;

    *t = vtkMath::Dot (edge2, qvec) * inv_det;

    return true;
}

bool _cull_test (const double direction[3],
                 double normal[3])
{
    return (vtkMath::Dot (direction, normal) >= 0.0);
}


RayTracer::RayTracer ()
    : points (nullptr)
    , lines (nullptr)
    , colors (nullptr)
    , beam_data (nullptr)
    , trace_surface_mapper (nullptr)
{
}

void RayTracer::set_beam_data (vtkPolyData *data)
{
    this->beam_data = data;
}

void RayTracer::set_trace_surface_mapper (vtkPolyDataMapper *mapper)
{
    this->trace_surface_mapper = mapper;
}

void RayTracer::set_refract_color_callback (RayTracerColorCallback cb)
{
    this->on_refract_color_ = std::move (cb);
}

void RayTracer::set_reflect_color_callback (RayTracerColorCallback cb)
{
    this->on_reflect_color_ = std::move (cb);
}

void RayTracer::render (const double orgin[3],
                        const double direction[3],
                        const double color[4])
{
    this->points = vtkPoints::New();
    this->lines = vtkCellArray::New();
    this->colors = vtkUnsignedCharArray::New();
    this->colors->SetNumberOfComponents (4);

    std::vector<vtkIdType> orgin_poly_ids (Limits::excluded_face_id_capacity,
                                           static_cast<vtkIdType> (-1));
    const vtkIdType orgin_point_id = this->points->InsertNextPoint (orgin);

    double direction_normalized[3] = {
        direction[0], direction[1], direction[2]
    };

    vtkMath::Normalize (direction_normalized);

    this->cast (orgin,
                direction_normalized,
                color,
                orgin_point_id,
                orgin_poly_ids.data (),
                orgin_poly_ids.size (),
                0);
    this->update ();
}

void RayTracer::cast (const double     orgin[3],
                      const double     direction[3],
                      const double     color[4],
                      const vtkIdType  orgin_point_id,
                      const vtkIdType *orgin_poly_ids,
                      const std::size_t orgin_poly_ids_count,
                      const int        cast_depth)
{
    if (cast_depth >= Limits::max_cast_depth)
    {
        return;
    }

    if (!this->trace_surface_mapper)
    {
        return;
    }

    vtkPolyData *input = vtkPolyData::SafeDownCast (this->trace_surface_mapper->GetInput ());
    if (!input)
    {
        return;
    }
    vtkPoints    *input_points = input->GetPoints ();
    vtkCellArray *input_polys  = input->GetPolys ();
    if (!input_points || !input_polys)
    {
        return;
    }

    std::vector<vtkIdType> projected_poly_ids (orgin_poly_ids_count,
                                               static_cast<vtkIdType> (-1));
    long projected_poly_ids_length = 0;

    double refracted[3],
           reflected[3],
           normal[3],

           length            = 10000.0,
           transmission      = 1.0,
           reflectance       = 0.0,
           n1                = 1.0003,
           n2                = 1.5000,

           projected_distance = MAX_DISTANCE,
           projected[3]       = {
               orgin[0] + length * direction[0],
               orgin[1] + length * direction[1],
               orgin[2] + length * direction[2]
           };

    bool is_inside = false;

    vtkIdType point_count;
    const vtkIdType *point_ids;

    const vtkIdType max_cell_size = input->GetMaxCellSize ();
    const vtkIdType min_cell_verts = 3;
    std::vector<std::array<double, 3>> p(
        static_cast<size_t> (max_cell_size > min_cell_verts ? max_cell_size : kFallbackMaxVertsPerCell));

    double distance, t, u, v;

    input_polys->InitTraversal ();

    const int poly_id_limit = static_cast<int> (orgin_poly_ids_count);

    for (int poly_id = 0; input_polys->GetNextCell (point_count, point_ids); poly_id++)
    {
        for (int i = 0; i < poly_id_limit && orgin_poly_ids[i] >= 0; i++)
            if (orgin_poly_ids[i] == poly_id)
                goto NextPoly;

        if (false)
            NextPoly: continue;

        if (point_count > static_cast<vtkIdType> (p.size ()))
        {
            p.resize (static_cast<size_t> (point_count));
        }
        for (int i = 0; i < point_count; i++)
            input_points->GetPoint (point_ids[i], p[static_cast<size_t> (i)].data ());

        if (!_intersect_triangle (orgin, direction, p[0].data (), p[1].data (), p[2].data (), &t, &u, &v))
            continue;

        if (_is_nan (t))
            continue;

        if (t < 0.0)
            continue;

        double intersected[3] = {
            orgin[0] + t * direction[0],
            orgin[1] + t * direction[1],
            orgin[2] + t * direction[2]
        };

        distance = sqrt (
            (orgin[0] - intersected[0]) * (orgin[0] - intersected[0]) +
            (orgin[1] - intersected[1]) * (orgin[1] - intersected[1]) +
            (orgin[2] - intersected[2]) * (orgin[2] - intersected[2])
            );

        if (distance < projected_distance)
        {
            projected[0] = intersected[0];
            projected[1] = intersected[1];
            projected[2] = intersected[2];

            projected_poly_ids_length = 0;
            projected_poly_ids[static_cast<size_t> (projected_poly_ids_length++)] = poly_id;
            projected_distance = distance;

            double edge1[3] = { p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2] };
            double edge2[3] = { p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2] };
            vtkMath::Cross (edge1, edge2, normal);
            vtkMath::Normalize (normal);

            is_inside = _cull_test (direction, normal);

            if (is_inside)
            {
                normal[0] = -normal[0];
                normal[1] = -normal[1];
                normal[2] = -normal[2];
            }

            if (projected_distance < MIN_RAY_LENGTH)
            {
                for (int i = 0;
                     i < poly_id_limit && orgin_poly_ids[i] >= 0
                     && projected_poly_ids_length < poly_id_limit - 1;
                     i++)
                    projected_poly_ids[static_cast<size_t> (projected_poly_ids_length++)] =
                        orgin_poly_ids[i];

                break;
            }
        }
    }

    vtkIdType projected_point_id = orgin_point_id;

    double projected_color[4] = { color[0], color[1], color[2], color[3] };
    for (int c = 0; c < 4; ++c)
    {
        projected_color[c] = std::clamp (projected_color[c], 0.0, 1.0);
    }

    unsigned char projected_color_uc[4] = {
        (unsigned char)(255 * projected_color[0]),
        (unsigned char)(255 * projected_color[1]),
        (unsigned char)(255 * projected_color[2]),
        (unsigned char)(255 * projected_color[3])
    };

    if (projected_distance > MIN_RAY_LENGTH)
    {
        projected_point_id = this->points->InsertNextPoint (projected);

        vtkLine *line = vtkLine::New ();
        line->GetPointIds ()->SetId (0, orgin_point_id);
        line->GetPointIds ()->SetId (1, projected_point_id);
        this->lines->InsertNextCell (line);
        line->Delete ();
        this->colors->InsertNextTypedTuple (projected_color_uc);
    }

    if (this->points->GetNumberOfPoints () > Limits::max_beam_points)
    {
        return;
    }

    if (projected_poly_ids_length + 1 < poly_id_limit)
    {
        projected_poly_ids[static_cast<size_t> (projected_poly_ids_length + 1)] =
            static_cast<vtkIdType> (-1);
    }

    double tmp = n1;

    if (is_inside)
    {
        tmp = n1;
        n1  = n2;
        n2  = tmp;
    }

    if (projected_distance < MAX_DISTANCE)
    {
        transmission = _refract (normal, direction, refracted, n1, n2);
        reflectance  = _reflect (normal, direction, reflected, n1, n2);
        reflectance  = 1.0 - transmission;

        double refracted_color[4] = { color[0], color[1], color[2], transmission * color[3] },
               reflected_color[4] = { color[0], color[1], color[2], reflectance * color[3] };

        RayHitColorContext ctx{};
        ctx.primary_cell_id           = projected_poly_ids[0];
        ctx.related_cell_ids          = projected_poly_ids.data ();
        ctx.related_cell_ids_capacity = orgin_poly_ids_count;
        ctx.hit_point[0]              = projected[0];
        ctx.hit_point[1]              = projected[1];
        ctx.hit_point[2]              = projected[2];
        ctx.normal[0]                 = normal[0];
        ctx.normal[1]                 = normal[1];
        ctx.normal[2]                 = normal[2];
        ctx.ray_origin_inside         = is_inside;
        ctx.n1                        = n1;
        ctx.n2                        = n2;
        ctx.incoming_rgba             = color;

        if (this->on_refract_color_)
        {
            std::memcpy (ctx.refracted_rgba, refracted_color, sizeof (ctx.refracted_rgba));
            std::memcpy (ctx.reflected_rgba, reflected_color, sizeof (ctx.reflected_rgba));
            this->on_refract_color_ (ctx);
            std::memcpy (refracted_color, ctx.refracted_rgba, sizeof (ctx.refracted_rgba));
            std::memcpy (reflected_color, ctx.reflected_rgba, sizeof (ctx.reflected_rgba));
        }
        if (this->on_reflect_color_)
        {
            std::memcpy (ctx.refracted_rgba, refracted_color, sizeof (ctx.refracted_rgba));
            std::memcpy (ctx.reflected_rgba, reflected_color, sizeof (ctx.reflected_rgba));
            this->on_reflect_color_ (ctx);
            std::memcpy (refracted_color, ctx.refracted_rgba, sizeof (ctx.refracted_rgba));
            std::memcpy (reflected_color, ctx.reflected_rgba, sizeof (ctx.reflected_rgba));
        }

        if ((reflected_color[0] == 0.0) &&
            (reflected_color[1] == 0.0) &&
            (reflected_color[2] == 0.0))
            reflected_color[3] = 0.0;

        if (refracted_color[3] > 0.01)
            this->cast (projected,
                        refracted,
                        refracted_color,
                        projected_point_id,
                        projected_poly_ids.data (),
                        orgin_poly_ids_count,
                        cast_depth + 1);

        if (reflected_color[3] > 0.01)
            this->cast (projected,
                        reflected,
                        reflected_color,
                        projected_point_id,
                        projected_poly_ids.data (),
                        orgin_poly_ids_count,
                        cast_depth + 1);
    }
}

void RayTracer::update ()
{
    beam_data->SetPoints (this->points);
    beam_data->SetLines (this->lines);
    beam_data->GetCellData ()->SetScalars (this->colors);
}
