[CmdletBinding()]
param(
    [ValidateSet('Check', 'Build', 'Setup', 'Play', 'Editor', 'Test', 'Package', 'CodeDatabase', 'ImportModels')]
    [string]$Action = 'Play',
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipBuild,
    [switch]$Capture,
    [switch]$Playthrough,
    [switch]$Grab,
    [switch]$Climbing,
    [switch]$LocalClimbing,
    [switch]$ClimbingGamepad,
    [switch]$Gamepad,
    [ValidateRange(0, 3600)][int]$TestSeconds = 0,
    [ValidateRange(15, 240)][int]$TestFPS = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'IshibashiriPrototype.uproject'
$MapFile = Join-Path $ProjectRoot 'Content\Maps\L_Prototype_01.umap'

function Find-Engine {
    if ($EngineRoot) {
        $Candidate = [System.IO.Path]::GetFullPath($EngineRoot)
        if (Test-Path -LiteralPath (Join-Path $Candidate 'Engine\Build\BatchFiles\Build.bat')) { return $Candidate }
        throw "EngineRoot is not an Unreal Engine installation: $Candidate"
    }
    $Candidates = [System.Collections.Generic.List[string]]::new()
    foreach ($Version in @('5.6', '5.5', '5.4')) {
        $Candidates.Add((Join-Path $env:USERPROFILE "UnrealEngine\UE_$Version"))
        $Candidates.Add("C:\Program Files\Epic Games\UE_$Version")
        $Key = Get-ItemProperty -LiteralPath "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$Version" -ErrorAction SilentlyContinue
        if ($Key -and $Key.PSObject.Properties['InstalledDirectory']) { $Candidates.Add($Key.InstalledDirectory) }
    }
    $Manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $Manifest) {
        $Installations = (Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json).InstallationList
        foreach ($Installation in $Installations) {
            if ($Installation.AppName -match '^UE_5\.[456]$') { $Candidates.Add($Installation.InstallLocation) }
        }
    }
    $SourceBuilds = Get-ItemProperty -LiteralPath 'HKCU:\Software\Epic Games\Unreal Engine\Builds' -ErrorAction SilentlyContinue
    if ($SourceBuilds) {
        foreach ($Property in $SourceBuilds.PSObject.Properties) {
            if ($Property.Name -notmatch '^PS' -and $Property.Value -is [string]) { $Candidates.Add($Property.Value) }
        }
    }
    foreach ($Candidate in $Candidates) {
        if (Test-Path -LiteralPath (Join-Path $Candidate 'Engine\Build\BatchFiles\Build.bat')) { return $Candidate }
    }
    throw 'UE 5.4-5.6 not found. Install Unreal Engine, then pass -EngineRoot "C:\Program Files\Epic Games\UE_5.6" (or set UE_ROOT). See README.md.'
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed (exit $LASTEXITCODE). See Saved\Logs." }
}

function Build-Editor {
    Invoke-Checked $BuildTool @('IshibashiriPrototypeEditor', 'Win64', 'Development', "-Project=$ProjectFile", '-WaitMutex', '-NoHotReloadFromIDE')
}

function Ensure-Map {
    if (!(Test-Path -LiteralPath $MapFile)) {
        $ScriptFile = Join-Path $PSScriptRoot 'CreatePrototypeMap.py'
        Invoke-Checked $EditorCmd @($ProjectFile, '-run=pythonscript', "-script=$ScriptFile", '-unattended', '-nop4', '-nullrhi', '-nosound', '-UTF8Output')
        if (!(Test-Path -LiteralPath $MapFile)) { throw 'Map generation failed: Content\Maps\L_Prototype_01.umap is missing.' }
    }
}

try {
    if (($Playthrough -or $Grab -or $Climbing -or $LocalClimbing -or $ClimbingGamepad -or $Gamepad -or $TestSeconds -gt 0) -and $Action -ne 'Test') { throw 'Test switches require -Action Test.' }
    if (@(@($Playthrough, $Grab, $Climbing, $LocalClimbing, $ClimbingGamepad, $Gamepad) | Where-Object { $_ }).Count -gt 1) { throw 'Choose one test mode.' }
    if ($TestSeconds -gt 0 -and !$Playthrough) { throw '-TestSeconds requires -Playthrough.' }
    if ($Capture -and ($Gamepad -or $Grab -or $LocalClimbing)) { throw '-Capture requires route climbing, smoke or playthrough tests.' }
    $ResolvedEngine = Find-Engine
    $BuildTool = Join-Path $ResolvedEngine 'Engine\Build\BatchFiles\Build.bat'
    $EditorExe = Join-Path $ResolvedEngine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $EditorCmd = Join-Path $ResolvedEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $VersionFile = Join-Path $ResolvedEngine 'Engine\Build\Build.version'
    $Version = Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json
    if ($Version.MajorVersion -ne 5 -or $Version.MinorVersion -notin @(4, 5, 6)) {
        throw "This prototype targets UE 5.4-5.6; found $($Version.MajorVersion).$($Version.MinorVersion)."
    }
    if (!(Test-Path -LiteralPath $EditorCmd)) { throw "UnrealEditor-Cmd.exe is missing from $ResolvedEngine. Finish the engine installation first." }
    Write-Host "Engine: $ResolvedEngine"
    Write-Host "Project: $ProjectFile"

    if ($Action -eq 'Check') {
        $VSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (!(Test-Path -LiteralPath $VSWhere)) { throw 'Visual Studio C++ tools not found. See README.md for installation requirements.' }
        $VSPath = & $VSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (!$VSPath) { throw 'No Visual Studio installation with the x64 C++ toolchain was found.' }
        Write-Host "Visual Studio C++: $VSPath"
        Write-Host 'Engine and compiler detected. Run Setup to validate UBT, SDK and compilation.'
        exit 0
    }

    if (!$SkipBuild) { Build-Editor }
    if ($Action -eq 'Build') { exit 0 }
    if ($Action -eq 'CodeDatabase') {
        Invoke-Checked $BuildTool @('IshibashiriPrototypeEditor', 'Win64', 'Development', "-Project=$ProjectFile", '-Mode=GenerateClangDatabase', '-Compiler=VisualStudio2022', '-DisableUnity', "-OutputDir=$ProjectRoot")
        $DatabaseFile = Join-Path $ProjectRoot 'compile_commands.json'
        if (!(Test-Path -LiteralPath $DatabaseFile)) { throw 'UnrealBuildTool did not generate compile_commands.json.' }
        $Commands = @(Get-Content -LiteralPath $DatabaseFile -Raw | ConvertFrom-Json)
        if ($Commands.Count -eq 0) { throw 'The generated compilation database is empty.' }
        if (!($Commands | Where-Object { $_.file -match 'PrototypePlayer\.cpp$' })) {
            throw 'The compilation database does not contain the game sources. Check the UE target and project path.'
        }
        Write-Host "C++ completion database: $DatabaseFile"
        exit 0
    }
    Ensure-Map
    switch ($Action) {
        'ImportModels' {
            $ScriptFile = Join-Path $PSScriptRoot 'ImportFreeModels.py'
            $ImportLog = Join-Path $ProjectRoot ('Saved\Logs\ImportModels-' + [guid]::NewGuid().ToString('N') + '.log')
            Invoke-Checked $EditorCmd @($ProjectFile, '-run=pythonscript', "-script=$ScriptFile", '-unattended', '-nop4', '-nullrhi', '-nosound', '-UTF8Output', "-abslog=$ImportLog")
            if (!(Select-String -LiteralPath $ImportLog -SimpleMatch 'FREE_MODELS_IMPORT_PASS' -Quiet)) {
                throw "Model import did not report success. Read $ImportLog"
            }
        }
        'Setup' { Write-Host 'Editor build and prototype map are ready.' }
        'Play' {
            # This is the visible game explicitly requested by the Play action.
            & $EditorExe $ProjectFile '/Game/Maps/L_Prototype_01' '-game' '-windowed' '-ResX=1280' '-ResY=800' '-NoSplash'
        }
        'Editor' { & $EditorExe $ProjectFile '/Game/Maps/L_Prototype_01' }
        'Test' {
            $LogDir = Join-Path $ProjectRoot 'Saved\Logs'
            New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
            $LogName = if ($Gamepad) { "PrototypeGamepad-$TestFPS.log" } elseif ($ClimbingGamepad) { "ClimbingGamepad-$TestFPS.log" } elseif ($Climbing) { "ClimbingTest-$TestFPS.log" } elseif ($LocalClimbing) { "PrototypeClimbing-$TestFPS.log" } elseif ($Grab) { "PrototypeGrab-$TestFPS.log" } elseif ($Playthrough) { "PrototypePlaythrough-$TestFPS.log" } elseif ($Capture) { "PrototypeVisual-$TestFPS.log" } else { "PrototypeSmoke-$TestFPS.log" }
            $LogFile = Join-Path $LogDir $LogName
            $RunId = [guid]::NewGuid().ToString('N')
            $TestFlag = if ($Gamepad) { '-PrototypeGamepadTest' } elseif ($Climbing -or $ClimbingGamepad) { '-ClimbingTest' } elseif ($LocalClimbing) { '-PrototypeClimbingTest' } elseif ($Grab) { '-PrototypeGrabTest' } elseif ($Playthrough) { '-PrototypePlaythrough' } else { '-PrototypeSmokeTest' }
            $TestArguments = @($ProjectFile, '/Game/Maps/L_Prototype_01', '-game', '-nosound', '-unattended', '-nop4', $TestFlag, "-PrototypeTestRun=$RunId", "-PrototypeTestFPS=$TestFPS", "-PrototypeTestSeconds=$TestSeconds", "-abslog=$LogFile")
            if ($ClimbingGamepad) { $TestArguments += '-ClimbingGamepad' }
            if ($Capture) {
                $TestArguments += @('-PrototypeCapture', '-RenderOffscreen', '-windowed', '-ResX=1280', '-ResY=800', '-ExecCmds=t.MaxFPS 60')
            } else {
                $TestArguments += '-nullrhi'
            }
            Invoke-Checked $EditorCmd $TestArguments
            $PassMarker = if ($Climbing -or $ClimbingGamepad) { "CLIMB_TEST_PASS $RunId" } else { "PROTOTYPE_TEST_PASS $RunId" }
            if (!(Select-String -LiteralPath $LogFile -SimpleMatch $PassMarker -Quiet)) {
                throw "Smoke test did not report success for this run. Read $LogFile"
            }
            Write-Host "Test passed: $LogFile"
            if ($Capture) {
                $CaptureDir = if ($Climbing -or $ClimbingGamepad) { Join-Path $ProjectRoot "Saved\Screenshots\Climbing\$RunId" } else { Join-Path $ProjectRoot "Saved\Screenshots\Prototype\$RunId" }
                $ShotNames = if ($Climbing -or $ClimbingGamepad) { @('01-Ground','02-FirstLedge','03-ShoulderCore','04-Summit','05-Victory','06-ThrownOff') } elseif ($Playthrough) { @('08-InputVictory') } else { @('01-Dodge', '02-Telegraph', '03-Counter', '04-Victory', '05-Defeat', '06-WallCamera', '07-BossCamera') }
                foreach ($Name in $ShotNames) {
                    $Shot = Get-Item -LiteralPath (Join-Path $CaptureDir "$Name.png") -ErrorAction Stop
                    if ($Shot.Length -lt 100) { throw "Screenshot is empty: $($Shot.FullName)" }
                }
                Write-Host "Rendered captures: $CaptureDir"
            }
        }
        'Package' {
            $UAT = Join-Path $ResolvedEngine 'Engine\Build\BatchFiles\RunUAT.bat'
            $OutputDir = Join-Path $ProjectRoot 'Artifacts\Windows'
            Invoke-Checked $UAT @('BuildCookRun', "-project=$ProjectFile", '-noP4', '-platform=Win64', '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-archive', "-archivedirectory=$OutputDir", '-utf8output')
            Write-Host "Packaged game: $OutputDir"
        }
    }
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
