configure_file(
  output: 'config.h',
  configuration: conf_h,
)

prism_sources = files(
  'main.cpp',
  'ray-tracer.cpp',
  'torch.cpp',
)

executable(
  'prism',
  prism_sources,
  dependencies: vtk_dep,
  install: true,
)
