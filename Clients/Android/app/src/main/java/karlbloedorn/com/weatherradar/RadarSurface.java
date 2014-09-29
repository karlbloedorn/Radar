package karlbloedorn.com.weatherradar;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import com.google.common.io.CharStreams;

public class RadarSurface extends GLSurfaceView {

    private ScaleGestureDetector scaleDetector;
    private GestureDetector panDetector;
    private RadarRenderer renderer;

    float mapScale = 60;
    float centerMapY = 54;
    float centerMapX = 78;

    private void init(Context context) {
        setEGLContextClientVersion(2);
        renderer = new RadarRenderer();
        renderer.overlays.add(new LineOverlay(context.getResources().openRawResource(R.raw.state_lines), "States"));
        renderer.overlays.add(new LineOverlay(context.getResources().openRawResource(R.raw.county_lines), "Counties"));
        renderer.overlays.add(new LineOverlay(context.getResources().openRawResource(R.raw.interstate_lines), "Interstates"));

        //renderer.overlays.add(new LineOverlay(context.getResources().openRawResource(R.raw.overlay_interstates), "Interstates"));
        panDetector = new GestureDetector(context,new PanListener());
        scaleDetector = new ScaleGestureDetector(context,new ScaleListener());

        InputStream fragmentStream =  context.getResources().openRawResource(R.raw.fragmenta);
        InputStream vertexStream =   context.getResources().openRawResource(R.raw.vertexa);
        try {
            renderer.vertexShaderCode = CharStreams.toString(new InputStreamReader(vertexStream));
            renderer.fragmentShaderCode = CharStreams.toString(new InputStreamReader(fragmentStream));
            fragmentStream.close();
            vertexStream.close();
        }catch(IOException e){
            Log.e("WeatherRadar", "Error loading vertex and shader" + e.getMessage());
        }
        updateViewMatrix();
        setRenderer(renderer);
    }

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
