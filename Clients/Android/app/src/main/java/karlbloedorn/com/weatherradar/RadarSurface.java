package karlbloedorn.com.weatherradar;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.google.common.io.CharStreams;

public class RadarSurface extends GLSurfaceView {

    private ScaleGestureDetector scaleDetector;
    private GestureDetector panDetector;
    private RadarRenderer renderer;

    float mapScale = 60;
    float centerMapY = 54;
    float centerMapX = 78;

    public boolean onTouchEvent(MotionEvent ev) {
        scaleDetector.onTouchEvent(ev);
        panDetector.onTouchEvent(ev);
        return true;
    }

    private class PanListener extends GestureDetector.SimpleOnGestureListener {
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2,
                                float distanceX, float distanceY) {
            centerMapX +=( distanceX/mapScale);
            centerMapY += (distanceY/mapScale);
            queueUpdateViewMatrix();
           // Log.i("Test", "DistanceX: " + distanceX + " DistanceY:" + distanceY);
            return true;
        }
    }

    private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            float scale = detector.getScaleFactor();


           // GLU.gluUnProject()

            mapScale *= scale;
            queueUpdateViewMatrix();

            Log.i("Test", "Scale: " + scale);
            return true;
        }
    }


    public RadarSurface(Context context) {
        super(context);
        init(context);
    }
    public RadarSurface(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }
    private void init(Context context) {
        panDetector = new GestureDetector(context,new PanListener());
        scaleDetector = new ScaleGestureDetector(context,new ScaleListener());

        setEGLContextClientVersion(2);
        renderer = new RadarRenderer();

        InputStream fragmentStream =  context.getResources().openRawResource(R.raw.fragmenta);
        InputStream vertexStream =   context.getResources().openRawResource(R.raw.vertexa);
        try {
            renderer.vertexShaderCode = CharStreams.toString(new InputStreamReader(vertexStream));
            renderer.fragmentShaderCode = CharStreams.toString(new InputStreamReader(fragmentStream));
            fragmentStream.close();
            vertexStream.close();
        }catch(IOException e){

        }
        updateViewMatrix();
        setRenderer(renderer);
        int radarPositionsSize = 1377156*8;
        int radarColorSize = 1377156*4;
        final ByteBuffer myColorBuffer = ByteBuffer.allocateDirect(radarPositionsSize);
        final ByteBuffer myBuffer = ByteBuffer.allocateDirect(radarPositionsSize);
        InputStream radarStream = context.getResources().openRawResource(R.raw.output4);
        BufferedInputStream radarBufferedStream = new BufferedInputStream(radarStream);

        try {
            byte[] radarData = new byte[radarPositionsSize];
            byte[] radarColorData = new byte[radarColorSize];

            DataInputStream radarDataIS = new DataInputStream(radarBufferedStream);
            radarDataIS.readFully(radarData, 0,radarPositionsSize);
            myBuffer.position(0);
            myBuffer.put(radarData);
            myBuffer.position(0);
            radarDataIS.readFully(radarColorData, 0,radarColorSize);
            myColorBuffer.position(0);
            myColorBuffer.put(radarColorData);
            myColorBuffer.position(0);
            radarBufferedStream.close();
            radarStream.close();

            queueEvent(new Runnable() {
                public void run() {
                    renderer.loadRadarBuffer(myBuffer.order ( ByteOrder.BIG_ENDIAN ).asFloatBuffer(), myColorBuffer);
                }
            });

        }catch(IOException e){
            Log.e("Error", "IOException: " + e.getMessage());
        }
    }





    private void queueUpdateViewMatrix(){
        queueEvent(new Runnable(){
            public void run(){
                updateViewMatrix();
            }
        });
    }

    private void updateViewMatrix(){
        renderer.updateViewMatrix(mapScale, centerMapY, centerMapX);
    }
}
