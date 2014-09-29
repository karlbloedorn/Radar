using System;
using System.IO;
using System.Linq;
using System.Text;

// Source: http://www.brendanbaker.com/alias/endian-binary-reader-writer

namespace EndianHandling
{
    public class EndianBinaryWriter
    {
        /* ===[ Notes ]====================================================== */

        /* If there is a discrepency between the system's endianness and the 
         * requested operation, reverse the bytes where needed. In cases where
         * the system's endianness is the requested mode, take no action. The
         * following table illustrates when byte reversals are made: 
         * 
         * +-----------------------------+-------------+---------+
         * | BitConverter.IsLittleEndian | this.bigEnd | Action  |
         * +-----------------------------+-------------+---------+
         * | True                        | True        | Reverse |
         * | True                        | False       | None    |
         * | False                       | True        | None    |
         * | False                       | False       | Reverse |
         * +-----------------------------+-------------+---------+
         *
         * Therefore, use XOR logic to determine whether to reverse bytes.
         *
         */

        /* ===[ Constants ]================================================== */

        internal const string DEFAULT_ENCODING   = "UTF-8";
        internal const bool   DEFAULT_BIG_ENDIAN = false;

        /* ===[ Fields ]===================================================== */
        
        protected Stream       output;
        protected Encoding     encoding;
        protected BinaryWriter writer;
        protected bool         bigEnd;

        /* ===[ Constructors ]=============================================== */

        public EndianBinaryWriter(Stream output)
            : this(output, Encoding.GetEncoding(DEFAULT_ENCODING)) { }

        public EndianBinaryWriter(Stream output, Encoding encoding)
            : this(output, encoding, DEFAULT_BIG_ENDIAN) { }

        public EndianBinaryWriter(Stream output, Encoding encoding, bool bigEndian)
        {
            this.output   = output;
            this.encoding = encoding;
            this.writer   = new BinaryWriter(output, encoding);
            this.bigEnd   = bigEndian;
        }

        /* ===[ Properties ]================================================= */

        public bool BigEndian
        {
            get { return bigEnd; }
            set { bigEnd = value; }
        }

        public bool LittleEndian
        {
            get { return !bigEnd; }
            set { bigEnd = !value; }
        }

        /* ===[ Methods ]==================================================== */

        public void Flush()
        {
            writer.Flush();
        }

        /* ===[ 8-bit ]============== */

        public uint Write(byte data)
        {
            writer.Write(data);
            return sizeof(byte);
        }

        public uint Write(sbyte data)
        {
            writer.Write(data);
            return sizeof(byte);
        }

        public uint Write(bool data)
        {
            return Write(data
                ? (byte)0x01
                : (byte)0x00);
        }

        /* ===[ 16-bit ]============= */

        public uint Write(char data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(short data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(ushort data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        /* ===[ 32-bit ]============= */

        public uint Write(float data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(int data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(uint data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        /* ===[ 64-bit ]============= */

        public uint Write(double data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(long data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        public uint Write(ulong data)
        {
            return Write(EndianBytes(BitConverter.GetBytes(data)));
        }

        /* ===[ N-bit ]============== */

        public uint Write(byte[] data)
        {
            writer.Write(data);
            return (uint)data.LongLength;
        }

        public uint Write(char[] data)
        {
            return (uint)data
                .Sum(c => Write(c));
        }

        public uint Write(string data)
        {
            return Write(encoding.GetBytes(data));
        }

        public uint Write(byte[] buffer, int index, int count)
        {
            writer.Write(buffer, index, count);
            return (uint)count;
        }

        public uint Write(char[] chars, int index, int count)
        {
            return (uint)chars
                .Skip(index)
                .Take(count)
                .Sum(c => Write(c));
        }

        /* ========================== */

        protected byte[] EndianBytes(byte[] data)
        {
            return (BitConverter.IsLittleEndian ^ bigEnd)
                ? data
                : data.Reverse().ToArray();
        }

        /* ===[ EOF ]======================================================== */
    }
}
