namespace TapGate.Services;

public class MqttFeed : IFeed
{
    public event EventHandler<string>? StatusUpdate;

    public Task SendCmd(string command)
    {
        // TODO: implement MQTT command sending
        StatusUpdate?.Invoke(this, $"MQTT sent: {command}");
        return Task.CompletedTask;
    }
}
