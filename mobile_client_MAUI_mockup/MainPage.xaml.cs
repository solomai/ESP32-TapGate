namespace MauiMockup;

public partial class MainPage : ContentPage
{
    public MainPage()
    {
        InitializeComponent();
    }
    protected override void OnSizeAllocated(double width, double height)
    {
        base.OnSizeAllocated(width, height);
        bool isLandscape = width > height;
        LandscapeLayout.IsVisible = isLandscape;
        PortraitLayout.IsVisible = !isLandscape;
    }
    void OnDoAction(object sender, EventArgs e)
    {
        // Placeholder for main action
    }
}
