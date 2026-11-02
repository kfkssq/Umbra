param(
 [Parameter(Mandatory=$true)][string]$GameRoot,
 [string]$SmithboxRoot,
 [string]$Tools="$PSScriptRoot/../../Saved/EquipmentImport/dcx_tools.json"
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$settings=Get-Content -LiteralPath $Tools -Raw | ConvertFrom-Json
if(!$SmithboxRoot){$SmithboxRoot=Split-Path $settings.witchy}
$SmithboxRoot=(Resolve-Path -LiteralPath $SmithboxRoot).Path
$GameRoot=(Resolve-Path -LiteralPath $GameRoot).Path
$output=Join-Path $root ('Saved/EquipmentImport/MaterialIndex/'+[guid]::NewGuid().ToString('N'))
$build=Join-Path $output 'Tool'
New-Item -ItemType Directory -Path $build -Force | Out-Null
& dotnet build "$PSScriptRoot/MaterialIndexTool/MaterialIndexTool.csproj" "-p:SmithboxDir=$SmithboxRoot" "-p:BaseIntermediateOutputPath=$output/obj/" -o $build
if($LASTEXITCODE -ne 0){throw 'Material audit tool build failed'}
Copy-Item -LiteralPath (Join-Path $GameRoot 'oo2core_6_win64.dll') -Destination $build
$definitions=Join-Path $output 'Definitions'
& dotnet (Join-Path $build 'MaterialIndexTool.dll') $SmithboxRoot $GameRoot $definitions
if($LASTEXITCODE -ne 0){throw 'Material extraction failed; existing config preserved'}
$python=Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$index=Join-Path $output 'material_index.json'
& $python -B "$PSScriptRoot/matbin_rules.py" --folder $definitions --output $index
if($LASTEXITCODE -ne 0){throw 'Index validation failed; existing config preserved'}
$settings | Add-Member -NotePropertyName material_index -NotePropertyValue $index -Force
$settings | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $Tools -Encoding utf8
Write-Output "Material index configured: $index"
