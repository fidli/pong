#version 330
layout(location = 0) in vec2 model;

uniform vec2 position;
uniform vec2 scale;

smooth out vec2 surfaceLocation;

void main()
{
    surfaceLocation = model;
    gl_Position = vec4(scale.x*model.x + position.x, scale.y*model.y+position.y, 1, 1);
}
