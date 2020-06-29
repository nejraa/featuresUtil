#version 300 es

// Set default precision to medium
precision mediump int;
precision mediump float;

in vec4 entityPos;
in vec4 entityCol;

in float inDist;

out vec4 col;
out float dist;

uniform mat4 entityMvp;

void main()
{
   col 			= entityCol;
   dist			= inDist;
   vec4 pos	 	= entityMvp * entityPos;
   gl_Position 	= pos;
}

