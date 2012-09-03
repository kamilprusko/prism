/*
 * Vertex shader for chromatic aberration effect
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

const float EtaR = 0.65;
const float EtaG = 0.67;        // Ratio of indices of refraction
const float EtaB = 0.69;
const float FresnelPower = 1.0;

const float F  = ((1.0-EtaG) * (1.0-EtaG)) / ((1.0+EtaG) * (1.0+EtaG));

varying vec3  Reflect;
varying vec3  RefractR;
varying vec3  RefractG;
varying vec3  RefractB;
varying float Ratio;

// Lighting
/*
varying vec3 normal;
varying vec4 pos;
varying vec4 rawpos;

void lighting() {
	normal = gl_NormalMatrix * gl_Normal;
	pos = gl_ModelViewMatrix * gl_Vertex;
	rawpos = gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
*/

//uniform vec3 LightPosition;
//uniform vec3 EyePosition;

/*
uniform vec3 LightPosition;
uniform vec3 EyePosition;
varying vec4 DiffuseColor;  // GLSLdemo requires vertex/fragment shader pair

    vec3 ecPosition = vec3(gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - ecPosition);
    float costheta  = dot(tnorm, lightVec);
    float cosalpha  = cross(EyePosition, lightVec);
    float a         = 1.0 - abs(cosalpha);
	a = a * 0.7;
    DiffuseColor    = max(vec4(a, 0.0, 0.0, a), vec4(0.0, 0.0, 0.0, 0.0));
*/

varying vec3 normal, lightDir, eyeVec;

void phong(){
	normal = gl_NormalMatrix * gl_Normal;

	vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);

	lightDir = vec3(gl_LightSource[0].position.xyz - vVertex);
	eyeVec = -vVertex;
}

void main()
{
    vec4 ecPosition  = gl_ModelViewMatrix * gl_Vertex;
    vec3 ecPosition3 = ecPosition.xyz / ecPosition.w;

    vec3 i = normalize(ecPosition3);
    vec3 n = normalize(gl_NormalMatrix * gl_Normal);

    Ratio   = F + (1.0 - F) * pow((1.0 - dot(-i, n)), FresnelPower);

    RefractR = refract(i, n, EtaR);
    RefractR = vec3(gl_TextureMatrix[0] * vec4(RefractR, 1.0));

    RefractG = refract(i, n, EtaG);
    RefractG = vec3(gl_TextureMatrix[0] * vec4(RefractG, 1.0));

    RefractB = refract(i, n, EtaB);
    RefractB = vec3(gl_TextureMatrix[0] * vec4(RefractB, 1.0));

    Reflect  = reflect(i, n);
    Reflect  = vec3(gl_TextureMatrix[0] * vec4(Reflect, 1.0));

//	lighting();
	phong();

    gl_Position = ftransform();
}
