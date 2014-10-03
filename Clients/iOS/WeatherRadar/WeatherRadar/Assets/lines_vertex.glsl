attribute vec2 position;
varying lowp vec4 colorVarying;
uniform mat4 modelViewProjectionMatrix;

void main()
{
    colorVarying = vec4(1.0,1.0,1.0,1.0); // color;
    gl_Position = modelViewProjectionMatrix * vec4(position, 0.0, 1.0);
}