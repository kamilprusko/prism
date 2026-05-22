configure_file(
  output: 'config.h',
  configuration: conf_h,
)

prism_sources = files(
  'main.cpp',
  'utils.cpp',
  'materials.cpp',
  'rayTracer.cpp',
  'prismActor.cpp',
  'beamActor.cpp',
  'labelActor.cpp',
  'torchMovedCallback.cpp',
  'scene.cpp',
  'torchWidget.cpp',
)

executable(
  'prism',
  prism_sources,
  dependencies: vtk_dep,
  install: true,
)
