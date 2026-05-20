/*
 * Copyright (c) 2010,2026  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkTexture.h>

#include "config.h"
#include "utils.h"

namespace {

std::unordered_map<std::string, std::string> ParseShader(const std::string &contents)
{
    std::unordered_map<std::string, std::string> out;
    std::istringstream stream(contents);
    std::string line;
    std::string curKey;
    std::ostringstream curBody;

    while (std::getline(stream, line)) {
        if (line.size() >= 6 && line.compare(0, 3, "###") == 0 &&
            line.compare(line.size() - 3, 3, "###") == 0) {

            if (!curKey.empty()) {
                out[curKey] = curBody.str();
                curBody.str("");
                curBody.clear();
            }

            curKey = line.substr(3, line.size() - 6);
            if (curKey == "END")
                curKey.clear();

            continue;
        }

        curBody << line << '\n';
    }

    if (!curKey.empty())
        out[curKey] = curBody.str();

    return out;
}

} // namespace

void LoadObject(vtkActor *actor, const char *filename)
{
    const std::string path = std::string(PACKAGE_DATADIR) + "/objects/" + filename;

    vtkNew<vtkOBJReader> reader;
    reader->SetFileName(path.c_str());
    reader->Update();

    vtkNew<vtkOpenGLPolyDataMapper> mapper;
    mapper->SetInputData(reader->GetOutput());
    mapper->Update();
    actor->SetMapper(mapper);
}

std::unordered_map<std::string, std::string> LoadShader(const char *filename)
{
    const std::string path = std::string(PACKAGE_DATADIR) + "/shaders/" + filename;
    std::ifstream in(path);
    if (!in)
        std::abort();

    std::ostringstream buf;
    buf << in.rdbuf();

    return ParseShader(buf.str());
}

void LoadTexture(vtkActor *actor, const char *filename)
{
    const std::string path = std::string(PACKAGE_DATADIR) + "/textures/" + filename;

    vtkNew<vtkJPEGReader> image;
    image->SetFileName(path.c_str());
    image->Update();

    vtkNew<vtkTexture> tex;
    tex->SetInputConnection(image->GetOutputPort());
    tex->InterpolateOn();
    actor->SetTexture(tex);
}
