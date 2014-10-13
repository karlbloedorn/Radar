package karlbloedorn.com.weatherradar;

import android.util.Log;
import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;

public class RadarOverlay {

    public RadarOverlay(InputStream filestream, String description) {
        Log.e("Test", "Started reading radar overlay.");
        try {
            DataInputStream dis = new DataInputStream(filestream);
            dis.reset();
            BufferedInputStream bufferedFilestream = new BufferedInputStream(filestream);
            DataInputStream radarDataStream = new DataInputStream(bufferedFilestream);
            byte[] magic = new byte[4];
            radarDataStream.readFully(magic,0,4);
            String magicString = new String(magic, "US-ASCII");
            int version = radarDataStream.readShort() & 0xFF;
            byte[] callsign = new byte[4];
            radarDataStream.readFully(callsign,0,4);
            String callsignString= new String(callsign, "US-ASCII");
            int op_mode = radarDataStream.readByte() & 0xF;
            int radar_status  = radarDataStream.readByte() & 0xF;
            long scan_type = radarDataStream.readInt() & 0xFFFF;
            int scan_date = radarDataStream.readInt(); // epoch seconds. this will overflow in 2038. fix before then.
            float latitude = radarDataStream.readFloat();
            float longitude = radarDataStream.readFloat();
            int number_of_radials = radarDataStream.readInt() & 0xFFFF;
            int number_of_bins = radarDataStream.readInt() & 0xFFFF;
            float first_bin_distance = radarDataStream.readFloat();
            float each_bin_distance= radarDataStream.readFloat();
            long crc32 = radarDataStream.readInt() & 0xFFFF;
            float[] azimuths = new float[number_of_radials];

            for (int i = 0; i < azimuths.length; i++){
                azimuths[i] = radarDataStream.readFloat();
            }
            int[][] data_array = new int[number_of_radials][number_of_bins];
            for(int i = 0; i < number_of_radials; i++){
                for(int j = 0; j < number_of_bins; j++){
                    data_array[i][j] = radarDataStream.readByte();
                }
            }
            radarDataStream.close();
            bufferedFilestream.close();
            filestream.close();
        }catch(IOException e){
            Log.e("Error", "IOException: " + e.getMessage());
        }
    }
}
