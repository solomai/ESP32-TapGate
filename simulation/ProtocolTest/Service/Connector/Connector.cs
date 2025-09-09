using System;

namespace ProtocolTest.Service.Connector
{
    internal class Connector
    {
        public bool SendCommand(string command)
        {
            return true;
        }

        public bool SetCallback(Action<string> callback)
        {
            return true;
        }
    }
}
