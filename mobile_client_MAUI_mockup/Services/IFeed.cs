namespace MauiMockup.Services;

public interface IFeed
{
    event EventHandler<string>? StatusUpdate;

    Task SendCmd(string command);
}
