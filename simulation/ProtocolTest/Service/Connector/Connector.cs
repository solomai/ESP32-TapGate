using System;

namespace ProtocolTest.Service.Connector
{
    internal class Connector
    {
        public Connector()
        {
        }
        public static bool SendCommand(string command)
        {
            return true;
        }
        public static bool SetCallback(Action<string> callback)
        {
            return true;
        }
    }
}
