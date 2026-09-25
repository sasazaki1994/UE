[CmdletBinding()]
param(
    [ValidateSet('Check', 'Build', 'Setup', 'Play', 'Editor', 'Test', 'Package', 'CodeDatabase', 'ImportModels')]
    [string]$Action = 'Play',
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipBuild,
    [switch]$Capture,
    [switch]$Campaign,
    [switch]$IshibashiriDemo,
    [switch]$Approach,
    [switch]$Basin,
    [switch]$Fuchimatoi,
    [switch]$Minedaki,
    [switch]$Magatsune,
    [switch]$Recovery,
    [switch]$Realtime,
    [switch]$Onscreen,
    [switch]$HighQuality,
    [ValidateSet(0, 1)][int]$ProductionVisuals = 0,
    [switch]$BasinScenario,
    [switch]$Playthrough,
    [switch]$Camera,
    [switch]$Grab,
    [switch]$Climbing,
    [switch]$ClimbingIK,
    [switch]$GrabMotionWarp,
    [switch]$LocalClimbing,
    [switch]$ClimbingGamepad,
    [switch]$Gamepad,
    [ValidateRange(0, 3600)][int]$TestSeconds = 0,
    [ValidateRange(15, 240)][int]$TestFPS = 60,
    # Zero selects a limit based on the simulated duration and capture frame rate.
    [ValidateRange(0, 86400)][int]$TestTimeoutSeconds = 0
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

function ConvertTo-NativeArgument {
    param([AllowEmptyString()][string]$Argument)
    if ($Argument.Length -gt 0 -and $Argument -notmatch '[\s"]') { return $Argument }
    # Start-Process joins ArgumentList verbatim. Escape quotes and trailing
    # backslashes according to Windows argv rules, including paths with spaces.
    $Escaped = [regex]::Replace($Argument, '(\\*)"', '$1$1\"')
    $Escaped = [regex]::Replace($Escaped, '(\\+)$', '$1$1')
    return '"' + $Escaped + '"'
}

function Get-TestTimeoutSeconds {
    param([string]$TestFlag, [bool]$IsCapture, [int]$Seconds, [int]$FPS)
    # Use in-game watchdogs where available, with conservative budgets for
    # Approach's current 1.31 km route (about 656 seconds) and Campaign's route
    # plus four encounters (300 + 360 + 300 + 240), cards and level transitions.
    $SimulatedLimit = switch ($TestFlag) {
        '-CampaignE2E' { 2400 }
        '-ApproachTest' { 720 }
        '-FuchimatoiTest' { 360 }
        '-MinedakiTest' { 300 }
        '-MagatsuneTest' { 240 }
        '-ClimbingTest' { 300 }
        '-BasinPlaythroughTest' { 74 } # Sum of the ten non-looping phase limits.
        '-PrototypePlaythrough' { [Math]::Max(180, $Seconds + 90) }
        default { 65 }
    }
    # Captures can be capped at 60 rendered FPS while simulation uses a higher
    # fixed FPS. Allow startup time and twice the expected run duration.
    $CaptureFactor = if ($IsCapture) { [Math]::Max(1.0, $FPS / 60.0) } else { 1.0 }
    return [int][Math]::Ceiling([Math]::Max(300.0, 120.0 + 2.0 * $SimulatedLimit * $CaptureFactor))
}

function Invoke-TimedTest {
    param([string]$Program, [string[]]$Arguments, [int]$TimeoutSeconds, [string]$LogFile, [switch]$ShowWindow)
    $CommandLine = ($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' '
    # -Onscreen explicitly requests a visible realtime test window.
    $WindowStyle = if ($ShowWindow) { 'Normal' } else { 'Hidden' }
    $Process = Start-Process -FilePath $Program -ArgumentList $CommandLine -PassThru -WindowStyle $WindowStyle
    try {
        # Retain the process handle so Windows PowerShell 5.1 can read ExitCode
        # even when a short-lived process exits before WaitForExit is called.
        $null = $Process.Handle
        if (!$Process.WaitForExit($TimeoutSeconds * 1000)) {
            try { $Process.Kill() } catch { if (!$Process.HasExited) { throw } }
            $null = $Process.WaitForExit(5000)
            throw "Test timed out after $TimeoutSeconds real seconds (PID $($Process.Id)). Read $LogFile"
        }
        if ($Process.ExitCode -ne 0) { throw "$Program failed (exit $($Process.ExitCode)). Read $LogFile" }
    } finally {
        $Process.Dispose()
    }
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
    if (@(@($Campaign,$IshibashiriDemo,$Approach,$Minedaki,$Magatsune,$Fuchimatoi) | Where-Object { $_ }).Count -gt 1) { throw 'Choose -Campaign, -IshibashiriDemo, -Approach or one encounter.' }
    $IsCampaignFlow = $Campaign -or $IshibashiriDemo
    if ($IsCampaignFlow -and ($Basin -or $Recovery -or $Realtime -or $Onscreen -or $BasinScenario -or $Playthrough -or $Camera -or $Grab -or $Climbing -or $ClimbingIK -or $GrabMotionWarp -or $LocalClimbing -or $ClimbingGamepad)) { throw 'Campaign flows cannot be combined with encounter/test scenario switches other than -Gamepad.' }
    if ($Magatsune -and ($Basin -or $Recovery -or $Realtime -or $BasinScenario -or $Playthrough -or $Camera -or $Grab -or $Climbing -or $ClimbingIK -or $GrabMotionWarp -or $LocalClimbing -or $ClimbingGamepad)) { throw '-Magatsune is a separate encounter. Use -Gamepad for its gamepad playthrough.' }
    if ($Minedaki -and ($Fuchimatoi -or $Basin -or $Recovery -or $Realtime -or $BasinScenario -or $Playthrough -or $Camera -or $Grab -or $Climbing -or $ClimbingIK -or $GrabMotionWarp -or $LocalClimbing -or $ClimbingGamepad)) { throw '-Minedaki is a separate encounter. Use -Gamepad for its gamepad playthrough.' }
    if ($Onscreen -and !$Realtime) { throw '-Onscreen requires -Realtime.' }
    if ($Realtime -and (!$Fuchimatoi -or $Action -ne 'Test' -or $Capture)) { throw '-Realtime requires -Action Test -Fuchimatoi without -Capture (screenshots distort frame timing).' }
    if ($Recovery -and (!$Fuchimatoi -or $Action -ne 'Test')) { throw '-Recovery requires -Action Test -Fuchimatoi.' }
    if ($Fuchimatoi -and ($Basin -or $BasinScenario -or $Playthrough -or $Camera -or $Grab -or $Climbing -or $ClimbingIK -or $GrabMotionWarp -or $LocalClimbing -or $ClimbingGamepad)) { throw '-Fuchimatoi is a separate encounter. Use -Gamepad for its gamepad playthrough.' }
    $LaunchMap = '/Game/Maps/L_Prototype_01'
    if ($IsCampaignFlow) { $LaunchMap += '?game=/Script/IshibashiriPrototype.CampaignGameMode' }
    if ($Approach) { $LaunchMap += '?game=/Script/IshibashiriPrototype.IshibashiriApproachGameMode' }
    if ($Fuchimatoi) { $LaunchMap += '?game=/Script/IshibashiriPrototype.FuchimatoiGameMode' }
    if ($Minedaki) { $LaunchMap += '?game=/Script/IshibashiriPrototype.MinedakiGameMode' }
    if ($Magatsune) { $LaunchMap += '?game=/Script/IshibashiriPrototype.MagatsuneGameMode' }
    if (($Camera -or $Playthrough -or $Grab -or $Climbing -or $ClimbingIK -or $GrabMotionWarp -or $LocalClimbing -or $ClimbingGamepad -or $Gamepad -or $BasinScenario -or $TestSeconds -gt 0) -and $Action -ne 'Test') { throw 'Test switches require -Action Test.' }
    if (@(@($Camera, $Playthrough, $Grab, $Climbing, $ClimbingIK, $GrabMotionWarp, $LocalClimbing, $ClimbingGamepad, $Gamepad, $BasinScenario) | Where-Object { $_ }).Count -gt 1) { throw 'Choose one test mode.' }
    if ($BasinScenario -and !$Basin) { throw '-BasinScenario requires -Basin.' }
    if ($TestSeconds -gt 0 -and !$Playthrough) { throw '-TestSeconds requires -Playthrough.' }
    if ($Capture -and (($Gamepad -and !$Fuchimatoi -and !$Minedaki -and !$Magatsune) -or $Grab -or $LocalClimbing)) { throw '-Capture requires route climbing, smoke or playthrough tests.' }
    if ($Capture -and $Approach) { throw '-Approach has no screenshot set yet (see Docs/IshibashiriApproachSlice.md); run it without -Capture.' }
    if ($TestTimeoutSeconds -gt 0 -and $Action -ne 'Test') { throw '-TestTimeoutSeconds requires -Action Test.' }
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
            $PlayArguments = @($ProjectFile, $LaunchMap, '-game', '-windowed', '-ResX=1280', '-ResY=800', '-NoSplash')
            $PlayArguments += "-ProductionVisuals=$ProductionVisuals"
            if ($Basin) { $PlayArguments += '-BasinPrototype' }
            if ($Campaign) { $PlayArguments += '-Campaign' }
            if ($IshibashiriDemo) { $PlayArguments += '-IshibashiriDemo' }
            if ($HighQuality) { $PlayArguments += @('-d3d12', '-sm6', '-ExecCmds=r.DynamicGlobalIlluminationMethod 1,r.ReflectionMethod 1,r.Shadow.Virtual.Enable 1,r.VolumetricFog 1,r.BloomQuality 4,r.DefaultFeature.AutoExposure 1') }
            Invoke-Checked $EditorExe $PlayArguments
        }
        'Editor' {
            $EditorArguments = @($ProjectFile, $LaunchMap)
            $EditorArguments += "-ProductionVisuals=$ProductionVisuals"
            if ($Basin) { $EditorArguments += '-BasinPrototype' }
            if ($HighQuality) { $EditorArguments += @('-d3d12', '-sm6', '-ExecCmds=r.DynamicGlobalIlluminationMethod 1,r.ReflectionMethod 1,r.Shadow.Virtual.Enable 1,r.VolumetricFog 1,r.BloomQuality 4,r.DefaultFeature.AutoExposure 1') }
            Invoke-Checked $EditorExe $EditorArguments
        }
        'Test' {
            $LogDir = Join-Path $ProjectRoot 'Saved\Logs'
            New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
            $LogName = if ($GrabMotionWarp) { "GrabMotionWarp-$TestFPS.log" } elseif ($BasinScenario) { "BasinScenario-$TestFPS.log" } elseif ($Camera) { "PrototypeCamera-$TestFPS.log" } elseif ($Gamepad) { "PrototypeGamepad-$TestFPS.log" } elseif ($ClimbingGamepad) { "ClimbingGamepad-$TestFPS.log" } elseif ($ClimbingIK) { "ClimbingIK-$TestFPS.log" } elseif ($Climbing) { "ClimbingTest-$TestFPS.log" } elseif ($LocalClimbing) { "PrototypeClimbing-$TestFPS.log" } elseif ($Grab) { "PrototypeGrab-$TestFPS.log" } elseif ($Playthrough) { "PrototypePlaythrough-$TestFPS.log" } elseif ($Capture) { "PrototypeVisual-$TestFPS.log" } else { "PrototypeSmoke-$TestFPS.log" }
            if ($Fuchimatoi) { $LogName = if ($Gamepad) { "FuchimatoiGamepad-$TestFPS.log" } else { "Fuchimatoi-$TestFPS.log" } }
            if ($Minedaki) { $LogName = if ($Gamepad) { "MinedakiGamepad-$TestFPS.log" } else { "Minedaki-$TestFPS.log" } }
            if ($Magatsune) { $LogName = if ($Gamepad) { "MagatsuneGamepad-$TestFPS.log" } else { "Magatsune-$TestFPS.log" } }
            if ($Campaign) { $LogName = "Campaign-$TestFPS.log" }
            if ($IshibashiriDemo) { $LogName = "IshibashiriDemo-$TestFPS.log" }
            if ($Recovery) { $LogName = if ($Gamepad) { "FuchimatoiRecoveryGamepad-$TestFPS.log" } else { "FuchimatoiRecovery-$TestFPS.log" } }
            if ($Realtime) { $LogName = "Realtime-$LogName" }
            if ($Onscreen) { $LogName = "Onscreen-$LogName" }
            $LogFile = Join-Path $LogDir $LogName
            $RunId = [guid]::NewGuid().ToString('N')
            $TestFlag = if ($BasinScenario) { '-BasinPlaythroughTest' } elseif ($Camera) { '-PrototypeCameraTest' } elseif ($Gamepad) { '-PrototypeGamepadTest' } elseif ($Climbing -or $ClimbingGamepad -or $ClimbingIK -or $GrabMotionWarp) { '-ClimbingTest' } elseif ($LocalClimbing) { '-PrototypeClimbingTest' } elseif ($Grab) { '-PrototypeGrabTest' } elseif ($Playthrough) { '-PrototypePlaythrough' } else { '-PrototypeSmokeTest' }
            if ($Fuchimatoi) { $TestFlag = '-FuchimatoiTest' }
            if ($Minedaki) { $TestFlag = '-MinedakiTest' }
            if ($Magatsune) { $TestFlag = '-MagatsuneTest' }
            if ($IsCampaignFlow) { $TestFlag = '-CampaignE2E' }
            if ($Approach) { $TestFlag = '-ApproachTest' }
            $TestArguments = @($ProjectFile, $LaunchMap, '-game', '-nosound', '-unattended', '-nop4', $TestFlag, "-PrototypeTestRun=$RunId", "-PrototypeTestFPS=$TestFPS", "-PrototypeTestSeconds=$TestSeconds", "-abslog=$LogFile")
            $TestArguments += "-ProductionVisuals=$ProductionVisuals"
            if ($Fuchimatoi -and $Gamepad) { $TestArguments += '-FuchimatoiGamepad' }
            if ($Minedaki -and $Gamepad) { $TestArguments += '-MinedakiGamepad' }
            if ($Magatsune -and $Gamepad) { $TestArguments += '-MagatsuneGamepad' }
            if ($Recovery) { $TestArguments += '-FuchimatoiRecovery' }
            if ($Campaign) { $TestArguments += '-Campaign'; if ($Gamepad) { $TestArguments += '-CampaignGamepad' } }
            if ($IshibashiriDemo) { $TestArguments += '-IshibashiriDemo'; if ($Gamepad) { $TestArguments += '-CampaignGamepad' } }
            if ($Realtime) { $TestArguments += '-FuchimatoiRealtime' }
            if ($Basin) { $TestArguments += '-BasinPrototype' }
            if ($HighQuality) { $TestArguments += @('-d3d12', '-sm6', '-ExecCmds=r.DynamicGlobalIlluminationMethod 1,r.ReflectionMethod 1,r.Shadow.Virtual.Enable 1,r.VolumetricFog 1,r.BloomQuality 4,r.DefaultFeature.AutoExposure 1') }
            if ($ClimbingGamepad) { $TestArguments += '-ClimbingGamepad' }
            if ($ClimbingIK) { $TestArguments += @('-ClimbingIKTest', '-ClimbingIKDebug') }
            if ($GrabMotionWarp) { $TestArguments += @('-GrabMotionWarpTest', '-GrabWarpDebug') }
            if ($Capture) {
                $TestArguments += @('-PrototypeCapture', '-RenderOffscreen', '-windowed', '-ResX=1280', '-ResY=800')
                if (!$HighQuality) { $TestArguments += '-ExecCmds=t.MaxFPS 60' }
            } elseif ($Realtime) {
                $TestArguments += @('-windowed', '-ResX=1280', '-ResY=800')
                if (!$Onscreen) { $TestArguments += '-RenderOffscreen' }
            } else {
                $TestArguments += '-nullrhi'
            }
            $Timeout = if ($TestTimeoutSeconds -gt 0) { $TestTimeoutSeconds } else {
                Get-TestTimeoutSeconds -TestFlag $TestFlag -IsCapture $Capture -Seconds $TestSeconds -FPS $TestFPS
            }
            Write-Host "Test timeout: $Timeout real seconds"
            Invoke-TimedTest -Program $EditorCmd -Arguments $TestArguments -TimeoutSeconds $Timeout -LogFile $LogFile -ShowWindow:$Onscreen
            $PassMarker = if ($GrabMotionWarp) { "GRAB_MOTION_WARP_TEST_PASS $RunId" } elseif ($BasinScenario) { "BASIN_SCENARIO_PASS $RunId" } elseif ($ClimbingIK) { "CLIMBING_IK_TEST_PASS $RunId" } elseif ($Climbing -or $ClimbingGamepad) { "CLIMB_TEST_PASS $RunId" } else { "PROTOTYPE_TEST_PASS $RunId" }
            if ($Fuchimatoi) { $PassMarker = "FUCHIMATOI_TEST_PASS $RunId" }
            if ($Minedaki) { $PassMarker = "MINEDAKI_TEST_PASS $RunId" }
            if ($Magatsune) { $PassMarker = "MAGATSUNE_TEST_PASS $RunId" }
            if ($Campaign) { $PassMarker = "CAMPAIGN_E2E_PASS $RunId" }
            if ($IshibashiriDemo) { $PassMarker = "ISHIBASHIRI_DEMO_E2E_PASS $RunId" }
            if ($Approach) { $PassMarker = "APPROACH_TEST_PASS $RunId" }
            if (!(Select-String -LiteralPath $LogFile -SimpleMatch $PassMarker -Quiet)) {
                throw "Smoke test did not report success for this run. Read $LogFile"
            }
            if ($IsCampaignFlow -and (Select-String -LiteralPath $LogFile -Pattern 'CAMPAIGN_E2E_FAIL|ISHIBASHIRI_DEMO_E2E_FAIL|_TEST_FAIL|Result=\{Fail\}|Test Failed' -Quiet)) { throw "Campaign flow E2E reported a failure. Read $LogFile" }
            Write-Host "Test passed: $LogFile"
            if ($Realtime -and !(Select-String -LiteralPath $LogFile -SimpleMatch "FUCHIMATOI_PERF $RunId" -Quiet)) { throw "Realtime test did not record performance. Read $LogFile" }
            if ($Capture) {
                if ($IshibashiriDemo) {
                    $PlannedDemoShots = @('01-Title','02-Prologue','03-Approach','04-IshibashiriReveal','05-Charge','06-GroundGrab','07-Climbing','08-Kakon1','09-Kakon2','10-Kakon3','11-Calm','12-DemoEnding1','13-DemoEnding2','14-ReturnToTitle')
                    Write-Host "Ishibashiri demo capture destination: Saved\Screenshots\IshibashiriDemo\$RunId"
                    Write-Host "Capture contract: $($PlannedDemoShots -join ', ')"
                    exit 0
                }
                if ($Campaign) {
                    $RequiredCampaignShots = @(
                        "Campaign\$RunId\01-Title.png", "Campaign\$RunId\02-Prologue.png",
                        "Climbing\$RunId\01-Ground.png", "Climbing\$RunId\05-Victory.png",
                        "Campaign\$RunId\05-Interlude1.png", "Fuchimatoi\$RunId\01-BiteWindup.png",
                        "Fuchimatoi\$RunId\10-Victory.png", "Campaign\$RunId\08-Interlude2.png",
                        "Minedaki\$RunId\01-GroundGrab.png", "Minedaki\$RunId\12-Calm.png",
                        "Campaign\$RunId\11-Interlude3.png", "Magatsune\$RunId\01-Arrival.png",
                        "Magatsune\$RunId\11-Calm.png", "Campaign\$RunId\14-Ending.png",
                        "Campaign\$RunId\15-Completed.png"
                    )
                    foreach ($Relative in $RequiredCampaignShots) {
                        $Shot = Get-Item -LiteralPath (Join-Path $ProjectRoot "Saved\Screenshots\$Relative") -ErrorAction Stop
                        if ($Shot.Length -lt 100) { throw "Screenshot is empty: $($Shot.FullName)" }
                    }
                    Write-Host "Rendered campaign captures: Saved\Screenshots\Campaign\$RunId and encounter directories"
                    exit 0
                }
                $CaptureDir = if ($GrabMotionWarp) { Join-Path $ProjectRoot "Saved\Screenshots\GrabMotionWarp\$RunId" } elseif ($BasinScenario) { Join-Path $ProjectRoot "Saved\Screenshots\Basin\$RunId" } elseif ($ClimbingIK) { Join-Path $ProjectRoot "Saved\Screenshots\ClimbingIK\$RunId\After" } elseif ($Climbing -or $ClimbingGamepad) { Join-Path $ProjectRoot "Saved\Screenshots\Climbing\$RunId" } else { Join-Path $ProjectRoot "Saved\Screenshots\Prototype\$RunId" }
                $ShotNames = if ($GrabMotionWarp) { @('01-BeforeGrab','02-WarpStart','03-Approach','04-BeforeContact','05-Attached','06-ClimbStart') } elseif ($BasinScenario) { @('01-Start','02-BeforeMount','03-FirstLedge','04-Landed','05-Retry') } elseif ($ClimbingIK) { @('01-Grab','02-ForelegClimb','03-HandsContact','04-FeetContact','05-FirstShoulder','06-BossMoving','07-ShakeCling') } elseif ($Camera) { @('06-WallCamera', '07-BossCamera', '09-CameraReturn') } elseif ($Climbing -or $ClimbingGamepad) { @('01-Ground','02-FirstLedge','03-ShoulderCore','04-Summit','05-Victory','06-ThrownOff') } elseif ($Playthrough) { @('08-InputVictory') } else { @('01-Dodge', '02-Telegraph', '03-Counter', '04-Victory', '05-Defeat', '06-WallCamera', '07-BossCamera', '09-CameraReturn') }
                if ($Fuchimatoi) {
                    $CaptureDir = Join-Path $ProjectRoot "Saved\Screenshots\Fuchimatoi\$RunId"
                    $ShotNames = @('01-BiteWindup','02-BiteLunge','03-Snagged','04-HeadGrab','05-Kakon1','06-Coiling','07-SnakeToRock','08-Kakon2','09-FinalClimb','10-Victory')
                    if ($Recovery) { $ShotNames += @('11-IntentionalFall','12-RecoveryPoint','13-RecoveryGrab','14-RouteRecovered','15-SecondFall') }
                }
                if ($Minedaki) {
                    $CaptureDir = Join-Path $ProjectRoot "Saved\Screenshots\Minedaki\$RunId"
                    $ShotNames = @('01-GroundGrab','02-Phase1WallClimb','03-FirstShake','04-Kakon1','05-Phase2BodyTransition','06-ArmRoutePlatform','07-Kakon2','08-Phase3Transition','09-FinalRoute','10-FinalCling','11-Kakon3','12-Calm','13-Victory','14-Fall','15-Recovery','16-Retry')
                }
                if ($Magatsune) {
                    $CaptureDir = Join-Path $ProjectRoot "Saved\Screenshots\Magatsune\$RunId"
                    $ShotNames = @('01-Arrival','02-RootMovement','03-FirstGrab','04-Kakon1','05-Phase2Opening','06-RootRockTransition','07-Kakon2','08-FinalRootRise','09-FinalCling','10-Kakon3','11-Calm','12-Victory')
                }
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
    # Start-Process does not update LASTEXITCODE. Explicitly report success to a
    # PowerShell caller, including when its previous native command failed.
    exit 0
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
