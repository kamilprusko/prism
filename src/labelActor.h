/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_LABEL_ACTOR_H
#define PRISM_LABEL_ACTOR_H

#include <vtkBillboardTextActor3D.h>
#include <vtkType.h>

/** vtkBillboardTextActor3D with scene lighting disabled on the text quad (uniform color). */
class LabelActor : public vtkBillboardTextActor3D
{
  public:
    static LabelActor *New();
    vtkTypeMacro(LabelActor, vtkBillboardTextActor3D);

  protected:
    LabelActor();
    ~LabelActor() override = default;
};

#endif
