package karlbloedorn.com.weatherradar;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import com.crashlytics.android.Crashlytics;
import com.joanzapata.android.iconify.IconDrawable;
import com.joanzapata.android.iconify.Iconify;

public class Radar extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Crashlytics.start(this);
        setContentView(R.layout.activity_radar);


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

        CharSequence[] items = {"States", "Counties", "Interstates"};
        boolean[] shownArray = {true, false, true};
        builder.setMultiChoiceItems(items,shownArray, new DialogInterface.OnMultiChoiceClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int indexSelected,
                                boolean isChecked) {
                /*if (isChecked) {
                    seletedItems.add(indexSelected);
                } else if (seletedItems.contains(indexSelected)) {
                    seletedItems.remove(Integer.valueOf(indexSelected));
                }*/
            }
        });
        builder.show();
    }


}
