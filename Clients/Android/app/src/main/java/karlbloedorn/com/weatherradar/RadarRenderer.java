package karlbloedorn.com.weatherradar;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class RadarRenderer implements GLSurfaceView.Renderer {
    public ArrayList<LineOverlay> overlays = new ArrayList<LineOverlay>();

    private float[] viewMatrix = new float[16];
    private float[] projectionMatrix = new float[16];
    private float[] modelViewProjectionMatrix = new float[16];
    private int mProgram;
    private int modelViewProjectionMatrixHandle;
    private int positionHandle;
    private int colorHandle;
    public String vertexShaderCode;
    public String fragmentShaderCode;

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig eglConfig) {
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
        mProgram = GLES20.glCreateProgram();             // create empty OpenGL ES Program
        GLES20.glAttachShader(mProgram, vertexShader);   // add the vertex fragment to program
        GLES20.glAttachShader(mProgram, fragmentShader); // add the fragment fragment to program
        GLES20.glLinkProgram(mProgram);
        modelViewProjectionMatrixHandle = GLES20.glGetUniformLocation(mProgram, "modelViewProjectionMatrix");
        positionHandle = GLES20.glGetAttribLocation(mProgram, "position");
       // colorHandle = GLES20.glGetAttribLocation(mProgram, "color");
        GLES20.glEnableVertexAttribArray(positionHandle);
        //GLES20.glEnableVertexAttribArray(colorHandle);

        for(LineOverlay overlay : overlays){
                overlay.Setup();
        }
    }
    @Override
    public void onSurfaceChanged(GL10 unused, int width, int height) {
        float top = -height/ 2.0f;
        float bottom = height / 2.0f;
        float left = -width/ 2.0f;
        float right = width / 2.0f;
        Matrix.orthoM(projectionMatrix,0,left,right, bottom, top, -10f,10f);
        GLES20.glViewport(0, 0, width, height);
    }
    @Override
    public void onDrawFrame(GL10 unused) {
        GLES20.glUseProgram ( mProgram );
        GLES20.glClear(GL10.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
        GLES20.glUniformMatrix4fv(modelViewProjectionMatrixHandle, 1, false, modelViewProjectionMatrix, 0);

        //GLES20.glUniform4f();
        for(LineOverlay overlay : overlays){
            overlay.Draw(mProgram);
        }

        int error;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.e("MyApp", ": glError " + error);
        }
    }
    public static int loadShader(int type, String shaderCode){
        int shader = GLES20.glCreateShader(type);
        if (shader != 0) {
            GLES20.glShaderSource(shader, shaderCode);
            GLES20.glCompileShader(shader);
            int[] compiled = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
            if (compiled[0] == 0) {
                Log.e("", "Could not compile fragment " + type + ":");
                Log.e("", GLES20.glGetShaderInfoLog(shader));
                GLES20.glDeleteShader(shader);
                shader = 0;
            }
        }
        return shader;
    }

    void updateViewMatrix(float mapScale, float centerMapY, float centerMapX){
        float[] translateMatrix = new float[16];
        Matrix.setIdentityM(translateMatrix,0);
        Matrix.translateM(translateMatrix,0, -centerMapX, -centerMapY, 0.0f);
        float[] scaleMatrix = new float[16];
        Matrix.setIdentityM(scaleMatrix, 0);
        Matrix.scaleM(scaleMatrix, 0, mapScale, mapScale, 1.0f);
        Matrix.multiplyMM(viewMatrix, 0, scaleMatrix, 0, translateMatrix, 0);
    }
}
