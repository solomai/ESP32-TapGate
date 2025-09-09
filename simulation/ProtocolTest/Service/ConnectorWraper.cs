namespace ProtocolTest.Service
{
    internal class ConnectorWraper
    {
        public ConnectorWraper(IConnector connector)
        {
            Connector = connector;
        }
        public IConnector Connector { get; }
    }
}
