#version 330
out vec4 color;

uniform vec4 overlayColor;

void main() {
	color = overlayColor*overlayColor.a + vec4(1, 1, 1, 1) * (1-overlayColor.a);
}
