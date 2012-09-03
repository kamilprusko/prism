/*
 * Fragment shader for chromatic aberration effect
 *
 * Copyright (c) 2003-2006  3Dlabs Inc. Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *     Neither the name of 3Dlabs Inc. Ltd. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *     Randi Rost
 *     Kamil Prusko
 */

// Reflections / Refraction
varying vec3  Reflect;
varying vec3  RefractR;
varying vec3  RefractG;
varying vec3  RefractB;
varying float Ratio;

uniform sampler2D Cubemap;

// Lighting
//varying vec3 normal;
//varying vec4 pos;

/*
vec4 lighting() {
	vec4 color = gl_FrontMaterial.diffuse;
	vec4 matspec = gl_FrontMaterial.specular;
	float shininess = gl_FrontMaterial.shininess;
	vec4 lightspec = gl_LightSource[0].specular;
	vec4 lpos = gl_LightSource[0].position;
	vec4 s = -normalize(pos-lpos);

	vec3 light = s.xyz;
	vec3 n = normalize(normal);
	vec3 r = -reflect(light, n);
	r = normalize(r);
	vec3 v = -pos.xyz;
	v = normalize(v);

	vec4 diffuse  = color * max(0.0, dot(n, s.xyz)) * gl_LightSource[0].diffuse;
	vec4 specular;
	if (shininess != 0.0) {
		specular = lightspec * matspec * pow(max(0.0, dot(r, v)), shininess);
	} else {
		specular = vec4(0.0, 0.0, 0.0, 0.0);
	}

	return diffuse + specular;
	//return DiffuseColor;
}
*/
//varying vec4 DiffuseColor;

varying vec3 normal, lightDir, eyeVec;

vec4 phong() {
	vec4 final_color = vec4(0.0, 0.0, 0.0, 0.0);
//		(gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) +
//		(gl_LightSource[0].ambient * gl_FrontMaterial.ambient);

	vec3 N = normalize(normal);
	vec3 L = normalize(lightDir);

	float lambertTerm = 1.0 - abs(dot(N,L));

	if(lambertTerm > 0.0)
	{
//		final_color += gl_LightSource[0].diffuse *
//		               gl_FrontMaterial.diffuse *
//					   lambertTerm;

		final_color += gl_LightSource[0].diffuse *
//		               vec4(0.0, 0.6, 1.0, 1.0) *
		               vec4(1.0, 1.0, 1.0, 0.0) *
					   pow(lambertTerm, 5.0) * 0.4;

		vec3 E = normalize(eyeVec);
		vec3 R = reflect(-L, N);
		float specular = pow( max(dot(R, E), 0.0),
		                 gl_FrontMaterial.shininess );

		final_color += gl_LightSource[0].specular *
		               gl_FrontMaterial.specular *
					   specular;

		final_color.a = 0.0;
	}
	return final_color;
}


void main()
{
    vec3 refractColor, reflectColor;

    refractColor.r = vec3(texture2D(Cubemap, RefractR)).r;
    refractColor.g = vec3(texture2D(Cubemap, RefractG)).g;
    refractColor.b = vec3(texture2D(Cubemap, RefractB)).b;

    reflectColor   = vec3(texture2D(Cubemap, Reflect));

    vec3 color     = mix(refractColor, reflectColor, Ratio);

//    gl_FragColor   = vec4(color, 1.0);
//	gl_FragColor   = vec4(color, (color.r + color.g + color.b)) + phong();

	gl_FragColor   = vec4(pow(color,2.0)*4.0, (color.r + color.g + color.b)*0.5) + phong()*5.0;


//	gl_FragColor   = vec4(color, (color.r + color.g + color.b)/2.0); // + phong();
//	gl_FragColor   = vec4(vec3(1.0,1.0,1.0), (color.r + color.g + color.b)/2.0); // + phong();
}
