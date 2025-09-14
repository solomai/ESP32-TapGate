using System;

namespace ProtocolTest.Service.Connector
{
    public class Connector
    {
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
