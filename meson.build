project(
  'prism',
  'cpp',
  version: '0.2',
  meson_version: '>=1.3.0',
  default_options: ['cpp_std=c++17'],
)

vtk_dep = dependency(
  'VTK',
  method: 'cmake',
  modules: [
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
)

package_datadir = join_paths(get_option('prefix'), get_option('datadir'), 'prism')

conf_h = configuration_data()
conf_h.set_quoted('PACKAGE_NAME', meson.project_name())
conf_h.set_quoted('PACKAGE_DATADIR', package_datadir)

conf_xml = configuration_data()
conf_xml.set('PACKAGE_DATADIR', package_datadir)


subdir('src')
subdir('data')
