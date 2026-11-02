// Read-only adapter to the user-installed Smithbox libraries; no library code vendored.
using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.InteropServices;
using System.Text.Json;
using Andre.Formats;
using Andre.Core;
using SoulsFormats;
using Andre.Core.Util;
using System.Security.Cryptography;

class Program {
 static string toolRoot;
 static string gameRoot;
 static string outputRoot;
 static void Main(string[] args) {
  if(args.Length!=3)throw new ArgumentException("Expected smithbox directory, game directory, output directory");
  toolRoot=Path.GetFullPath(args[0]);gameRoot=Path.GetFullPath(args[1]);outputRoot=Path.GetFullPath(args[2]);
  AssemblyLoadContext.Default.Resolving+=(ctx,name)=>{var p=Path.Combine(toolRoot,name.Name+".dll");return File.Exists(p)?ctx.LoadFromAssemblyPath(p):null;};
  AssemblyLoadContext.Default.ResolvingUnmanagedDll+=(assembly,name)=>{var p=Path.Combine(toolRoot,name.EndsWith(".dll")?name:name+".dll");return File.Exists(p)?NativeLibrary.Load(p):IntPtr.Zero;};
  Extract();
 }
 static void Extract(){
  string game=gameRoot;
  string output=outputRoot;Directory.CreateDirectory(output);
  Directory.SetCurrentDirectory(AppContext.BaseDirectory);
  var dict=new BhdDictionary(File.ReadAllText(Path.Combine(toolRoot,"Assets/UXM Dictionaries/EldenRingDictionary.txt")),BHD5.Game.EldenRing);
  var names=File.ReadAllLines(Path.Combine(toolRoot,"Assets/UXM Dictionaries/EldenRingDictionary.txt")).Where(s=>s.StartsWith("/material/")&&s.Contains("allmaterial")&&s.EndsWith(".dcx")).ToArray();
  foreach(var bhd in BinderArchive.FindBHDs(game,Game.ER)){
   Console.WriteLine("READ "+Path.GetFileName(bhd));
   using var archive=new BinderArchive(bhd,Path.ChangeExtension(bhd,".bdt"),Game.ER);
   foreach(var name in names){
    var bytes=archive.TryReadFileFromHash(dict.ComputeHash(name));if(bytes==null)continue;
    string target=Path.Combine(output,Path.GetFileName(name)+".json");if(File.Exists(target))throw new Exception("Preserve existing "+target);
    var binder=BND4.Read(bytes);var entries=new List<object>();
    foreach(var f in binder.Files){
     var m=MATBIN.Read(f.Bytes);
     entries.Add(new {name=f.Name,sha256=Convert.ToHexString(SHA256.HashData(f.Bytes.Span)),shader=m.ShaderPath,source=m.SourcePath,
      parameters=m.Params.Select(p=>new {name=p.Name,value=p.Value,type=p.Type.ToString()}),
      samplers=m.Samplers.Select(s=>new {type=s.Type,path=s.Path})});
    }
    File.WriteAllText(target,JsonSerializer.Serialize(new{archive=name,sha256=Convert.ToHexString(SHA256.HashData(bytes)),entries},new JsonSerializerOptions{WriteIndented=true}));
    Console.WriteLine("EXTRACTED "+name+" entries="+entries.Count);
   }
  }
 }
}
