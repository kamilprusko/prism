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
#include "torch.h"


void TorchMovedCallback::set_raytracer (RayTracer *raytracer)
{
    this->raytracer = raytracer;
}

void TorchMovedCallback::set_torch_transform (vtkTransformPolyDataFilter *torch_transform)
{
    this->torch_transform = torch_transform;
}

void TorchMovedCallback::Execute (vtkObject *caller,
                                  unsigned long,
                                  void*)
{
    vtkTransform  *transform = vtkTransform::New();
    vtkLineWidget *widget    = reinterpret_cast<vtkLineWidget*>(caller);

    double *a = widget->GetPoint1();
    double *b = widget->GetPoint2();
    double direction[3];
    double orgin[3];
    double color[4] = {1.0, 1.0, 1.0, 0.8};

    vtkMath::Subtract (b, a, direction);
    vtkMath::Normalize (direction);
    vtkMath::MultiplyScalar (direction, 15.0);
    vtkMath::Subtract (a, direction, orgin);

    // Set torch position
    transform->Identity();
    transform->PostMultiply();
    transform->RotateZ (90.0);

    this->set_transform_direction (transform, direction);
    transform->Translate (orgin[0], orgin[1], orgin[2]);

    // Follow Torch in given direction
    if (this->torch_transform)
        this->torch_transform->SetTransform (transform);

    if (this->raytracer)
        this->raytracer->render (orgin, direction, color);

    transform->Delete();
}

/** Face transformation against passed direction
 */
void TorchMovedCallback::set_transform_direction (vtkTransform *transform,
                                                  const double  direction[3])
{
    double direction_normalized[3] = { direction[0], direction[1], direction[2] };
    vtkMath::Normalize (direction_normalized);

    double x[3]    = {1.0, 0.0, 0.0};
    double axis[3] = {1.0, 0.0, 0.0};
    double theta   = acos (vtkMath::Dot (x, direction_normalized));

    vtkMath::Cross (x, direction_normalized, axis);
    transform->RotateWXYZ (theta * 180.0 / vtkMath::Pi(), axis[0], axis[1], axis[2]);
}
