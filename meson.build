project(
  'prism',
  'cpp',
  version: '0.3',
  meson_version: '>=1.3.0',
  default_options: ['cpp_std=c++17'],
)

if get_option('vtk_kits')
  vtk_module_targets = [
    'VTK::Common',
    'VTK::Filters',
    'VTK::IO',
    'VTK::Interaction',
    'VTK::Rendering',
    'VTK::OpenGL',
  ]
else
  vtk_module_targets = [
    'VTK::CommonCore',
    'VTK::FiltersGeneral',
    'VTK::FiltersSources',
    'VTK::IOGeometry',
    'VTK::InteractionStyle',
    'VTK::InteractionWidgets',
    'VTK::RenderingAnnotation',
    'VTK::RenderingCore',
    'VTK::RenderingOpenGL2',
    'VTK::RenderingUI',
    'VTK::RenderingFreeType',
  ]
endif

vtk_dep = dependency(
  'VTK',
  method: 'cmake',
  components: [
    'CommonCore',
    'FiltersGeneral',
    'FiltersSources',
    'IOGeometry',
    'InteractionStyle',
    'InteractionWidgets',
    'RenderingAnnotation',
    'RenderingCore',
    'RenderingOpenGL2',
    'RenderingUI',
    'RenderingFreeType',
  ],
  modules: vtk_module_targets,
)

package_datadir = join_paths(get_option('prefix'), get_option('datadir'), 'prism')

conf_h = configuration_data()
conf_h.set_quoted('PACKAGE_NAME', meson.project_name())
conf_h.set_quoted('PACKAGE_DATADIR', package_datadir)

subdir('src')
subdir('data')
