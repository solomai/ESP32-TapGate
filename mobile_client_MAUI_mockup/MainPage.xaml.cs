using MauiMockup.Services;

﻿namespace MauiMockup;

public partial class MainPage : ContentPage
{
    private readonly Connector _connector = new();

    public MainPage()
    {
        InitializeComponent();
        _connector.StatusUpdate += OnStatusUpdate;
        SetGateName("gate is not configured yet");
        SetGateStatus(EnumGateState.Offline);
        SetStatusText("");
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
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Star });
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            ContentGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            ContentGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Star });
            ContentGrid.RowSpacing = 0;
            ContentGrid.ColumnSpacing = 40;

            Grid.SetRow(StatusLayout, 0);
            Grid.SetColumn(StatusLayout, 0);
            Grid.SetRow(ActionButton, 0);
            Grid.SetColumn(ActionButton, 1);
            Grid.SetRow(StatusTextLabel, 1);
            Grid.SetColumn(StatusTextLabel, 0);
            Grid.SetColumnSpan(StatusTextLabel, 2);
        }
        else
        {
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Star });
            ContentGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            ContentGrid.RowSpacing = 40;
            ContentGrid.ColumnSpacing = 0;

            Grid.SetRow(StatusLayout, 0);
            Grid.SetColumn(StatusLayout, 0);
            Grid.SetRow(ActionButton, 1);
            Grid.SetColumn(ActionButton, 0);
            Grid.SetRow(StatusTextLabel, 2);
            Grid.SetColumn(StatusTextLabel, 0);
        }
    }

    void OnRightBtn(object sender, EventArgs e)
    {
        // Placeholder for right button action
        // TODO: Debug mockup
        void SetNextGateStatus()
        {
            // Get current state from GateStatusLabel.Text
            EnumGateState currentState;
            if (GateStatusLabel.Text.Contains("Offline"))
                currentState = EnumGateState.Offline;
            else if (GateStatusLabel.Text.Contains("Online"))
                currentState = EnumGateState.Online;
            else
                currentState = EnumGateState.Connecting;

            // Get next state, wrap to first if at last
            var states = Enum.GetValues<EnumGateState>();
            int nextIndex = ((int)currentState + 1) % states.Length;
            SetGateStatus(states[nextIndex]);
        }
        SetNextGateStatus();
        SetStatusText("Next state"); // TODO: Debug mockup
        // TODO: Debug mockup - END
    }
    void OnLeftBtn(object sender, EventArgs e)
    {
        // Placeholder for left button action
        // TODO: Debug mockup
        void SetNextGateStatus()
        {
            // Get current state from GateStatusLabel.Text
            EnumGateState currentState;
            if (GateStatusLabel.Text.Contains("Offline"))
                currentState = EnumGateState.Offline;
            else if (GateStatusLabel.Text.Contains("Online"))
                currentState = EnumGateState.Online;
            else
                currentState = EnumGateState.Connecting;

            // Get all states in order
            var states = Enum.GetValues<EnumGateState>();
            int currentIndex = Array.IndexOf(states, currentState);
            // Previous index with wrap-around
            int prevIndex = (currentIndex - 1 + states.Length) % states.Length;
            SetGateStatus(states[prevIndex]);
        }
        SetNextGateStatus();
        SetStatusText("Prev state"); // TODO: Debug mockup
        // TODO: Debug mockup - END
    }
    void OnTitleBtn(object sender, EventArgs e)
    {
        // Placeholder for title button action
        SetStatusText("Gate Title Button"); // TODO: Debug mockup
    }
    void OnDoAction(object sender, EventArgs e)
    {
        SetStatusText("DoAction"); // TODO: Debug mockup
        _connector.DoAction("default");
    }

    void OnStatusUpdate(object? sender, string status)
    {
        // Placeholder for handling status updates
    }

    public enum EnumGateState
    {
        Offline,
        Online,
        Connecting
    }

    public void SetGateName(string name)
    {
        GateTitleButton.Text = name;
    }

    public void SetGateStatus(EnumGateState state)
    {
        Color color;
        string text;
        bool enable_button = false;
        const string prefix = "Gate Status: ";
        switch (state)
        {
            case EnumGateState.Offline:
                color = (Color)Application.Current!.Resources["ColorOffline"];
                text = prefix + "Offline";
                break;
            case EnumGateState.Online:
                enable_button = true;
                color = (Color)Application.Current!.Resources["ColorOnline"];
                text = prefix + "Online";
                break;
            default:
                color = (Color)Application.Current!.Resources["ColorConnecting"];
                text = prefix + "Connecting";
                break;
        }
        GateStatusPath.Fill = new SolidColorBrush(color);
        GateStatusLabel.Text = text;

        ActionButton.IsEnabled = enable_button;
    }
    public void SetStatusText(string text)
    {
        StatusTextLabel.Text = text;
    }
}
