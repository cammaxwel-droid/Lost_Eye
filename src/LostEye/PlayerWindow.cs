using LibVLCSharp.Shared;
using System.Diagnostics;

namespace LostEye;

internal sealed class PlayerWindow : Form
{
    static LibVLC CreateEngine()
    {
        // The build generates this cache with the exact shipped binaries. Relative plugin
        // paths make it relocatable. Avoid loading every module just to discover it.
        var cache = Path.Combine(AppContext.BaseDirectory, "libvlc", "win-x64", "plugins", "plugins.dat");
        return new LibVLC("--no-video-title-show", "--no-osd", "--no-snapshot-preview",
            "--no-metadata-network-access", "--no-sub-autodetect-file", "--file-caching=300",
            File.Exists(cache) ? "--no-plugins-scan" : "--plugins-scan");
    }
    readonly LibVLC engine = CreateEngine();
    readonly Panel surface = new() { Dock = DockStyle.Fill, BackColor = Color.Black };
    readonly Panel welcome = new() { Dock = DockStyle.Fill, BackColor = Color.FromArgb(19, 20, 23) };
    readonly Label message = new() { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleCenter, ForeColor = Color.FromArgb(216, 218, 224), Padding = new Padding(32), AccessibleName = "Player message" };
    readonly Label filename = new() { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, AutoEllipsis = true, Text = "Lost Eye", Padding = new Padding(12, 0, 8, 0) };
    readonly Button open = MakeButton("Open file", 100);
    readonly Button play = MakeButton("Play", 78);
    readonly Button mute = MakeButton("Mute", 78);
    readonly Button fullscreen = MakeButton("Full screen", 105);
    readonly CheckBox loop = new() { Text = "Loop", AutoSize = true, Anchor = AnchorStyles.Left, AccessibleName = "Loop entire video", ForeColor = Color.White, Margin = new Padding(12, 0, 12, 0) };
    readonly TrackBar seek = new() { Minimum = 0, Maximum = 10000, TickStyle = TickStyle.None, Dock = DockStyle.Fill, AutoSize = false, Height = 30, AccessibleName = "Seek video", SmallChange = 100, LargeChange = 500 };
    readonly TrackBar volume = new() { Minimum = 0, Maximum = 100, Value = 80, TickStyle = TickStyle.None, Width = 110, AutoSize = false, Height = 32, Anchor = AnchorStyles.Left, AccessibleName = "Volume", SmallChange = 5, LargeChange = 10 };
    readonly Label time = new() { Text = "0:00 / 0:00", Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleRight, AccessibleName = "Elapsed and total time" };
    readonly Label volumeLabel = new() { Text = "80%", AutoSize = true, Anchor = AnchorStyles.Left, MinimumSize = new Size(36, 0), AccessibleName = "Volume percent" };
    readonly System.Windows.Forms.Timer timer = new() { Interval = 200 };
    readonly ToolTip hints = new();
    readonly SemaphoreSlim changes = new(1, 1);
    MediaPlayer? player;
    Media? media;
    bool dragging, ended, failed, closing, closeReady, isFullscreen, loading;
    bool muted;
    int requestId;
    long lastDuration;
    Rectangle savedBounds;
    FormWindowState savedState;
    readonly Stopwatch opening = new();
    string? currentPath;
    public PlayerWindow(string? path = null)
    {
        SuspendLayout();
        Text = "Lost Eye";
        ClientSize = new Size(900, 560);
        MinimumSize = new Size(700, 420);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(28, 29, 33);
        ForeColor = Color.White;
        Font = new Font("Segoe UI", 10);
        AutoScaleDimensions = new SizeF(96, 96);
        AutoScaleMode = AutoScaleMode.Dpi;
        Icon = new Icon(Path.Combine(AppContext.BaseDirectory, "lost-eye.ico"));
        KeyPreview = true;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, Margin = Padding.Empty };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 48));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 92));
        var top = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, Padding = new Padding(8, 4, 8, 4) };
        top.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 108));
        top.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        top.Controls.Add(open, 0, 0); top.Controls.Add(filename, 1, 0);
        var stage = new Panel { Dock = DockStyle.Fill, Margin = Padding.Empty };
        stage.Controls.Add(surface); stage.Controls.Add(welcome);
        welcome.Controls.Add(message);
        var bottom = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 2, ColumnCount = 1, Padding = new Padding(8, 4, 8, 8), Margin = Padding.Empty };
        bottom.RowStyles.Add(new RowStyle(SizeType.Absolute, 32));
        bottom.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        var timeline = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2 };
        timeline.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        timeline.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 145));
        timeline.Controls.Add(seek); timeline.Controls.Add(time);
        var controls = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 7, Padding = new Padding(4, 0, 4, 0) };
        foreach (var size in new float[] { 82, 85, 82, 115, 42 }) controls.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, size));
        controls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        controls.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 110));
        controls.Controls.Add(play, 0, 0); controls.Controls.Add(loop, 1, 0); controls.Controls.Add(mute, 2, 0);
        controls.Controls.Add(volume, 3, 0); controls.Controls.Add(volumeLabel, 4, 0); controls.Controls.Add(fullscreen, 6, 0);
        bottom.Controls.Add(timeline); bottom.Controls.Add(controls);
        layout.Controls.Add(top); layout.Controls.Add(stage); layout.Controls.Add(bottom); Controls.Add(layout);
        ShowMessage("Open a video to start watching\n\nUse Open file or drop a video here.");
        play.Enabled = seek.Enabled = false;
        hints.SetToolTip(open, "Open a video (Ctrl+O)"); hints.SetToolTip(play, "Play / pause (Space)");
        hints.SetToolTip(loop, "Repeat the whole video until switched off (L)");
        hints.SetToolTip(mute, "Mute / unmute (M)"); hints.SetToolTip(fullscreen, "Full screen (F11); Escape to leave");
        open.Click += async (_, _) => await ChooseFileAsync();
        play.Click += async (_, _) => await TogglePlaybackAsync();
        mute.Click += (_, _) => SetMuted(!muted);
        volume.ValueChanged += (_, _) => { if (volume.Value > 0) muted = false; ApplyVolume(); };
        fullscreen.Click += (_, _) => ToggleFullscreen();
        surface.DoubleClick += (_, _) => ToggleFullscreen();
        loop.CheckedChanged += (_, _) => loop.ForeColor = loop.Checked ? Color.FromArgb(255, 216, 66) : Color.White;
        seek.MouseDown += (_, e) => { if (e.Button == MouseButtons.Left) dragging = true; };
        seek.MouseUp += (_, e) => { if (e.Button == MouseButtons.Left) { dragging = false; SeekTo(seek.Value / 10000f); } };
        seek.MouseCaptureChanged += (_, _) => { if (!seek.Capture && dragging) { dragging = false; SeekTo(seek.Value / 10000f); } };
        seek.KeyUp += (_, e) => { if (e.KeyCode is Keys.Left or Keys.Right or Keys.Home or Keys.End or Keys.PageUp or Keys.PageDown) SeekTo(seek.Value / 10000f); };
        EnableDrop(this);
        timer.Tick += (_, _) => RefreshPlayback(); timer.Start();
        Shown += async (_, _) => { if (!string.IsNullOrWhiteSpace(path)) await OpenFileAsync(path); };
        FormClosing += async (_, e) =>
        {
            if (closeReady) return;
            e.Cancel = true;
            if (closing) return;
            closing = true; ++requestId; timer.Stop(); Enabled = false;
            await changes.WaitAsync();
            try { await ReleasePlayerAsync(); engine.Dispose(); }
            finally { changes.Release(); }
            closeReady = true; Close();
        };
        FormClosed += (_, _) => { timer.Dispose(); hints.Dispose(); Icon?.Dispose(); };
        ResumeLayout(true);
    }

    static Button MakeButton(string text, int width) => new()
    {
        Text = text, AccessibleName = text, Width = width, Height = 34, Anchor = AnchorStyles.Left,
        BackColor = Color.FromArgb(47, 49, 55), ForeColor = Color.White, FlatStyle = FlatStyle.Flat,
        UseVisualStyleBackColor = false, Margin = new Padding(2)
    };
    void EnableDrop(Control control)
    {
        control.AllowDrop = true;
        control.DragEnter += (_, e) => e.Effect = e.Data?.GetData(DataFormats.FileDrop) is string[] { Length: 1 } ? DragDropEffects.Copy : DragDropEffects.None;
        control.DragDrop += async (_, e) => { if (e.Data?.GetData(DataFormats.FileDrop) is string[] paths) await OpenDroppedFilesAsync(paths); };
        foreach (Control child in control.Controls) EnableDrop(child);
    }
    internal Task OpenDroppedFilesAsync(string[] paths)
    {
        if (paths.Length == 1) return OpenFileAsync(paths[0]);
        return Task.CompletedTask;
    }
    async Task ChooseFileAsync()
    {
        using var dialog = new OpenFileDialog
        {
            Title = "Open a video", CheckFileExists = true, Multiselect = false,
            Filter = "Video files|*.mp4;*.mkv;*.webm;*.avi;*.mov;*.m4v;*.wmv;*.mpg;*.mpeg;*.ts;*.m2ts;*.ogv;*.3gp;*.flv|All files|*.*"
        };
        if (dialog.ShowDialog(this) == DialogResult.OK) await OpenFileAsync(dialog.FileName);
    }
    internal async Task OpenFileAsync(string path)
    {
        if (closing) return;
        var id = ++requestId;
        await changes.WaitAsync();
        try
        {
            if (id != requestId || closing) return;
            loading = true; play.Enabled = seek.Enabled = false;
            ShowMessage("Opening video…");
            await ReleasePlayerAsync();
            if (id != requestId || closing) return;
            failed = ended = false; lastDuration = 0; seek.Value = 0; time.Text = "0:00 / 0:00";
            currentPath = Path.GetFullPath(path);
            filename.Text = Path.GetFileName(currentPath); Text = filename.Text + " — Lost Eye";
            if (!File.Exists(currentPath)) { Fail("This file is missing or unavailable. Check its location and open it again."); return; }
            using (var probe = new FileStream(currentPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                if (probe.Length == 0) { Fail("This file is empty. Choose another video."); return; }
            media = new Media(engine, currentPath, FromType.FromPath);
            player = new MediaPlayer(engine) { Hwnd = surface.Handle, EnableHardwareDecoding = true };
            // Disable libVLC input handling so our window owns keyboard and mouse controls.
            player.EnableKeyInput = false; player.EnableMouseInput = false;
            var active = player;
            active.Playing += (_, _) => Queue(active, () => { ApplyVolume(); return Task.CompletedTask; });
            active.EndReached += (_, _) => Queue(active, () => EndPlaybackAsync(active));
            active.EncounteredError += (_, _) => Queue(active, () => { Fail("Lost Eye couldn't play this file. It may be damaged or use an unsupported format."); return Task.CompletedTask; });
            opening.Restart();
            if (!active.Play(media)) { Fail("Lost Eye couldn't open this video. Try another file."); return; }
            ApplyVolume();
        }
        catch (UnauthorizedAccessException) { Fail("Lost Eye doesn't have permission to read this file. Choose a file you can access."); }
        catch (IOException) { Fail("This file couldn't be read. Check that it is available and try again."); }
        catch (Exception) { Fail("Lost Eye couldn't open this file. Check the path or choose another video."); }
        finally { loading = false; changes.Release(); }
    }
    void Queue(MediaPlayer active, Func<Task> action)
    {
        if (closing || IsDisposed || !IsHandleCreated) return;
        try { BeginInvoke(async () => { if (!closing && ReferenceEquals(active, player)) await action(); }); }
        catch (InvalidOperationException) { /* The window is already closing. */ }
    }
    async Task ReleasePlayerAsync()
    {
        var old = player; player = null;
        if (old != null) { await Task.Run(old.Stop); old.Dispose(); }
        media?.Dispose(); media = null;
        opening.Reset();
    }
    async Task EndPlaybackAsync(MediaPlayer active)
    {
        if (failed || loading) return;
        ended = true;
        play.Text = "Replay";
        if (lastDuration > 0) { seek.Value = seek.Maximum; time.Text = $"{FormatTime(lastDuration)} / {FormatTime(lastDuration)}"; }
        if (!HadVideoFrames) { Fail("No playable video was found. The file may be damaged or unsupported."); return; }
        if (loop.Checked) await RestartAsync(active);
    }
    async Task RestartAsync(MediaPlayer active)
    {
        await changes.WaitAsync();
        try
        {
            if (closing || !ReferenceEquals(active, player)) return;
            loading = true;
            await Task.Run(active.Stop);
            if (closing || !ReferenceEquals(active, player)) return;
            ended = false; active.Play(); ApplyVolume();
        }
        finally { loading = false; changes.Release(); }
    }
    internal async Task TogglePlaybackAsync()
    {
        if (loading || failed || player == null) return;
        if (ended) await RestartAsync(player);
        else player.SetPause(player.IsPlaying);
        RefreshPlayback();
    }
    internal void SeekTo(float position)
    {
        if (player == null || !player.IsSeekable || ended || failed || loading) return;
        player.Position = Math.Clamp(position, 0, 1);
    }
    void RefreshPlayback()
    {
        if (player == null || loading || failed) return;
        if (!ended)
        {
            if (player.Length > 0) lastDuration = player.Length;
            if (!dragging) seek.Value = Math.Clamp((int)(player.Position * 10000), 0, 10000);
            var positionTime = dragging ? (long)(seek.Value / 10000d * lastDuration) : Math.Max(0, player.Time);
            time.Text = $"{FormatTime(positionTime)} / {FormatTime(lastDuration)}";
            play.Text = player.IsPlaying ? "Pause" : "Play";
        }
        play.Enabled = true;
        seek.Enabled = !ended && player.IsSeekable && lastDuration > 0;
        if (HadVideoFrames) { surface.Visible = true; welcome.Visible = false; opening.Stop(); }
        else if (opening.IsRunning && opening.Elapsed.TotalSeconds > 20)
        {
            Fail("No playable video was found. The file may be damaged or unsupported.");
            var active = player;
            _ = StopFailedAsync(active);
        }
    }
    async Task StopFailedAsync(MediaPlayer active)
    {
        await changes.WaitAsync();
        try { if (ReferenceEquals(active, player)) await Task.Run(active.Stop); }
        finally { changes.Release(); }
    }
    void Fail(string text)
    {
        failed = true; ended = false; opening.Stop(); play.Text = "Play"; play.Enabled = seek.Enabled = false;
        ShowMessage(text + "\n\nUse Open file to choose another video.");
    }
    void ShowMessage(string text) { surface.Visible = false; message.Text = text; welcome.Visible = true; welcome.BringToFront(); }
    internal void SetMuted(bool value) { muted = value; ApplyVolume(); }
    void ApplyVolume()
    {
        if (player != null) { player.Volume = volume.Value; player.Mute = muted || volume.Value == 0; }
        mute.Text = muted || volume.Value == 0 ? "Unmute" : "Mute";
        volumeLabel.Text = volume.Value + "%";
    }
    internal void ToggleFullscreen()
    {
        if (!isFullscreen)
        {
            savedBounds = Bounds; savedState = WindowState;
            WindowState = FormWindowState.Normal; FormBorderStyle = FormBorderStyle.None;
            Bounds = Screen.FromControl(this).Bounds; isFullscreen = true; fullscreen.Text = "Exit full";
        }
        else
        {
            FormBorderStyle = FormBorderStyle.Sizable; Bounds = savedBounds; WindowState = savedState;
            isFullscreen = false; fullscreen.Text = "Full screen";
        }
    }
    protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
    {
        if (keyData == (Keys.Control | Keys.O)) { _ = ChooseFileAsync(); return true; }
        if (keyData == Keys.F11 || keyData == Keys.F) { ToggleFullscreen(); return true; }
        if (keyData == Keys.Escape && isFullscreen) { ToggleFullscreen(); return true; }
        if (keyData == Keys.Space && ActiveControl is not Button && ActiveControl is not CheckBox) { _ = TogglePlaybackAsync(); return true; }
        if (keyData == Keys.M) { SetMuted(!muted); return true; }
        if (keyData == Keys.L) { loop.Checked = !loop.Checked; return true; }
        if (keyData is Keys.Left or Keys.Right && !seek.Focused && !volume.Focused && player != null)
        {
            if (player.Length > 0) SeekTo((float)(player.Time + (keyData == Keys.Left ? -5000 : 5000)) / player.Length);
            return true;
        }
        return base.ProcessCmdKey(ref msg, keyData);
    }
    internal static string FormatTime(long milliseconds)
    {
        var t = TimeSpan.FromMilliseconds(Math.Max(0, milliseconds));
        return t.TotalHours >= 1 ? $"{(int)t.TotalHours}:{t.Minutes:00}:{t.Seconds:00}" : $"{(int)t.TotalMinutes}:{t.Seconds:00}";
    }
    internal bool HadVideoFrames => media?.Statistics.DecodedVideo > 0;
    internal MediaPlayer? ActivePlayer => player;
    internal bool Failed => failed;
    internal bool Ended => ended;
    internal string MessageText => message.Text;
    internal bool Fullscreen => isFullscreen;
    [System.ComponentModel.DesignerSerializationVisibility(System.ComponentModel.DesignerSerializationVisibility.Hidden)]
    internal bool Loop { get => loop.Checked; set => loop.Checked = value; }
    [System.ComponentModel.DesignerSerializationVisibility(System.ComponentModel.DesignerSerializationVisibility.Hidden)]
    internal int Volume { get => volume.Value; set => volume.Value = value; }
}





