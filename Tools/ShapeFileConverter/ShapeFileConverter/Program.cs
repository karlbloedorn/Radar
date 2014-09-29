using DotSpatial.Data;
using EndianHandling;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShapeFileConverter
{
  class Program
  {
    static void Main(string[] args)
    {
      Process(".shp");

    }

    static void Process(string filename)
    {
      int lineCount = 0;
      IFeatureSet fs = FeatureSet.Open(filename);
      var shapes = fs.ShapeIndices.ToList();
      foreach (var shape in shapes)
      {
        foreach (var part in shape.Parts)
        {
          lineCount += (part.NumVertices - 1);
        }
      }

      var fileStream = File.Create(filename + ".wrlineoverlay");
      var endianBinaryWriter = new EndianBinaryWriter(fileStream);

      endianBinaryWriter.Write(lineCount);

      float[] vertexArray = new float[lineCount * 2 * 3]; //2 points in each line  3 dim:xyz
      foreach (var shape in shapes)
      {
        foreach (var part in shape.Parts)
        {
          for (int i = 2; i < part.NumVertices; i++)
          {
            int baseIndex = (2 * part.StartIndex) + i * 2;
            endianBinaryWriter.Write( ConvertLongitude(fs.Vertex[baseIndex - 2]));  //long1
            endianBinaryWriter.Write(ConvertLatitude(fs.Vertex[baseIndex - 3])); //lat1
            endianBinaryWriter.Write(ConvertLongitude(fs.Vertex[baseIndex - 0])); //long2
            endianBinaryWriter.Write(ConvertLatitude(fs.Vertex[baseIndex - 1])); //lat2
          }
        }
      }
      endianBinaryWriter.Flush();
      fileStream.Close();
    }

    private static float ConvertLatitude(double latitude)
    {
      const int mapWidth = 360;
      const int mapHeight = 180;

      // convert from degrees to radians
      var latRad = latitude * Math.PI / 180;

      // get y value
      var mercN = Math.Log(Math.Tan((Math.PI / 4) + (latRad / 2)));
      var y = (mapHeight / 2) - (mapWidth * mercN / (2 * Math.PI));
      return (float)y;
    }

    private static float ConvertLongitude(double longitude)
    {
      const int mapWidth = 360;
      // get x value
      var x = (longitude + 180) * (mapWidth / 360.0);
      return (float)x;
    }



  }
}
