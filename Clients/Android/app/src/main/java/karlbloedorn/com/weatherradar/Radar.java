package karlbloedorn.com.weatherradar;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import com.crashlytics.android.Crashlytics;

public class Radar extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Crashlytics.start(this);
        setContentView(R.layout.activity_radar);



        /*
        InputStream is = getResources().openRawResource(
                getResources().getIdentifier("world",
                        "raw", getPackageName()));
        ValidationPreferences validationPrefs = new ValidationPreferences();
        validationPrefs.setMaxNumberOfPointsPerShape(999999);


        try {
            ShapeFileReader r = new ShapeFileReader(is, validationPrefs );
            ShapeFileHeader h = r.getHeader();

            int total = 0;
            AbstractShape s;
            while ((s = r.next()) != null) {

                switch (s.getShapeType()) {
                    case POINT:
                        PointShape aPoint = (PointShape) s;
                        // Do something with the point shape...
                        break;
                    case MULTIPOINT_Z:
                        MultiPointZShape aMultiPointZ = (MultiPointZShape) s;
                        // Do something with the MultiPointZ shape...
                        break;
                    case POLYGON:
                        PolygonShape aPolygon = (PolygonShape) s;
                        System.out.println("I read a Polygon with "
                                + aPolygon.getNumberOfParts() + " parts and "
                                + aPolygon.getNumberOfPoints() + " points");
                        for (int i = 0; i < aPolygon.getNumberOfParts(); i++) {
                            PointData[] points = aPolygon.getPointsOfPart(i);
                            System.out.println("- part " + i + " has " + points.length
                                    + " points.");
                        }
                        break;
                    default:
                        System.out.println("Read other type of shape.");
                }
                total++;
            }

            System.out.println("Total shapes read: " + total);

            is.close();




            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage("The shape type of this files is " + h.getShapeType()).setTitle("Test");
            AlertDialog dialog = builder.create();
            dialog.show();

        } catch (Exception e){
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage(e.getMessage()).setTitle("Fail");
            AlertDialog dialog = builder.create();
            dialog.show();
        }

    */
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.radar, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
