# Trichroic Prism Visualization

This program is an interactive 3D visualization showing dispersion
of light by a trichroic prism (consisting of three prisms and two
polarizing filters). It's also a good example how to use VTK with
interactive widgets and shaders.

Trichroic prisms are used in 3-CMOS/3CCD cameras, which use three separate
image sensors, each one taking a separate measurement of the primary
colors: red, green, or blue light. Light coming into the lens is split
by a trichroic prism assembly, which directs the appropriate wavelength
ranges of light to their respective sensors. The system is employed by
still cameras, telecine systems, professional video cameras and some
prosumer video cameras.

A 3-chip system avoids the "Bayer filter" used on single chips, meaning
there is no interpolation (guessing) of color. This results in superior
color accuracy, better resolution and lower noise. It eliminates "moiré"
artifacts, which is critical for filming things like stadium turf or
finely patterned clothing.

It's still in use in broadcasting, where it's easier to pair large zoom
lens with smaller sensors. High-end commercial cinema projectors almost
exclusively use 3-chip DLP (Digital Light Processing) technology. In the
world of Cinema (ARRI, RED, Sony Venice), the industry has moved away
from prisms and toward Single-Chip Large Format sensors. 


## Screenshots

![](screenshots/1.png)
<br/>

![](screenshots/2.png)
<br/>

## Build and run

### From source code

**Requirements:**

* [VTK](https://vtk.org/) 9.x (Visualization Toolkit), discoverable by CMake (for example `vtk-devel` on Fedora, or the VTK module built by the Flatpak manifest below)
* [Meson](https://mesonbuild.com/) 1.3 or newer and Ninja

Install VTK development files so that CMake can find `VTKConfig.cmake`, then:

```sh
meson setup --reconfigure build --prefix=$PWD/.install
meson compile -C build
meson install -C build
.install/bin/prism
```

If CMake does not find VTK, point Meson at the install prefix, for example:

```sh
meson setup build --cmake-prefix-path=/path/to/vtk/prefix
```

### With Flatpak

You may build it inside a Flatpak container. Keep in mind that it'll require compiling VTK, which may take a lot of time.

Install `flatpak` and `flatpak-builder`, install the Freedesktop 24.08 SDK and runtime if needed, then run from the repository root:

```sh
flatpak-builder --user --install --force-clean _flatpak_build io.github.kamilprusko.Prism.json
flatpak run io.github.kamilprusko.Prism
```

The first build compiles VTK 9.4 and the application; later builds are incremental. Use `flatpak-builder --build-shell _flatpak_build io.github.kamilprusko.Prism.json` for an interactive shell inside the build environment.

