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

#include <vtkCommand.h>
#include <vtkLineWidget.h>
#include <vtkMath.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

class RayTracer;
class vtkRenderWindowInteractor;

class TorchMovedCallback : public vtkCommand
{
private:
    RayTracer                    *raytracer = nullptr;
    vtkTransformPolyDataFilter   *torch_transform = nullptr;
    vtkLineWidget                *line_widget = nullptr;
    bool                          widget_dragging = false;

public:
    void set_raytracer (RayTracer *raytracer);
    void set_torch_transform (vtkTransformPolyDataFilter *torch_transform);
    /** Arm Start/EndInteraction observers for drag tracking (see TorchInteractorStyle). */
    void attach_for_live_drag (vtkLineWidget *widget, vtkRenderWindowInteractor *iren);
    void Execute (vtkObject *caller, unsigned long, void*) override;

    bool is_widget_dragging () const { return widget_dragging; }
    /** Raytrace + torch pose from widget endpoints (call after vtkLineWidget handled MouseMove). */
    void sync_torch_from_widget (vtkLineWidget *widget);

private:
    void sync_from_widget (vtkLineWidget *widget);
    void set_transform_direction (vtkTransform *transform, const double  direction[3]);

    static void OnWidgetStart (vtkObject *, unsigned long, void *clientdata, void *);
    static void OnWidgetEnd (vtkObject *, unsigned long, void *clientdata, void *);
};
