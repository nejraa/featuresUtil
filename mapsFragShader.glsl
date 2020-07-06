#version 300 es

// Set default precision to medium
precision mediump int;
precision mediump float;

in vec4 col;
out vec4 out_0;

flat in vec4 startPos;
in vec4 vertPos;

uniform vec2 	u_resolution;
uniform float	u_dashSize;
uniform	float	u_gapSize;
uniform	float	u_dotSize;
void main()
{
        vec2 dir	= (vertPos.xy - startPos.xy) * u_resolution/2.0;
        float dist	= length(dir);

        if (fract(dist / (u_dashSize + u_gapSize)) > u_dashSize/(u_dashSize + u_gapSize))
                discard;
        if ((u_dotSize!=0.0)&&(fract(dist / (u_dashSize + u_gapSize)) >0.05) && (fract(dist / (u_dashSize + u_gapSize)) < 0.15))
                discard;
    out_0 = col;
}

