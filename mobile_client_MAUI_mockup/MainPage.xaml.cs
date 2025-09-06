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
        UpdateLayout(isLandscape);
    }

    void UpdateLayout(bool isLandscape)
    {
        ContentGrid.RowDefinitions.Clear();
        ContentGrid.ColumnDefinitions.Clear();

        if (isLandscape)
        {
            ContentGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            ContentGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Star });
            ContentGrid.RowSpacing = 0;
            ContentGrid.ColumnSpacing = 40;

            Grid.SetRow(StatusLayout, 0);
            Grid.SetColumn(StatusLayout, 0);
            Grid.SetRow(ActionButton, 0);
            Grid.SetColumn(ActionButton, 1);
        }
        else
        {
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Star });
            ContentGrid.RowSpacing = 40;
            ContentGrid.ColumnSpacing = 0;

            Grid.SetRow(StatusLayout, 0);
            Grid.SetColumn(StatusLayout, 0);
            Grid.SetRow(ActionButton, 1);
            Grid.SetColumn(ActionButton, 0);
        }
    }
    void OnDoAction(object sender, EventArgs e)
    {
        // Placeholder for main action
    }
}
