using System;
using ProtocolTest.Service.Connector;

namespace ProtocolTest.Service
{
    internal class ConnectorWraper
    {
        private static Connector? _connector;

        public ConnectorWraper()
        {
            _connector ??= new Connector();
        }

        public bool SendCommand(string command) => _connector!.SendCommand(command);

        public bool SetCallback(Action<string> callback) => _connector!.SetCallback(callback);
    }
}
