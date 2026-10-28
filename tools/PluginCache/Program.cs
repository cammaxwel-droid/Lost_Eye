using LibVLCSharp.Shared;
if (args.Length != 1) { Console.Error.WriteLine("Pass the bundled libvlc/win-x64 directory."); return 1; }
var directory = Path.GetFullPath(args[0]);
Core.Initialize(directory);
Console.WriteLine("Generating native plugin cache. Cold antivirus scans may take a minute.");
using (var engine = new LibVLC("--reset-plugins-cache")) { }
var cache = Path.Combine(directory, "plugins", "plugins.dat");
if (!File.Exists(cache) || new FileInfo(cache).Length == 0) throw new IOException("VLC did not generate its plugin cache.");
Console.WriteLine($"Plugin cache ready: {new FileInfo(cache).Length} bytes.");
return 0;
