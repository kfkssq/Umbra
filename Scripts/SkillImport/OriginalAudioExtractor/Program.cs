using System.Reflection;using System.Runtime.Loader;using System.Numerics;using System.Security.Cryptography;using System.Text.Json;using SoulsFormats;
var tools=Environment.GetEnvironmentVariable("UMBRA_SMITHBOX") ?? throw new Exception("Set UMBRA_SMITHBOX to the Smithbox directory");
AssemblyLoadContext.Default.Resolving+=(ctx,name)=>File.Exists(Path.Combine(tools,name.Name+".dll"))?ctx.LoadFromAssemblyPath(Path.Combine(tools,name.Name+".dll")):null;
var work=Path.GetFullPath("Saved/SkillImport/AudioTools");var output=Path.GetFullPath("Saved/SkillImport/GlintstonePebble/OriginalAudio");Directory.CreateDirectory(output);
var source=Path.Combine(Environment.GetEnvironmentVariable("UMBRA_ER_GAME") ?? throw new Exception("Set UMBRA_ER_GAME to the game directory"), "sd");
var cache=Path.Combine(work,"sd.header.bin");
var archiveHash=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.Combine(source,"sd.bhd"))));
var cacheHash=cache+".sha256";
byte[] header;
if(File.Exists(cache) && File.Exists(cacheHash) && File.ReadAllText(cacheHash)==archiveHash)header=File.ReadAllBytes(cache);else{
 using var rsa=RSA.Create();rsa.ImportFromPem(File.ReadAllText(Path.Combine(work,"sd.pem")));var key=rsa.ExportParameters(false);
 var n=new BigInteger(key.Modulus!,true,true);var e=new BigInteger(key.Exponent!,true,true);int block=key.Modulus!.Length;
 var encrypted=File.ReadAllBytes(Path.Combine(source,"sd.bhd"));header=new byte[encrypted.Length/block*(block-1)];
 Parallel.For(0,encrypted.Length/block,i=>{var c=new BigInteger(encrypted.AsSpan(i*block,block),true,true);var value=BigInteger.ModPow(c,e,n).ToByteArray(true,true);value.CopyTo(header.AsSpan(i*(block-1)+(block-1)-value.Length));});
 Console.WriteLine(System.Text.Encoding.ASCII.GetString(header,0,4));File.WriteAllBytes(cache,header);File.WriteAllText(cacheHash,archiveHash);
}
var bhd=BHD5.Read(header,BHD5.Game.EldenRing);
ulong Hash(string name)=>name.ToLowerInvariant().Aggregate(0UL,(h,c)=>unchecked(h*133+c));
var dictionary=File.ReadAllLines(Path.Combine(work,"sd.txt")).Where(s=>!string.IsNullOrWhiteSpace(s)).ToDictionary(Hash,s=>s);
var files=bhd.Buckets.SelectMany(b=>b).ToList();
File.WriteAllText(Path.Combine(output,"archive-index.json"),JsonSerializer.Serialize(files.Select(f=>new{name=dictionary.GetValueOrDefault(f.FileNameHash),hash=f.FileNameHash,size=f.UnpaddedFileSize}),new JsonSerializerOptions{WriteIndented=true}));
using var stream=File.OpenRead(Path.Combine(source,"sd.bdt"));
foreach(var f in files){if(dictionary.TryGetValue(f.FileNameHash,out var name) && (args.Contains(name)|| (args.Length==0 && new[]{"/enus/cs_main.bnk","/init.bnk"}.Contains(name.ToLowerInvariant())))){
 var bytes=f.ReadFile(stream); if(f.UnpaddedFileSize>0) Array.Resize(ref bytes,checked((int)f.UnpaddedFileSize));var path=Path.Combine(output,Path.GetFileName(name));File.WriteAllBytes(path,bytes);Console.WriteLine(name+" "+bytes.Length+" "+System.Text.Encoding.ASCII.GetString(bytes,0,4));
}}
