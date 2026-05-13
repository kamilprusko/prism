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

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkRenderWindowInteractor.h>


void TorchMovedCallback::set_raytracer (RayTracer *raytracer)
{
    this->raytracer = raytracer;
}

void TorchMovedCallback::set_torch_transform (vtkTransformPolyDataFilter *torch_transform)
{
    this->torch_transform = torch_transform;
}

void TorchMovedCallback::attach_for_live_drag (vtkLineWidget             *widget,
                                               vtkRenderWindowInteractor *iren)
{
    (void)iren;
    this->line_widget = widget;

    vtkCallbackCommand *start = vtkCallbackCommand::New ();
    start->SetClientData (this);
    start->SetCallback (TorchMovedCallback::OnWidgetStart);
    widget->AddObserver (vtkCommand::StartInteractionEvent, start);
    start->Delete ();

    vtkCallbackCommand *end = vtkCallbackCommand::New ();
    end->SetClientData (this);
    end->SetCallback (TorchMovedCallback::OnWidgetEnd);
    widget->AddObserver (vtkCommand::EndInteractionEvent, end);
    end->Delete ();
}

void TorchMovedCallback::OnWidgetStart (vtkObject *,
                                        unsigned long,
                                        void *clientdata,
                                        void *)
{
    static_cast<TorchMovedCallback *>(clientdata)->widget_dragging = true;
}

void TorchMovedCallback::OnWidgetEnd (vtkObject *,
                                     unsigned long,
                                     void *clientdata,
                                     void *)
{
    TorchMovedCallback *self = static_cast<TorchMovedCallback *>(clientdata);
    self->widget_dragging = false;
    if (self->line_widget)
    {
        self->sync_from_widget (self->line_widget);
    }
}

void TorchMovedCallback::Execute (vtkObject *caller,
                                  unsigned long,
                                  void*)
{
    vtkLineWidget *widget = vtkLineWidget::SafeDownCast (caller);
    if (!widget)
    {
        return;
    }
    this->sync_from_widget (widget);
}

void TorchMovedCallback::sync_torch_from_widget (vtkLineWidget *widget)
{
    this->sync_from_widget (widget);
}

void TorchMovedCallback::sync_from_widget (vtkLineWidget *widget)
{
    if (!widget)
    {
        return;
    }

    vtkTransform *transform = vtkTransform::New ();

    const double *a = widget->GetPoint1 ();
    const double *b = widget->GetPoint2 ();
    double        direction[3];
    double        orgin[3];
    double        color[4] = {1.0, 1.0, 1.0, 0.8};

    vtkMath::Subtract (b, a, direction);
    vtkMath::Normalize (direction);
    vtkMath::MultiplyScalar (direction, 15.0);
    vtkMath::Subtract (a, direction, orgin);

    transform->Identity ();
    transform->PostMultiply ();
    transform->RotateZ (90.0);

    this->set_transform_direction (transform, direction);
    transform->Translate (orgin[0], orgin[1], orgin[2]);

    if (this->torch_transform)
    {
        this->torch_transform->SetTransform (transform);
        this->torch_transform->Update ();
    }

    if (this->raytracer)
    {
        this->raytracer->render (orgin, direction, color);
    }

    transform->Delete ();

    vtkRenderWindowInteractor *iren = widget->GetInteractor ();
    if (iren)
    {
        iren->Render ();
    }
}

void TorchMovedCallback::set_transform_direction (vtkTransform *transform,
                                                 const double  direction[3])
{
    double direction_normalized[3] = { direction[0], direction[1], direction[2] };
    vtkMath::Normalize (direction_normalized);

    double x[3] = {1.0, 0.0, 0.0};
    const double c = vtkMath::Dot (x, direction_normalized);

    if (c > 1.0 - 1.0e-7)
    {
        return;
    }
    if (c < -1.0 + 1.0e-7)
    {
        transform->RotateWXYZ (180.0, 0.0, 0.0, 1.0);
        return;
    }

    double axis[3];
    vtkMath::Cross (x, direction_normalized, axis);
    const double theta = acos (c);
    transform->RotateWXYZ (theta * 180.0 / vtkMath::Pi (), axis[0], axis[1], axis[2]);
}
