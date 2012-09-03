varying vec4 color;  // gl_Color from vertex shader
varying vec3 normal; // world normal vector
varying vec3 eye;    // world view vector

void main ()
{
    vec3 view_vertex;

    color = gl_Color;
    normal = gl_NormalMatrix * gl_Normal;

    view_vertex = vec3 (gl_ModelViewMatrix * gl_Vertex);
    eye         = - view_vertex;

    gl_Position = ftransform ();
}
