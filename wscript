#! /usr/bin/env python
# encoding: utf-8

VERSION='0.2'
APPNAME='prism'

top = '.'
out = 'build'

vtk_libs = (
        'vtkCommon',
        'vtkFiltering',
        'vtkImaging',
        'vtkGraphics',
        'vtkGenericFiltering',
        'vtkIO',
        'vtkRendering',
        'vtkVolumeRendering',
        'vtkHybrid',
        'vtkWidgets',
        'vtkParallel',
        'vtkInfovis',
        'vtkTextAnalysis',
        'vtkGeovis',
        'vtkViews',
        'vtkCharts')

import os

def options(opt):
    opt.tool_options('compiler_cxx')
    opt.tool_options('intltool')


def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('intltool')

    conf.load('compiler_cxx')
    conf.load('intltool')

    conf.env.append_value('LINKFLAGS', '-L/usr/lib64/vtk')
    conf.env.append_value('LINKFLAGS', '-L/usr/lib/vtk')
    conf.env.append_value('CXXFLAGS', '-I/usr/include/vtk')
    conf.env.append_value('CXXFLAGS', '-I'+conf.bldnode.abspath())

    for lib in vtk_libs:
        conf.check_cxx(lib=lib, uselib_store=lib, mandatory=True)

    conf.env['PACKAGE_NAME'] = APPNAME
    conf.env['PACKAGE_DATADIR'] = os.path.join(_unquote(conf.get_define('DATADIR')), APPNAME)

    conf.define('PACKAGE_NAME', conf.env['PACKAGE_NAME'])
    conf.define('PACKAGE_DATADIR', conf.env['PACKAGE_DATADIR'])

    conf.write_config_header('config.h')


def build(bld):
    bld.add_subdirs('data')
    bld.add_subdirs('src')


def _unquote(str):
    return str.replace('"', '')
