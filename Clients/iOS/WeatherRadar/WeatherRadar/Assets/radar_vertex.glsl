attribute vec2 position;
attribute vec4 color;

varying lowp vec4 colorVarying;
uniform mat4 modelViewProjectionMatrix;

void main()
{
    colorVarying = color;
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0);
}