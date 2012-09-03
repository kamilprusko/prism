varying vec4 color;  // gl_Color from vertex shader
varying vec3 normal; // world normal vector
varying vec3 eye;    // world view vector

void main ()
{
	gl_FragColor = vec4 (color.r, color.g, color.b, color.a);
}
