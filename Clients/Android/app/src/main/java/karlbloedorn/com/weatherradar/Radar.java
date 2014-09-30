package karlbloedorn.com.weatherradar;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;

import com.crashlytics.android.Crashlytics;
import com.joanzapata.android.iconify.IconDrawable;
import com.joanzapata.android.iconify.Iconify;

import java.util.ArrayList;
import java.util.List;

public class Radar extends Activity {

    private RadarSurface surfaceView;
    private SharedPreferences preferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        preferences = getSharedPreferences("FilterLayers", Context.MODE_PRIVATE);
        super.onCreate(savedInstanceState);
        Crashlytics.start(this);
        setContentView(R.layout.activity_radar);

        surfaceView = (RadarSurface) findViewById(R.id.radar_surface);

        surfaceView.overlays.add(new LineOverlay(this.getResources().openRawResource(R.raw.state_lines), "States"));
        surfaceView.overlays.add(new LineOverlay(this.getResources().openRawResource(R.raw.county_lines), "Counties"));
        surfaceView.overlays.add(new LineOverlay(this.getResources().openRawResource(R.raw.interstate_lines), "Interstates"));

        for(LineOverlay overlay : surfaceView.overlays){
           overlay.render =  preferences.getBoolean(overlay.description, overlay.description == "Counties" ? false: true);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.radar, menu);
        menu.findItem(R.id.action_filter).setIcon( new IconDrawable(this, Iconify.IconValue.fa_filter).colorRes(android.R.color.white).actionBarSize());
        menu.findItem(R.id.action_alerts).setIcon( new IconDrawable(this, Iconify.IconValue.fa_bullhorn).colorRes(android.R.color.white).actionBarSize());
        menu.findItem(R.id.action_sliders).setIcon( new IconDrawable(this, Iconify.IconValue.fa_sliders).colorRes(android.R.color.white).actionBarSize());
        menu.findItem(R.id.action_share).setIcon( new IconDrawable(this, Iconify.IconValue.fa_share_alt).colorRes(android.R.color.white).actionBarSize());
        menu.findItem(R.id.action_location).setIcon( new IconDrawable(this, Iconify.IconValue.fa_location_arrow).colorRes(android.R.color.white).actionBarSize());

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_alerts) {

            return true;
        } else if (id == R.id.action_sliders) {

            return true;
        }else if (id == R.id.action_filter) {
            showLayerFilter();
            return true;
        }else if (id == R.id.action_location) {

            return true;
        }else if (id == R.id.action_share) {

            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    public void showLayerFilter(){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Filter layers").setPositiveButton("Done", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int id) {
                //  Your code when user clicked on OK
                //  You can write the code  to save the selected item here

            }
        });

        List<CharSequence> items =  new ArrayList<CharSequence>();
        boolean[] shownArray = new boolean[surfaceView.overlays.size()];

        int index = 0;
        for(LineOverlay lo : surfaceView.overlays){
            items.add(lo.description);
            shownArray[index] = lo.render;
            index++;
        }

        builder.setMultiChoiceItems(
                items.toArray(new CharSequence[items.size()]),
                shownArray,
                new DialogInterface.OnMultiChoiceClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int indexSelected,
                                boolean isChecked) {
                LineOverlay cur = surfaceView.overlays.get(indexSelected);
                cur.render = isChecked;
                SharedPreferences.Editor editor = preferences.edit();
                editor.putBoolean(cur.description, cur.render);
                editor.commit();
            }
        });
        builder.show();
    }


}
