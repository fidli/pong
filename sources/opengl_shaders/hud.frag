#version 330
out vec4 color;

smooth in vec2 surfaceLocation;

uniform vec4 overlayColor;
uniform sampler2D sampler;
uniform vec2 textureOffset;
uniform vec2 textureScale;

void main() {
    vec2 surfaceLocationFrom = surfaceLocation;
    vec2 textureLocation = textureOffset + vec2(textureScale.x * surfaceLocationFrom.x, textureScale.y * surfaceLocationFrom.y);
    vec4 texel  = texture2D(sampler, textureLocation);
    
    color = vec4(texel.r, texel.g, texel.b, texel.a);

    color = overlayColor * overlayColor.a + texel * (texel.a-overlayColor.a);
}
