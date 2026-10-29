using System.Diagnostics;
using System.Text.Json;
using LibVLCSharp.Shared;
using LostEye;

internal static class Tests
{
    static readonly List<object> results = [];
    static string fixtures = "", output = "";
    static int exitCode;
    [STAThread]
    static int Main(string[] args)
    {
        fixtures = Path.GetFullPath(args[0]); output = Path.GetFullPath(args[1]);
        Directory.CreateDirectory(output);
        Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false); Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
        try
        {
            File.WriteAllText(Path.Combine(output,"progress.txt"), "Loading core\n");
            Core.Initialize(args.Length > 2 ? Path.GetFullPath(args[2]) : Path.Combine(AppContext.BaseDirectory, "libvlc", "win-x64"));
            File.AppendAllText(Path.Combine(output,"progress.txt"), "Constructing form\n");
            var clock = Stopwatch.StartNew();
            using var form = new PlayerWindow();
            File.AppendAllText(Path.Combine(output,"progress.txt"), "Form constructed\n");
            form.Shown += async (_, _) =>
            {
                try { await Run(form, clock); }
                catch (Exception e) { exitCode = 1; Record("Unhandled test failure", false, e.ToString()); }
                finally
                {
                    File.WriteAllText(Path.Combine(output,"results.json"), JsonSerializer.Serialize(new { timestamp = DateTimeOffset.Now, os = Environment.OSVersion.ToString(), architecture = System.Runtime.InteropServices.RuntimeInformation.ProcessArchitecture.ToString(), results }, new JsonSerializerOptions { WriteIndented = true }));
                    form.Close();
                }
            };
            File.AppendAllText(Path.Combine(output,"progress.txt"), "Starting message loop\n");
            Application.Run(form);
        }
        catch (Exception e) { Console.WriteLine(e); return 1; }
        return exitCode;
    }
    static void Record(string name, bool pass, string detail = "")
    {
        File.AppendAllText(Path.Combine(output,"progress.txt"), name + ": " + pass + "\n");
        results.Add(new { name, pass, detail });
        Console.WriteLine($"{(pass ? "PASS" : "FAIL")}: {name} {detail}");
        if (!pass) exitCode = 1;
    }
    static async Task<bool> Until(Func<bool> predicate, int timeout = 15000)
    {
        var sw = Stopwatch.StartNew();
        while (!predicate() && sw.ElapsedMilliseconds < timeout) await Task.Delay(100);
        return predicate();
    }
    static async Task Run(PlayerWindow f, Stopwatch clock)
    {
        Record("Window starts", f.Visible && f.Text == "Lost Eye", $"{clock.ElapsedMilliseconds} ms constructor-to-shown; not cold process startup");
        var registryTestPath = @"Software\LostEye-Tests\" + Guid.NewGuid().ToString("N");
        try
        {
            using var sandbox = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(registryTestPath);
            using (var mp4 = sandbox.CreateSubKey(".mp4")) mp4.SetValue("", "Existing.Default.Player");
            const string fakeExe = @"C:\Example folder\Lost Eye.exe";
            OpenWithRegistration.Register(sandbox, fakeExe);
            using (var command = sandbox.OpenSubKey(@"Applications\Lost Eye.exe\shell\open\command"))
                Record("Open with quotes executable and file", command?.GetValue("") as string == "\"" + fakeExe + "\" \"%1\"");
            using (var mp4 = sandbox.OpenSubKey(".mp4"))
                Record("Registration preserves existing default", mp4?.GetValue("") as string == "Existing.Default.Player");
            OpenWithRegistration.Unregister(sandbox, fakeExe);
            using (var app = sandbox.OpenSubKey(@"Applications\Lost Eye.exe")) Record("Optional registration removal", app == null);
            using (var mp4 = sandbox.OpenSubKey(".mp4")) Record("Removal preserves existing default", mp4?.GetValue("") as string == "Existing.Default.Player");
        }
        finally { Microsoft.Win32.Registry.CurrentUser.DeleteSubKeyTree(registryTestPath, false); }
        Record("Time formatting", PlayerWindow.FormatTime(3661000) == "1:01:01" && PlayerWindow.FormatTime(65000) == "1:05" && PlayerWindow.FormatTime(-1) == "0:00");
        using (var bitmap = new Bitmap(f.Width, f.Height)) { f.DrawToBitmap(bitmap, new Rectangle(Point.Empty, f.Size)); bitmap.Save(Path.Combine(output, "layout-empty.png")); }
        await f.OpenFileAsync(Path.Combine(fixtures, "missing.mp4"));
        Record("Missing file handled", f.Failed && f.MessageText.Contains("missing"));
        await f.OpenFileAsync(Path.Combine(fixtures, "empty.mp4"));
        Record("Empty file handled", f.Failed && f.MessageText.Contains("empty"));
        await f.OpenFileAsync(Path.Combine(fixtures, "damaged.mp4"));
        Record("Damaged file handled", await Until(() => f.Failed, 25000), f.MessageText);
        foreach (var name in new[] { "sample video.mp4", "sample.mkv", "sample.webm" })
        {
            var start = Stopwatch.StartNew();
            await f.OpenFileAsync(Path.Combine(fixtures, name));
            bool decoded = await Until(() => f.HadVideoFrames && (f.ActivePlayer?.Time ?? 0) > 200);
            Record("Decode and advance " + name, decoded, $"{start.ElapsedMilliseconds} ms to decoded frames + clock; libVLC {f.ActivePlayer?.State}");
            Record("Video output " + name, f.ActivePlayer?.VoutCount > 0);
        }
        await f.OpenDroppedFilesAsync([Path.Combine(fixtures, "sample video.mp4")]);
        Record("Drop handler opens file", await Until(() => f.HadVideoFrames && (f.ActivePlayer?.Time ?? 0) > 500), "Calls the same file-drop handler; Explorer drag gesture still requires manual testing");
        f.SetMuted(true);
        Record("Mute", await Until(() => f.ActivePlayer!.Mute, 3000));
        f.SetMuted(false); f.Volume = 35;
        Record("Volume and unmute", await Until(() => !f.ActivePlayer!.Mute && f.ActivePlayer.Volume == 35, 3000));
        await f.TogglePlaybackAsync();
        bool paused = await Until(() => f.ActivePlayer!.State == VLCState.Paused);
        long before = f.ActivePlayer!.Time;
        await Task.Delay(700);
        Record("Pause holds position", paused && Math.Abs(f.ActivePlayer.Time - before) < 150);
        f.SeekTo(0.55f);
        await f.TogglePlaybackAsync();
        Record("Seek and resume", await Until(() => f.ActivePlayer.IsPlaying && f.ActivePlayer.Time > 3500), $"time={f.ActivePlayer.Time}");
        var bounds = f.Bounds;
        f.ToggleFullscreen();
        Record("Fullscreen", f.Fullscreen && f.FormBorderStyle == FormBorderStyle.None);
        f.ToggleFullscreen();
        Record("Fullscreen restores size", !f.Fullscreen && f.Bounds == bounds);
        f.Size = f.MinimumSize;
        await Task.Delay(200);
        Record("Resizable window", f.Width == f.MinimumSize.Width);
        using (var bitmap = new Bitmap(f.Width, f.Height)) { f.DrawToBitmap(bitmap, new Rectangle(Point.Empty, f.Size)); bitmap.Save(Path.Combine(output, "layout-minimum.png")); }
        f.Size = bounds.Size;
        f.Loop = true;
        int ends = 0;
        f.ActivePlayer.EndReached += (_, _) => Interlocked.Increment(ref ends);
        f.SeekTo(0.86f);
        Record("Whole video loops twice", await Until(() => Volatile.Read(ref ends) >= 2 && f.ActivePlayer.IsPlaying && f.ActivePlayer.Time < 3000, 22000), $"EndReached count={ends}");
        f.Loop = false;
        f.SeekTo(0.86f);
        Record("Loop off stops at end", await Until(() => f.Ended && !f.ActivePlayer.IsPlaying, 12000));
        await Task.Delay(700);
        Record("Stays stopped with loop off", f.Ended && !f.ActivePlayer.IsPlaying);
        await f.TogglePlaybackAsync();
        Record("Replay after end", await Until(() => f.ActivePlayer.IsPlaying && f.ActivePlayer.Time > 200 && f.ActivePlayer.Time < 3000));
        await Task.WhenAll(f.OpenFileAsync(Path.Combine(fixtures,"sample.mkv")), f.OpenFileAsync(Path.Combine(fixtures,"sample.webm")));
        Record("Rapid replacement keeps latest file", await Until(() => f.HadVideoFrames && f.Text.StartsWith("sample.webm")));
        await f.OpenFileAsync(Path.Combine(fixtures,"damaged.mp4"));
        Record("Error during session handled", await Until(() => f.Failed,25000));
        await f.OpenFileAsync(Path.Combine(fixtures,"sample video.mp4"));
        Record("Recovery from error", await Until(() => f.HadVideoFrames && !f.Failed));
        Record("Working set sample", true, $"{Process.GetCurrentProcess().WorkingSet64 / 1048576d:F1} MiB in integration test process");
    }
}





