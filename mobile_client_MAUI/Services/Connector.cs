namespace TapGate.Services;

public class Connector
{
    private readonly IFeed _bluetooth;
    private readonly IFeed _mqtt;

    public event EventHandler<string>? StatusUpdate;

    public Connector()
    {
        _bluetooth = new BluetoothFeed();
        _bluetooth.StatusUpdate += OnStatusUpdate;
        _mqtt = new MqttFeed();
        _mqtt.StatusUpdate += OnStatusUpdate;
    }

    public Task DoAction(string command)
    {
        // TODO: route command to appropriate feed
        return _bluetooth.SendCmd(command);
    }

    private void OnStatusUpdate(object? sender, string status)
    {
        StatusUpdate?.Invoke(sender, status);
    }
}
