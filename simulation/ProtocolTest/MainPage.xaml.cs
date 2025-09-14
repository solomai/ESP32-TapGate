// File: MainPage.xaml.cs
using ProtocolTest.Service.Cryptographer.Impl;
using System.Collections.ObjectModel;

namespace ProtocolTest
{
    public partial class MainPage : ContentPage
    {
        // Terminal data source (read-only view)
        public ObservableCollection<string> LogLines { get; } = new();

        // Track collapse state
        private bool _paneCollapsed = false;

        public MainPage()
        {
            InitializeComponent();
            BindingContext = this;

            // Initial line
            Log("Console ready.");
        }

        // Append a line to the terminal and auto-scroll to bottom
        public void Log(string line)
        {
            // Output to system console as well
            //Console.WriteLine(line);

            // Ensure non-null
            line ??= string.Empty;

            LogLines.Add(line);

            // Scroll to the newest line on the UI thread
            MainThread.BeginInvokeOnMainThread(() =>
            {
                if (LogLines.Count > 0)
                {
                    TerminalView.ScrollTo(LogLines.Count - 1, position: ScrollToPosition.End, animate: false);
                }
            });
        }

        // Collapse/expand left panel
        private void OnTogglePaneClicked(object? sender, EventArgs e)
        {
            _paneCollapsed = !_paneCollapsed;

            // Collapse: hide panel and set width to 0; Expand: show and set width to 260
            if (_paneCollapsed)
            {
                LeftPanel.IsVisible = false;                  // remove from hit-testing/measure
                ColLeft.Width = new GridLength(0);            // no width
                TogglePaneBtn.Text = ">";                     // show expand chevron
            }
            else
            {
                ColLeft.Width = new GridLength(260);          // restore width
                LeftPanel.IsVisible = true;                   // make visible again
                TogglePaneBtn.Text = "<";                     // show collapse chevron
            }
        }

        // Start button handler (replace with real logic)
        private void OnStartClicked(object? sender, EventArgs e)
        {
            Log($"[{DateTime.Now:HH:mm:ss}] Start pressed.");

            // Example: Generate RSA key pair and log lengths
            (string publicPem, string privatePem) = RsaPem.GenerateRsaKeyPair();
            Log($"Generated RSA Key Pair:\nPublic Key: {publicPem.Length} symbols\n{publicPem}\nPrivate Key: {privatePem.Length} symbols\n{privatePem}");

            // Example: store and load keys
            PemStorage.SaveKeysAsync(privatePem, publicPem).Wait();
            // Load keys back
            var (loadedPriv, loadedPub, loadedDevicePub) = PemStorage.LoadKeysAsync().Result;
            Log($"Generated RSA Key Pair:\nPublic Key: {(loadedPub?.Length ?? 0)} symbols\n{loadedPub ?? string.Empty}\nPrivate Key: {(loadedPriv?.Length ?? 0)} symbols\n{loadedPriv ?? string.Empty}");

            // Example: encode string to Base64
        }
    }
}
