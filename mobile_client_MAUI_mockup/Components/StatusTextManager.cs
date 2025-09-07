using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Maui.Controls;

namespace MauiMockup.Components;

/// <summary>
/// Provides logic for displaying status text with a timed fade-out.
/// </summary>
public class StatusTextManager
{
    private readonly Label _label;
    private CancellationTokenSource? _cancellation;

    private static readonly TimeSpan VisibleDuration = TimeSpan.FromSeconds(2);
    private const uint FadeDuration = 500;

    public StatusTextManager(Label label)
    {
        _label = label;
    }

    /// <summary>
    /// Sets the status text and schedules it to fade away.
    /// </summary>
    public void SetText(string text)
    {
        _cancellation?.Cancel();
        _cancellation?.Dispose();
        _cancellation = null;
        _label.CancelAnimations();

        if (string.IsNullOrWhiteSpace(text))
        {
            _label.Text = string.Empty;
            _label.Opacity = 0;
            return;
        }

        _label.Text = text;
        _label.Opacity = 1;

        _cancellation = new CancellationTokenSource();
        _ = FadeOutAsync(_cancellation.Token);
    }

    private async Task FadeOutAsync(CancellationToken token)
    {
        try
        {
            await Task.Delay(VisibleDuration, token);
            await _label.FadeTo(0, FadeDuration, Easing.Linear);
            if (!token.IsCancellationRequested)
            {
                _label.Text = string.Empty;
            }
        }
        catch (TaskCanceledException)
        {
            // ignored
        }
    }
}

