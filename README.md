Prism
=====

This program is an interactive 3D visualization showing dispersion
of light by a trichroic prism (consisting of three prisms and two
polarizing filters). It's also a nice example how to use VTK with
interactive widgets and shaders.

Trichroic prisms are used in 3CCD cameras, which uses three separate
image sensors, each one taking a separate measurement of the primary
colors, red, green, or blue light. Light coming into the lens is split
by a trichroic prism assembly, which directs the appropriate wavelength
ranges of light to their respective sensors. The system is employed by
still cameras, telecine systems, professional video cameras and some
prosumer video cameras.

Compared to cameras with only one CCD, 3CCD cameras generally provide
superior image quality through enhanced resolution and lower noise. By
contrast, almost all single-CCD cameras use a Bayer filter, which allows
them to detect only one-third of the color information for each pixel.
The other two-thirds must be interpolated with a demosaicing algorithm
to 'fill in the gaps', resulting in a much lower effective resolution.


### Requirements

* [VTK](https://vtk.org/) 9.x (Visualization Toolkit), discoverable by CMake (for example `vtk-devel` on Fedora, or the VTK module built by the Flatpak manifest below)
* [Meson](https://mesonbuild.com/) 1.3 or newer and Ninja


### Build (Meson)

Install VTK development files so that CMake can find `VTKConfig.cmake`, then:

```sh
meson setup build
meson compile -C build
meson install -C build
```

If CMake does not find VTK, point Meson at the install prefix, for example:

```sh
meson setup build --cmake-prefix-path=/path/to/vtk/prefix
```


### Build and run with Flatpak (recommended for development)

Install `flatpak` and `flatpak-builder`, install the Freedesktop 24.08 SDK and runtime if needed, then from the repository root:

```sh
flatpak-builder --user --install --force-clean _flatpak_build io.github.kamilprusko.Prism.json
flatpak run io.github.kamilprusko.Prism
```

The first build compiles VTK 9.4 and the application; later builds are incremental. Use `flatpak-builder --build-shell _flatpak_build io.github.kamilprusko.Prism.json` for an interactive shell inside the build environment.

Optional: add `build/` and `_flatpak_build/` to `.gitignore` (already covered in this repository) and use [`.flatpakignore`](.flatpakignore) so large local directories are not copied into the Flatpak build.


### Run (installed with Meson)

```sh
prism
```

Data files are installed under `share/prism/` relative to the installation prefix.


### Screenshots

Click to view.

[![](http://kamilprusko.org/files/prism/screenshot-1-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-1.png)
[![](http://kamilprusko.org/files/prism/screenshot-2-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-2.png)
[![](http://kamilprusko.org/files/prism/screenshot-3-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-3.png)
[![](http://kamilprusko.org/files/prism/screenshot-4-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-4.png)
