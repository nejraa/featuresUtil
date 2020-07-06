#version 300 es

// Set default precision to medium
precision mediump int;
precision mediump float;

in vec4 entityPos;
in vec4 entityCol;

flat out vec4 startPos;
out vec4 vertPos;

out vec4 col;

uniform mat4 entityMvp;

void main()
{
   col 			= entityCol;
   vec4 pos	 	= entityMvp * entityPos;
   gl_Position 	= pos;
   vertPos		= pos;
   startPos		= vertPos;
}

