using LibVLCSharp.Shared;
using System.Runtime.CompilerServices;
[assembly: InternalsVisibleTo("LostEye.SmokeTests")]
namespace LostEye;
internal static class Program
{
    [STAThread]
    static void Main(string[] args)
    {
        Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        if (args.Length == 1 && args[0] is "--register-open-with" or "--unregister-open-with") { OpenWithRegistration.Run(args[0] == "--unregister-open-with"); return; }
        Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
        Application.ThreadException += (_, _) => MessageBox.Show("Lost Eye encountered a problem. Close it and try opening the video again.", "Lost Eye", MessageBoxButtons.OK, MessageBoxIcon.Error);
        try
        {
            Core.Initialize(Path.Combine(AppContext.BaseDirectory, "libvlc", "win-x64"));
            Application.Run(new PlayerWindow(args.FirstOrDefault()));
        }
        catch (Exception)
        {
            MessageBox.Show("Lost Eye could not start its playback engine. Extract the entire ZIP into a folder and run Lost Eye.exe from there. If the problem continues, download a fresh copy.", "Lost Eye", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
