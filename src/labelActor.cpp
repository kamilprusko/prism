/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "labelActor.h"

#include <vtkActor.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>

vtkStandardNewMacro(LabelActor);

LabelActor::LabelActor()
{
    vtkProperty *prop = this->QuadActor->GetProperty();
    prop->LightingOff();
    prop->SetAmbient(1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
}
