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

#define MAX_DISTANCE  (1.0e15)
#define MIN_RAY_LENGTH  (1.0e-6)


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
    // NOTE: all vectors need to be normalized

    // double eta = n1 / n2;  // For material that absorbs light, like metals and semiconductors,
                           // n is complex, and Rp does not generally go to zero.

    double cos_theta1 = -vtkMath::Dot (incident, normal);
    // double cos_theta2 =  sqrt (1.0 - (eta*eta*(1.0 - cos_theta1*cos_theta1)));
    double transmittance = 1.0;

    reflection[0] = incident[0] + 2.0 * cos_theta1 * normal[0];
    reflection[1] = incident[1] + 2.0 * cos_theta1 * normal[1];
    reflection[2] = incident[2] + 2.0 * cos_theta1 * normal[2];

    vtkMath::Normalize (reflection);

    /*
    // Fresnel equations
    double fresnel_Rs = pow ((n1*cos_theta1 - n2*cos_theta2) / (n1*cos_theta1 + n2*cos_theta2), 2);
    double fresnel_Rp = pow ((n1*cos_theta2 - n2*cos_theta1) / (n1*cos_theta2 + n2*cos_theta1), 2);

    double reflectance = (fresnel_Rs + fresnel_Rp) / 2.0;

    if (n1 > n2)
    {
        // Total Internal Reflection

        double brewstersAngle = atan (n2 / n1);
        // theta1 + theta2 == 90
        //printf ("--> %g, %g - %g\n", fresnel_Rs, fresnel_Rp, brewstersAngle*180.0/M_PI);

        //if(n1 + n2 != 0.0)
            reflectance = pow((n1 - n2) / (n1 + n2), 2);
    }

    return reflectance;
    */

    return transmittance;
}

double _refract (const double normal[3],
                 const double incident[3],
                 double       refraction[3],
                 const double n1,
                 const double n2)
{
    // NOTE: all vectors need to be normalized

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

    // Fresnel equations
    double fresnel_Rs = pow ((n1*cos_theta1 - n2*cos_theta2) / (n1*cos_theta1 + n2*cos_theta2), 2);
    double fresnel_Rp = pow ((n1*cos_theta2 - n2*cos_theta1) / (n1*cos_theta2 + n2*cos_theta1), 2);

    double reflectance = (fresnel_Rs + fresnel_Rp) / 2.0;

    if (n1 > n2)
    {
        // Total Internal Reflection

        // double brewstersAngle = atan (n2 / n1);
        //// theta1 + theta2 == 90
        //// printf ("--> %g, %g - %g\n", fresnel_Rs, fresnel_Rp, brewstersAngle*180.0/M_PI);

        // reflectance = pow((n1 - n2) / (n1 + n2), 2);
    }

    return 1.0 - reflectance;

    // double transmittance = ((1.0-fresnel_Rs) + (1.0-fresnel_Rp)) / 2.0;
    // return transmittance;
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
    // Based on paper: "Fast, minimum storage ray/triangle intersection" by Tomas Möller & Ben Trumbore
    double dir[3] = {
        direction[0], direction[1], direction[2]
        };
    vtkMath::Normalize (dir);

    double edge1[3], edge2[3], pvec[3], qvec[3], tvec[3];
    double det, inv_det;

    // Find vectors for two edges sharing v0
    vtkMath::Subtract (v1, v0, edge1);
    vtkMath::Subtract (v2, v0, edge2);

    // Begin calculating determinant - also used to calculate U param
    vtkMath::Cross (dir, edge2, pvec);

    // If determinant is near zero, ray lies in place of triangle
    det = vtkMath::Dot (edge1, pvec);

    if (det > -VTK_DBL_EPSILON && det < VTK_DBL_EPSILON)
        return false;

    inv_det = 1.0 / det;

    // Calculate distance from vert0 to ray orgin
    vtkMath::Subtract (orig, v0, tvec);

    // Calculate U parameter and test bounds
    *u = vtkMath::Dot (tvec, pvec) * inv_det;
    if (*u < 0.0 || *u > 1.0)
        return false;

    // Prepare to test V parameter
    vtkMath::Cross (tvec, edge1, qvec);

    // calculate V parameter and test bounds
    *v = vtkMath::Dot (dir, qvec) * inv_det;
    if (*v < 0.0 || *u + *v > 1.0)
        return false;

    // calculate t, ray intersects triangle
    *t = vtkMath::Dot (edge2, qvec) * inv_det;

    return true;
}

bool _cull_test (const double direction[3],
                 double normal[3])
{
    return (vtkMath::Dot (direction, normal) >= 0.0);
}


RayTracer::RayTracer ()
{
}

void RayTracer::set_beam_data (vtkPolyData *data)
{
    this->beam_data = data;
}

void RayTracer::set_prism_mapper (vtkPolyDataMapper *mapper)
{
    this->prism_mapper = mapper;
}

void RayTracer::render (const double orgin[3],
                        const double direction[3],
                        const double color[4])
{
    //Create a vtkPoints object and store the points in it
    this->points = vtkPoints::New();

    //Create a cell array to store the lines in and add the lines to it
    this->lines = vtkCellArray::New();

    //Setup the colors array
    this->colors = vtkUnsignedCharArray::New();
    this->colors->SetNumberOfComponents(4);

    vtkIdType orginPolyIDs[1] = {-1};
    vtkIdType orginPointID =
        this->points->InsertNextPoint (orgin);

    double directionNormalized[3] = {
        direction[0], direction[1], direction[2]
        };

    vtkMath::Normalize (directionNormalized);

    this->cast (orgin, directionNormalized, color, orginPointID, orginPolyIDs);
    this->update();
}

void RayTracer::cast (const double     orgin[3],
                      const double     direction[3],
                      const double     color[4],
                      const vtkIdType  orginPointID,
                      const vtkIdType *orginPolyIDs)
{
    vtkPolyData  *input       = prism_mapper->GetInput();
    vtkPoints    *inputPoints = input->GetPoints();
    vtkCellArray *inputPolys  = input->GetPolys();

    vtkIdType     projectedPolyIDs[8] = {-1, -1};
    long          projectedPolyIDsLength = 0;


    double        refracted[3],
                  reflected[3],
                  normal[3],

                  length       = 10000.0,
                  transmission = 1.0,
                  reflectance  = 0.0,
                  n1           = 1.0003,
                  n2           = 1.5000,

                  projectedDistance = MAX_DISTANCE,
                  projected[3] = {
                        orgin[0] + length*direction[0],
                        orgin[1] + length*direction[1],
                        orgin[2] + length*direction[2]
                  };

    bool is_inside;

    // Iterate Polygons
    vtkIdType *pointIDs,
               pointCount;

    double     p[input->GetMaxCellSize()][3],
               distance, t, u, v;

    inputPolys->InitTraversal();

    for (int polyID=0; inputPolys->GetNextCell(pointCount,pointIDs); polyID++)
    {
        // Exclude orgin polys
        for (int i=0; i<8 && orginPolyIDs[i] >= 0; i++)
            if (orginPolyIDs[i] == polyID)
                goto NextPoly;

        if (false)
            NextPoly: continue;

        // Make local copy of poly points
        for (int i=0; i<pointCount; i++)
            inputPoints->GetPoint (pointIDs[i], p[i]);

        // Test for Plane Intersection
        // TODO: Support quads too
        if (!_intersect_triangle (orgin, direction, p[0], p[1], p[2], &t, &u, &v))
            continue;

        // Ensure correct direction
        if (_is_nan(t))
            continue;

        if (t < 0.0)
        {
            // FIXME: find another zero-distance poly
            //    projectedPolyIDs [projectedPolyIDsLength++] = polyID;
            continue;
        }

        // Intersection point
        double intersected[3] = {
            orgin[0] + t*direction[0],
            orgin[1] + t*direction[1],
            orgin[2] + t*direction[2]
            };

        distance = sqrt (
            (orgin[0]-intersected[0]) * (orgin[0]-intersected[0]) +
            (orgin[1]-intersected[1]) * (orgin[1]-intersected[1]) +
            (orgin[2]-intersected[2]) * (orgin[2]-intersected[2])
            );

        // Find nearest poly
        if (distance < projectedDistance)
        {
            projected[0] = intersected[0];
            projected[1] = intersected[1];
            projected[2] = intersected[2];

            //if (distance == projectedDistance)
            projectedPolyIDsLength = 0;
            projectedPolyIDs [projectedPolyIDsLength++] = polyID;
            projectedDistance = distance;

            // Find polygon normal
            double edge1[3] = { p[1][0]-p[0][0], p[1][1]-p[0][1], p[1][2]-p[0][2] };
            double edge2[3] = { p[2][0]-p[0][0], p[2][1]-p[0][1], p[2][2]-p[0][2] };
            // SUB (points[1], points[0], edge1);
            // SUB (points[2], points[0], edge2);
            vtkMath::Cross (edge1, edge2, normal);
            vtkMath::Normalize (normal);

            is_inside = _cull_test (direction, normal);

            if (is_inside)
            {
                normal[0] = - normal[0];
                normal[1] = - normal[1];
                normal[2] = - normal[2];
            }

            if (projectedDistance < MIN_RAY_LENGTH) // distance < VTK_DBL_EPSILON)
            {
                // XXX: Gives no or allittle effect

                // if found zero-distance, there is no use to search better match;
                // include also previous zero-distance polys
                for (int i=0; i<8 && orginPolyIDs[i] >= 0; i++)
                    projectedPolyIDs [projectedPolyIDsLength++] = orginPolyIDs[i];

                break;
            }
        }
    }

    // Even if not projected, cast ray into infinity
    // ...

    vtkIdType projectedPointID = orginPointID;

    double projected_color[4] = { color[0], color[1], color[2], color[3] };
    double color_range[2] = {0.0, 1.0};
    vtkMath::ClampValue (projected_color, color_range);

    unsigned char projected_color_uc[4] = {
                        (unsigned char)(255 * projected_color[0]),
                        (unsigned char)(255 * projected_color[1]),
                        (unsigned char)(255 * projected_color[2]),
                        (unsigned char)(255 * projected_color[3])
                        };

    if (projectedDistance > MIN_RAY_LENGTH)
    {
        projectedPointID = this->points->InsertNextPoint (projected);

        vtkLine *line = vtkLine::New();
        line->GetPointIds()->SetId (0, orginPointID);     // the second 0 is the index of the Origin in the vtkPoints
        line->GetPointIds()->SetId (1, projectedPointID); // the second 1 is the index of P0 in the vtkPoints
        this->lines->InsertNextCell (line);
        this->colors->InsertNextTupleValue (projected_color_uc);
    }


    if (projectedPointID > 400)
    {
        //debug ("**Warning: Maximum trace count reached. Ray tracing halted.\n");
        return;
    }

    projectedPolyIDs [projectedPolyIDsLength+1] = -1; // Mark end of the list

    // a ray of light strikes a medium boundary at an angle larger
    // than a particular critical angle with respect to the normal to the surface.
    // If the refractive index is lower on the other side of the boundary,
    // no light can pass through and all of the light is reflected.
    // The critical angle is the angle of incidence above which the total internal reflection occurs.

    double tmp = n1;

    if (is_inside)
    {
        tmp = n1;
        n1  = n2;
        n2  = tmp;
    }

    if (projectedDistance < MAX_DISTANCE)
    {
        transmission = _refract (normal, direction, refracted, n1, n2);
        reflectance  = _reflect (normal, direction, reflected, n1, n2);
        reflectance  = 1.0 - transmission;

        int f1 = projectedPolyIDs[0] == 138 || projectedPolyIDs[0] == 139,
            f2 = projectedPolyIDs[0] == 26  || projectedPolyIDs[0] == 27 ||
                 projectedPolyIDs[1] == 26  || projectedPolyIDs[1] == 27;

        double refractedColor[4] = {color[0], color[1], color[2], transmission*color[3]},
               reflectedColor[4] = {color[0], color[1], color[2], reflectance*color[3]};

        if (f1 > 0) {
            refractedColor[2] = 0.0;
            reflectedColor[0] = reflectedColor[1] = 0.0;
            refractedColor[3] = reflectedColor[3] = color[3];
        }

        if (f2 > 0) { // Reflect Red, Transmit Green
            refractedColor[0] = refractedColor[2] = 0.0;
            reflectedColor[1] = reflectedColor[2] = 0.0;
            refractedColor[3] = reflectedColor[3] = color[3];
        }

        if ((reflectedColor[0] == 0.0) &&
            (reflectedColor[1] == 0.0) &&
            (reflectedColor[2] == 0.0))
            reflectedColor[3] = 0.0;

        if (refractedColor[3] > 0.01)
            this->cast (projected, refracted, refractedColor, projectedPointID, projectedPolyIDs);

        if (reflectedColor[3] > 0.01)
            this->cast (projected, reflected, reflectedColor, projectedPointID, projectedPolyIDs);
    }
}

void RayTracer::update ()
{
    // Add the points to the dataset
    if (beam_data->GetPoints())
        beam_data->GetPoints()->Delete();

    // Add the lines to the dataset
    if (beam_data->GetLines())
        beam_data->GetLines()->Delete();

    // Color the lines
    if (beam_data->GetCellData()->GetScalars())
        beam_data->GetCellData()->GetScalars()->Delete();

    beam_data->SetPoints (this->points);
    beam_data->SetLines (this->lines);
    beam_data->GetCellData()->SetScalars (this->colors);
}
