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

* [VTK](http://vtk.org/) (Visualization ToolKit)
* [Waf](http://code.google.com/p/waf/) 1.6.x installed system-wide


### Build or Install

	$ waf configure --prefix=./install
	$ waf build install

### Run

    $ ./install/bin/prism

### Screenshots

Click to view.

[![](http://kamilprusko.org/files/prism/screenshot-1-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-1.png)
[![](http://kamilprusko.org/files/prism/screenshot-2-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-2.png)
[![](http://kamilprusko.org/files/prism/screenshot-3-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-3.png)
[![](http://kamilprusko.org/files/prism/screenshot-4-thumbnail.png)](http://kamilprusko.org/files/prism/screenshot-4.png)

