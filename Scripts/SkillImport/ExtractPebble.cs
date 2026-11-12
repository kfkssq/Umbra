// Source-only extraction. No writes to the source dump, tools installation or UE Content.
using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;
using SoulsFormats;

if (args.Length != 3) throw new ArgumentException("Expected: source-root smithbox-root output-directory");
var source = Path.GetFullPath(args[0]);
var tools = Path.GetFullPath(args[1]);
var output = Path.GetFullPath(args[2]);
if (output.StartsWith(source + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)
    || output.Equals(source, StringComparison.OrdinalIgnoreCase))
    throw new ArgumentException("Output must be isolated from source files.");
Directory.CreateDirectory(output);
AssemblyLoadContext.Default.Resolving += (ctx, name) =>
    File.Exists(Path.Combine(tools, name.Name + ".dll"))
        ? ctx.LoadFromAssemblyPath(Path.Combine(tools, name.Name + ".dll")) : null;
NativeLibrary.SetDllImportResolver(typeof(BND4).Assembly, (name, assembly, path) =>
{
    foreach (var candidate in new[] { Path.Combine(tools, name), Path.Combine(tools, name + ".dll") })
        if (File.Exists(candidate)) return NativeLibrary.Load(candidate);
    return IntPtr.Zero;
});
// SoulsFormats checks the working directory for Oodle. Use the build output only.
var oodle = Path.Combine(AppContext.BaseDirectory, "oo2core_6_win64.dll");
if (!File.Exists(oodle)) File.Copy(Path.Combine(tools, "oo2core_6_win64.dll"), oodle);
Directory.SetCurrentDirectory(AppContext.BaseDirectory);
var options = new JsonSerializerOptions { WriteIndented = true };
var inputs = new Dictionary<string, string>();
var extracted = new List<object>();
string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes));
BND4 Read(string path)
{
    var bytes = File.ReadAllBytes(path);
    inputs[path] = Hash(bytes);
    return BND4.Read(bytes);
}
void Save(string name, byte[] bytes, string origin)
{
    if (Path.GetFileName(name) != name) throw new ArgumentException("Output must be a leaf filename");
    var path = Path.Combine(output, name);
    if (File.Exists(path))
    {
        if (Hash(File.ReadAllBytes(path)) != Hash(bytes)) throw new IOException("Existing output conflict: " + path);
    }
    else File.WriteAllBytes(path, bytes);
    extracted.Add(new { name, origin, sha256 = Hash(bytes), size = bytes.Length });
}
// The verified player action group is physically in a9x, not a4x.
foreach (var name in new[] { "c0000.anibnd.dcx", "c0000_a9x.anibnd.dcx", "c0000.chrbnd.dcx" })
{
    var archive = Read(Path.Combine(source, "chr", name));
    foreach (var entry in archive.Files)
    {
        var leaf = Path.GetFileName(entry.Name);
        if (leaf.StartsWith("a401_") || leaf == "a401.tae" || leaf == "a9x.compendium"
            || leaf.Equals("Skeleton.hkx", StringComparison.OrdinalIgnoreCase) || leaf == "c0000.flver")
            Save(leaf, entry.Bytes.ToArray(), name + ":" + entry.Name);
    }
}
var tae = TAE.Read(Path.Combine(output, "a401.tae"));
File.WriteAllText(Path.Combine(output, "a401-events.json"), JsonSerializer.Serialize(new
{
    event_bank = tae.EventBank,
    status = "raw_only_installed_template_does_not_support_bank_37",
    animations = tae.Animations.Select(a => new
    {
        a.ID, a.AnimFileName,
        header = JsonSerializer.SerializeToElement(a.MiniHeader, a.MiniHeader.GetType()),
        events = a.Events.Select(e => new { e.Type, e.StartTime, e.EndTime,
            raw = Convert.ToHexString(e.GetParameterBytes(tae.BigEndian)) })
    })
}, options));
var common = Read(Path.Combine(source, "sfx", "sfxbnd_commoneffects.ffxbnd.dcx"));
var ids = new HashSet<string> { "430", "440", "510010", "523000", "523001", "523002", "523003" };
foreach (var entry in common.Files.Where(f => ids.Contains(Path.GetFileNameWithoutExtension(f.Name).TrimStart('f', '0'))))
    Save(Path.GetFileName(entry.Name), entry.Bytes.ToArray(), entry.Name);
var required = ids.SelectMany(id => File.ReadAllLines(Path.Combine(output, "f" + id.PadLeft(9, '0') + ".ffxreslist")))
    .Where(s => !string.IsNullOrWhiteSpace(s)).Select(s => Path.GetFileNameWithoutExtension(s.Trim())).ToHashSet();
var found = new HashSet<string>();
foreach (var entry in common.Files.Where(f => required.Contains(Path.GetFileNameWithoutExtension(f.Name))))
{
    Save(Path.GetFileName(entry.Name), entry.Bytes.ToArray(), entry.Name);
    found.Add(Path.GetFileNameWithoutExtension(entry.Name));
    if (entry.Name.EndsWith(".tpf"))
        foreach (var texture in TPF.Read(entry.Bytes).Textures)
            Save(texture.Name + ".dds", texture.Bytes, entry.Name + ":" + texture.Name);
}
// The base impact resource list refers to s00002_m, supplied by DLC02 common resources.
if (required.Except(found).Any())
{
    var supplementPath = Path.Combine(source, "sfx", "sfxbnd_commoneffects_dlc02.ffxbnd.dcx");
    if (File.Exists(supplementPath))
    {
        var supplement = Read(supplementPath);
        foreach (var entry in supplement.Files.Where(f => required.Contains(Path.GetFileNameWithoutExtension(f.Name))
                     && !found.Contains(Path.GetFileNameWithoutExtension(f.Name))))
        {
            Save(Path.GetFileName(entry.Name), entry.Bytes.ToArray(), supplementPath + ":" + entry.Name);
            found.Add(Path.GetFileNameWithoutExtension(entry.Name));
            if (entry.Name.EndsWith(".tpf"))
                foreach (var texture in TPF.Read(entry.Bytes).Textures)
                    Save(texture.Name + ".dds", texture.Bytes, supplementPath + ":" + texture.Name);
        }
    }
}
File.WriteAllText(Path.Combine(output, "extraction-report.json"), JsonSerializer.Serialize(new
{
    status = "partial_source_extraction_not_ue_import", inputs, extracted,
    missing_resources = required.Except(found).Order().ToArray(),
    tool = new { path = typeof(BND4).Assembly.Location,
        sha256 = Hash(File.ReadAllBytes(Path.Combine(tools, "Andre.SoulsFormats.dll"))) },
    source_rights = "user_local_dump_authorization_and_distribution_license_not_verified"
}, options));
Console.WriteLine($"EXTRACTED {extracted.Count}; missing resources: {string.Join(",", required.Except(found))}");
