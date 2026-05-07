###VERTEX_POSITIONVC_DEC###
###VERTEX_POSITIONVC_IMPL###
###FRAGMENT_LIGHT_DEC###
###FRAGMENT_LIGHT_IMPL###
  {
    /* -----------------------------------------------------------------------
       Glass shader — no ray tracing, no texture, no extra uniforms.
       Techniques: Fresnel (Schlick), procedural sky/ground gradient,
       chromatic dispersion, multi-lobe specular, edge brightening,
       interior depth cue.
       ----------------------------------------------------------------------- */

    vec3  N    = normalize(normalVCVSOutput);
    vec3  V    = normalize(-vertexVC.xyz);
    float cosT = abs(dot(N, V));

    /* --- Fresnel (Schlick) ------------------------------------------------- */
    float F0         = 0.04;                               /* IOR ~1.5, glass  */
    float fresnel    = F0 + (1.0 - F0) * pow(1.0 - cosT, 5.0);
    float silhouette = 1.0 - cosT;

    /* --- Reflection vector (view space) ------------------------------------ */
    /* V is view-space, which is fine here: the gradient is evaluated from R  */
    /* so it moves naturally as the camera orbits — R.y changes because the   */
    /* face normals rotate relative to V when you move the camera.            */
    vec3  R      = reflect(-V, N);

    /* --- Procedural sky / ground gradient ---------------------------------- */
    /* R.y: +1 = straight up (sky), -1 = straight down (ground).             */
    float skyT   = clamp(R.y * 0.5 + 0.5, 0.0, 1.0);

    /* Tune these three colours to match your scene's ambient mood.           */
    vec3 skyColor  = vec3(1.0, 1.0, 1.0);  /* sky color                  */
    vec3 groundColor  = vec3(0.0, 0.0, 0.0);  /* ground floor              */

    /* Two-stop gradient: horizon as pivot.                                   */
    vec3 env = mix(groundColor, skyColor, skyT * 2.0);

    /* --- Base glass colour ------------------------------------------------- */
    /* Face-on (fresnel~0): mostly glass tint. Glancing (fresnel~1): env.     */
    vec3 baseColor = vec3(0.1, 0.1, 0.1);
    vec3 base      = mix(baseColor, env, 0.2 + fresnel * 1.0);

    /* --- Interior depth cue ------------------------------------------------ */
    base *= (1.0 - 0.18 * cosT);

    /* --- Edge brightening -------------------------------------------------- */
    vec3 edge = vec3(0.9, 0.9, 0.9) * pow(silhouette, 4.0) * 0.55;

    /* --- Multi-lobe specular ----------------------------------------------- */
    /* Lobe A: tight primary key highlight.                                   */
    vec3  L     = normalize(vec3(0.5, 1.0, 0.7));
    vec3  H     = normalize(L + V);
    float specA = pow(max(dot(N, H), 0.0), 180.0);

    /* Lobe B: softer fill from opposite side.                                */
    vec3  L2    = normalize(vec3(-0.4, 0.6, 0.5));
    vec3  H2    = normalize(L2 + V);
    float specB = pow(max(dot(N, H2), 0.0), 90.0) * 0.35;

    /* Lobe C: retroreflective rim streak at grazing angles.                  */
    float specC = pow(silhouette, 6.0) * fresnel * 0.60;

    vec3 sharpSpec = vec3(1.00, 1.00, 1.02) * (specA + specB) * 3.0
                   + vec3(0.92, 0.92, 0.92) * specC;

    vec3 vtkSpec = specular * 0.15;

    /* --- Composite --------------------------------------------------------- */
    vec3  col   = base + edge + vtkSpec + sharpSpec;
    float alpha = clamp(0.36 + 0.10 * silhouette + 0.06 * fresnel, 0.34, 0.54);

    gl_FragData[0] = vec4(col, alpha);
  }
###END###