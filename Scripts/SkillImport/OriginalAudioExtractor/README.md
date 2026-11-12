# Original Pebble audio extraction

Run from the Umbra repository root. The source is the user's local Elden Ring base-game `sd` archive; no audio redistribution is provided by these scripts.

Prerequisites: .NET 10, Smithbox's `Andre.SoulsFormats.dll`, public `sd.pem` and `sd.txt` from [BinderKeys EldenRing_PC](https://github.com/JKAnderson/BinderKeys/tree/master/EldenRing_PC) under `Saved/SkillImport/AudioTools`. Put the official [vgmstream r2168](https://github.com/vgmstream/vgmstream/releases/tag/r2168) Windows distribution under its `vgmstream` subdirectory and retain its licenses. Event tracing used [wwiser v20260808](https://github.com/bnnm/wwiser/releases/tag/v20260808); local TXTP evidence is under `OriginalAudio/events-original`.

Set `UMBRA_SMITHBOX` to the Smithbox installation directory and `UMBRA_ER_GAME` to the game's `Game` directory. Run:

```powershell
dotnet run --project Scripts/SkillImport/OriginalAudioExtractor/AudioProbe.csproj "-p:SmithboxRoot=$env:UMBRA_SMITHBOX" "-p:BaseOutputPath=$PWD/Saved/SkillImport/AudioExtractor/bin/" "-p:BaseIntermediateOutputPath=$PWD/Saved/SkillImport/AudioExtractor/obj/" -- /enus/cs_main.bnk /init.bnk
# Use the Python bundled with the associated UE installation:
python Scripts/SkillImport/decode_pebble_original_audio.py
```

The extractor reads archives, resolves public path hashes, trims archive padding, and writes only requested banks to `Saved/SkillImport/GlintstonePebble/OriginalAudio`. The decoder extracts five verified WEM files, records hashes and source offsets, and decodes them without pitch/resampling/synthesis. It trims the flight WAV to the source loop interval [0,18687) at 44100 Hz.

Run `configure_pebble_original_audio.py` through UE 5.8's Python commandlet after building UmbraEditor. This creates/saves assets through editor APIs, disables the old synthetic charge sound, and wires release/flight/impact into the definition. Existing synthetic assets are retained but unreferenced by the skill. Rerunning the original skill bootstrap preserves the original audio wiring.

Source event mapping: `Play_s000523001` → 524617251; `Play_s000523002` → 1040010023; `Play_s000523003` → 82601330 / 164447618 / 123415567. FXR action 75 in f000523001–3 supplies the sound IDs; Wwise HIRC actions and containers resolve the media. Static event gains are -3/-15/-3 dB. UE's authored spatial attenuation and impact shuffle bag do not reproduce all original Wwise buses, RTPCs, environments or last-one avoidance semantics.
