#version 330
out vec4 color;

smooth in vec2 surfaceLocation;
smooth in vec3 windowPosition;

uniform ivec2 windowPx;
uniform vec4 overlayColor;
uniform sampler2D sampler;
uniform vec2 textureOffset;
uniform vec2 textureScale;

void main() {
    vec2 surfaceLocationFrom = surfaceLocation;
    vec2 textureLocation = textureOffset + vec2(textureScale.x * surfaceLocationFrom.x, textureScale.y * surfaceLocationFrom.y);
    vec4 texel = texture2D(sampler, textureLocation);
    
    float xDiff = ((windowPosition.x+1)/2)*windowPx.x - gl_FragCoord.x;
    float yDiff = ((windowPosition.y+1)/2)*windowPx.y - gl_FragCoord.y;
    
    color = overlayColor * overlayColor.a + texel * (texel.a-overlayColor.a);    
    color = overlayColor;
}
