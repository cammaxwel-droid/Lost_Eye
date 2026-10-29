using Microsoft.Win32;
using System.Runtime.InteropServices;
namespace LostEye;
internal static class OpenWithRegistration
{
    const string AppKey = @"Applications\Lost Eye.exe";
    const string ProgId = "LostEye.Video";
    internal static readonly string[] Extensions = [".mp4", ".mkv", ".webm", ".avi", ".mov", ".m4v", ".wmv", ".mpg", ".mpeg", ".ts", ".m2ts", ".ogv", ".3gp", ".flv"];
    [DllImport("shell32.dll")] static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2);
    internal static void Register(RegistryKey classes, string executable)
    {
        var command = $"\"{executable}\" \"%1\"";
        using (var app = classes.CreateSubKey(AppKey))
        {
            app.SetValue("FriendlyAppName", "Lost Eye");
            using var icon = app.CreateSubKey("DefaultIcon"); icon.SetValue("", $"\"{executable}\",0");
            using var cmd = app.CreateSubKey(@"shell\open\command"); cmd.SetValue("", command);
            using var types = app.CreateSubKey("SupportedTypes");
            foreach (var extension in Extensions) types.SetValue(extension, "");
        }
        using (var prog = classes.CreateSubKey(ProgId))
        {
            prog.SetValue("", "Lost Eye video");
            using var icon = prog.CreateSubKey("DefaultIcon"); icon.SetValue("", $"\"{executable}\",0");
            using var cmd = prog.CreateSubKey(@"shell\open\command"); cmd.SetValue("", command);
        }
        foreach (var extension in Extensions)
        {
            using var list = classes.CreateSubKey(extension + @"\OpenWithProgids");
            list.SetValue(ProgId, Array.Empty<byte>(), RegistryValueKind.None);
        }
    }
    internal static void Unregister(RegistryKey classes, string executable)
    {
        using var existing = classes.OpenSubKey(AppKey + @"\shell\open\command");
        if (!string.Equals(existing?.GetValue("") as string, $"\"{executable}\" \"%1\"", StringComparison.OrdinalIgnoreCase)) return;
        foreach (var extension in Extensions)
        {
            using var list = classes.OpenSubKey(extension + @"\OpenWithProgids", writable: true);
            list?.DeleteValue(ProgId, throwOnMissingValue: false);
        }
        classes.DeleteSubKeyTree(AppKey, throwOnMissingSubKey: false);
        classes.DeleteSubKeyTree(ProgId, throwOnMissingSubKey: false);
    }
    internal static void Run(bool remove)
    {
        try
        {
            using var classes = Registry.CurrentUser.CreateSubKey(@"Software\Classes");
            var exe = Environment.ProcessPath!;
            if (remove) Unregister(classes, exe); else Register(classes, exe);
            SHChangeNotify(0x08000000, 0, IntPtr.Zero, IntPtr.Zero);
            MessageBox.Show(remove ? "Lost Eye's Open with listing has been removed for this folder." : "Lost Eye is now available in Windows Open with. Your default player has not been changed.\n\nKeep this folder in the same location, or run this registration again after moving it.", "Lost Eye", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception) { MessageBox.Show("Windows couldn't update the Open with listing. You can still choose Lost Eye.exe using Open with > Choose another app > Choose an app on your PC.", "Lost Eye", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }
}
