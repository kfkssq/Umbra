param([switch]$Apply, [switch]$Activate, [string]$EngineRoot, [string]$Config)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (!$EngineRoot) {
    $localConfig = Join-Path $projectRoot 'Saved/EquipmentImport/dcx_tools.json'
    if (!(Test-Path -LiteralPath $localConfig)) { throw 'Provide -EngineRoot or configure Saved/EquipmentImport/dcx_tools.json.' }
    $EngineRoot = (Get-Content -LiteralPath $localConfig -Raw | ConvertFrom-Json).ue_root
}
$projectFile = Join-Path $projectRoot 'Umbra.uproject'
$association = (Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json).EngineAssociation
$version = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if ($association -ne "$($version.MajorVersion).$($version.MinorVersion)") { throw 'Engine version does not match Umbra.uproject.' }
$editorExe = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $editorExe)) { throw 'Unreal editor not found in configured engine.' }
if (Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Close Unreal editors before preparing assets.' }
if (!$Config) { $Config = Join-Path $PSScriptRoot 'random_equipment_v2.json' }
$Config = (Resolve-Path -LiteralPath $Config).Path
$settings = Get-Content -LiteralPath $Config -Raw | ConvertFrom-Json
$reportName = if ($settings.report_directory) { $settings.report_directory } else { 'RandomEquipment' }
$reportDir = Join-Path $projectRoot "Saved/$reportName"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
$oldApply = $env:UMBRA_RANDOM_APPLY
$oldActivate = $env:UMBRA_RANDOM_ACTIVATE
$oldReport = $env:UMBRA_RANDOM_REPORT
$oldConfig = $env:UMBRA_RANDOM_CONFIG
try {
    $env:UMBRA_RANDOM_CONFIG = $Config
    $env:UMBRA_RANDOM_APPLY = if ($Apply) { '1' } else { '0' }
    $env:UMBRA_RANDOM_REPORT = if ($Apply) { 'apply.json' } else { 'audit.json' }
    $scriptPath = Join-Path $PSScriptRoot 'create_random_equipment.py'
    & $editorExe $projectFile '-run=pythonscript' "-script=$scriptPath" '-unattended' '-nullrhi' '-nosound' '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' "-abslog=$reportDir/prepare.log"
    if ($LASTEXITCODE -ne 0) { throw "Preparation failed: $reportDir/prepare.log" }
    $report = Get-Content -LiteralPath (Join-Path $reportDir $env:UMBRA_RANDOM_REPORT) -Raw | ConvertFrom-Json
    if (!$report.ok) { throw "Preparation incomplete: $($report.error)" }
    if ($Activate) {
        $env:UMBRA_RANDOM_ACTIVATE = '1'
        $scriptPath = Join-Path $PSScriptRoot 'activate_random_equipment.py'
        & $editorExe $projectFile '-run=pythonscript' "-script=$scriptPath" '-unattended' '-nullrhi' '-nosound' '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' "-abslog=$reportDir/activation.log"
        if ($LASTEXITCODE -ne 0) { throw "Activation failed: $reportDir/activation.log" }
    }
    Write-Output "Equipment report: $reportDir"
} finally {
    $env:UMBRA_RANDOM_APPLY = $oldApply
    $env:UMBRA_RANDOM_ACTIVATE = $oldActivate
    $env:UMBRA_RANDOM_REPORT = $oldReport
    $env:UMBRA_RANDOM_CONFIG = $oldConfig
}
