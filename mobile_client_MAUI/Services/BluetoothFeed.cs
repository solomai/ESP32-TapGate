namespace MauiMockup.Services;

public class BluetoothFeed : IFeed
{
    public event EventHandler<string>? StatusUpdate;

    public Task SendCmd(string command)
    {
        // TODO: implement Bluetooth command sending
        StatusUpdate?.Invoke(this, $"Bluetooth sent: {command}");
        return Task.CompletedTask;
    }
}
