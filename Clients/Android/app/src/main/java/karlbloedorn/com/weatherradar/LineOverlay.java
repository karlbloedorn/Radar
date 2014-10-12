package karlbloedorn.com.weatherradar;

import android.opengl.GLES20;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class LineOverlay {
    private int [] vbos = new int[1];
    public String description = "";
    public int lineCount;
    private ByteBuffer myBuffer;
    public boolean render = true;
    public boolean setup = false;

    public LineOverlay(InputStream filestream, String description){
        this.description = description;
        try {
            DataInputStream dis = new DataInputStream(filestream);
            dis.reset();
            lineCount = dis.readInt();
            int bytes = lineCount * 16;
            myBuffer = ByteBuffer.allocateDirect(bytes);
            byte[] lineData = new byte[bytes];
            BufferedInputStream bufferedFilestream = new BufferedInputStream(filestream);
            DataInputStream lineDataStream = new DataInputStream(bufferedFilestream);
            lineDataStream.readFully(lineData,0, bytes);
            myBuffer.position(0);
            myBuffer.put(lineData);
            myBuffer.position(0);
            lineDataStream.close();
            bufferedFilestream.close();
            filestream.close();
        }catch(IOException e){
            Log.e("Error", "IOException: " + e.getMessage());
        }
    }
    public void Setup()
    {
       setup = true;
       myBuffer.position(0);
       GLES20.glGenBuffers(1, vbos, 0);
       GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, vbos[0]);
       GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER,   this.lineCount*16 , myBuffer, GLES20.GL_STATIC_DRAW);
    }
    public void Draw(int program)
    {
        if(render){
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, vbos[0]);
            int positionHandle = GLES20.glGetAttribLocation(program, "position");
            GLES20.glVertexAttribPointer(positionHandle, 2, GLES20.GL_FLOAT, false, 0, 0);
            GLES20.glEnableVertexAttribArray(positionHandle);
            GLES20.glDrawArrays(GLES20.GL_LINES, 0, this.lineCount* 2);
        }
    }
}

