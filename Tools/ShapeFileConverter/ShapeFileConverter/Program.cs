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
      Process("C:/Users/Karl/Downloads/in15oc03/in101503.shp");
    }
    static void Process(string filename)
    {
      int floatsSent = 0;
      int lineCount = 0;
      IFeatureSet fs = FeatureSet.Open(filename);
      var shapes = fs.ShapeIndices.ToList();
      foreach (var shape in shapes)
      {
        foreach (var part in shape.Parts)
        {
          lineCount += part.Segments.Count();
        }
      }
      var fileStream = File.Create(filename + ".wrlineoverlay");
      EndianBinaryWriter endianBinaryWriter = new EndianBinaryWriter(fileStream, Encoding.ASCII, true);
      endianBinaryWriter.Write(lineCount);
      endianBinaryWriter.Flush();
      BinaryWriter binaryWriter = new BinaryWriter(fileStream);
      foreach (var shape in shapes)
      {
        foreach (var part in shape.Parts)
        {
          foreach(var s in part.Segments)
          {
            binaryWriter.Write(ConvertLongitude(s.P1.X));  //long1
            binaryWriter.Write(ConvertLatitude(s.P1.Y)); //lat1
            binaryWriter.Write(ConvertLongitude(s.P2.X)); //long2
            binaryWriter.Write(ConvertLatitude(s.P2.Y)); //lat2
            floatsSent += 4;
          }
  
        }
      }
      binaryWriter.Flush();
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
