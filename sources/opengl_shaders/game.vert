#version 330
layout(location = 0) in vec2 model;

uniform mat3 modelMatrix;
uniform mat3 worldMatrix;
uniform mat3 projectionMatrix;

smooth out vec2 surfaceLocation;

void main()
{
    surfaceLocation = vec2(model.x, 1-model.y);

    vec3 localLoc = modelMatrix * vec3(model, 1);
    vec3 worldLoc = worldMatrix *  vec3(localLoc.xy, 1);
    vec3 cameraLoc = projectionMatrix * vec3(worldLoc.xy, 1);
    gl_Position = vec4(cameraLoc.xy, 0, 1);
    
}
