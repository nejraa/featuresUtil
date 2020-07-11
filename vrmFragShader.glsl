#version 300 es

// Set default precision to medium
precision mediump int;
precision mediump float;

in float dist;

in vec4 col;
out vec4 out_0;

uniform vec2 	u_resolution;
uniform float	u_dashSize;
uniform	float	u_gapSize;

void main()
{
        if (fract(dist / (u_dashSize + u_gapSize)) > u_dashSize/(u_dashSize + u_gapSize))
                discard;
    out_0 = col;
}

