[CmdletBinding()]
param()

# Runs without Unreal Engine. Keep this compatible with Play.cmd's PowerShell 5.1.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$SourceScript = Join-Path $PSScriptRoot 'Prototype.ps1'
$Tokens = $null
$ParseErrors = $null
$Ast = [System.Management.Automation.Language.Parser]::ParseFile($SourceScript, [ref]$Tokens, [ref]$ParseErrors)
if ($ParseErrors.Count) { throw ($ParseErrors | Out-String) }
foreach ($Function in $Ast.FindAll({ param($Node) $Node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
    if ($Function.Name -in @('ConvertTo-NativeArgument', 'Get-TestTimeoutSeconds', 'Invoke-TimedTest')) {
        . ([scriptblock]::Create($Function.Extent.Text))
    }
}

$FixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('Prototype Tool Tests ' + [guid]::NewGuid().ToString('N'))
$PriorMode = $env:PROTOTYPE_TOOL_FIXTURE_MODE
$Unrelated = $null
try {
    $ProjectRoot = Join-Path $FixtureRoot 'Project With Spaces'
    $FakeEngine = Join-Path $FixtureRoot 'Engine With Spaces'
    $BinDir = Join-Path $FakeEngine 'Engine\Binaries\Win64'
    $BuildDir = Join-Path $FakeEngine 'Engine\Build'
    foreach ($Directory in @($BinDir, (Join-Path $BuildDir 'BatchFiles'), (Join-Path $ProjectRoot 'Tools'), (Join-Path $ProjectRoot 'Content\Maps'))) {
        New-Item -ItemType Directory -Path $Directory -Force | Out-Null
    }
    $ScriptCopy = Join-Path $ProjectRoot 'Tools\Prototype.ps1'
    Copy-Item -LiteralPath $SourceScript -Destination $ScriptCopy
    Set-Content -LiteralPath (Join-Path $ProjectRoot 'IshibashiriPrototype.uproject') -Value '{}'
    Set-Content -LiteralPath (Join-Path $ProjectRoot 'Content\Maps\L_Prototype_01.umap') -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $BuildDir 'Build.version') -Value '{"MajorVersion":5,"MinorVersion":6}'
    Set-Content -LiteralPath (Join-Path $BuildDir 'BatchFiles\Build.bat') -Value '@exit /b 0'
    $FixtureExe = Join-Path $BinDir 'UnrealEditor-Cmd.exe'
    Add-Type -OutputAssembly $FixtureExe -OutputType ConsoleApplication -TypeDefinition @'
using System;
using System.IO;
using System.Diagnostics;
using System.Threading;
public static class PrototypeToolFixture {
    public static int Main(string[] args) {
        if (args.Length > 0 && args[0] == "--wait") { Thread.Sleep(60000); return 0; }
        if (args.Length > 1 && args[0] == "--record") {
            File.WriteAllLines(args[1], args);
            return 0;
        }
        string log = null, run = null, marker = "PROTOTYPE_TEST_PASS";
        File.WriteAllLines(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "last-args.txt"), args);
        foreach (string arg in args) {
            if (arg.StartsWith("-abslog=")) log = arg.Substring(8);
            if (arg.StartsWith("-PrototypeTestRun=")) run = arg.Substring(18);
            if (arg == "-CampaignE2E") marker = "CAMPAIGN_E2E_PASS";
            if (arg == "-ApproachTest") marker = "APPROACH_TEST_PASS";
            if (arg == "-FuchimatoiTest") marker = "FUCHIMATOI_TEST_PASS";
            if (arg == "-MinedakiTest") marker = "MINEDAKI_TEST_PASS";
            if (arg == "-MagatsuneTest") marker = "MAGATSUNE_TEST_PASS";
            if (arg == "-BasinPlaythroughTest") marker = "BASIN_SCENARIO_PASS";
            if (arg == "-ClimbingTest") marker = "CLIMB_TEST_PASS";
            if (arg == "-ClimbingIKTest") marker = "CLIMBING_IK_TEST_PASS";
            if (arg == "-GrabMotionWarpTest") marker = "GRAB_MOTION_WARP_TEST_PASS";
        }
        string mode = Environment.GetEnvironmentVariable("PROTOTYPE_TOOL_FIXTURE_MODE");
        if (log != null) {
            File.WriteAllText(log + ".pid", Process.GetCurrentProcess().Id.ToString());
            if (mode == "wrong-marker") marker = "PROTOTYPE_TEST_PASS";
            File.WriteAllText(log, mode == "missing-pass" ? "No success marker" : marker + " " + run);
            if (Array.IndexOf(args, "-FuchimatoiRealtime") >= 0) File.AppendAllText(log, "\nFUCHIMATOI_PERF " + run);
        }
        if (mode == "hang") Thread.Sleep(60000);
        return mode == "failure" ? 17 : 0;
    }
}
'@
    Copy-Item -LiteralPath $FixtureExe -Destination (Join-Path $BinDir 'UnrealEditor.exe')
    $WindowsPowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'

    function Invoke-FixtureCase {
        param([string]$Name, [string]$Action, [string]$Mode, [int]$ExpectedExit, [string]$ExpectedOutput,
            [string]$EntryScript = $ScriptCopy, [string[]]$ExtraArguments = @(), [switch]$AutomaticTimeout)
        $env:PROTOTYPE_TOOL_FIXTURE_MODE = $Mode
        $Arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $EntryScript,
            '-Action', $Action, '-SkipBuild', '-EngineRoot', $FakeEngine)
        if ($Action -eq 'Test' -and !$AutomaticTimeout) { $Arguments += @('-TestTimeoutSeconds', '2') }
        $Arguments += $ExtraArguments
        $CommandLine = ($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' '
        $OutputFile = Join-Path $FixtureRoot ($Name + '.out.txt')
        $ErrorFile = Join-Path $FixtureRoot ($Name + '.err.txt')
        $Process = Start-Process -FilePath $WindowsPowerShell -ArgumentList $CommandLine -PassThru -WindowStyle Hidden `
            -RedirectStandardOutput $OutputFile -RedirectStandardError $ErrorFile
        try {
            $null = $Process.Handle
            if (!$Process.WaitForExit(15000)) { $Process.Kill(); throw "Fixture $Name exceeded its own timeout" }
            $Output = (Get-Content -LiteralPath $OutputFile -Raw) + (Get-Content -LiteralPath $ErrorFile -Raw)
            if ($Process.ExitCode -ne $ExpectedExit -or $Output -notmatch $ExpectedOutput) {
                throw "Fixture $Name failed: exit=$($Process.ExitCode), expected=$ExpectedExit`n$Output"
            }
        } finally { $Process.Dispose() }
        Write-Host "PASS $Name"
    }

    # The process receiving the timeout must be killed; another process running
    # the very same executable must remain alive.
    $Unrelated = Start-Process -FilePath $FixtureExe -ArgumentList '--wait' -PassThru -WindowStyle Hidden
    Invoke-FixtureCase 'test-success' 'Test' 'success' 0 'Test passed:'
    Invoke-FixtureCase 'test-nonzero-with-pass-marker' 'Test' 'failure' 1 'exit 17'
    Invoke-FixtureCase 'test-missing-pass-marker' 'Test' 'missing-pass' 1 'did not report success'
    Invoke-FixtureCase 'test-timeout' 'Test' 'hang' 1 'timed out after 2 real seconds'
    $TestPIDFile = Join-Path $ProjectRoot 'Saved\Logs\PrototypeSmoke-60.log.pid'
    $TimedOutPID = [int](Get-Content -LiteralPath $TestPIDFile -Raw)
    if (Get-Process -Id $TimedOutPID -ErrorAction SilentlyContinue) { throw 'Timed-out test is still running' }
    if ($Unrelated.HasExited) { throw 'Timeout terminated an unrelated process' }
    Invoke-FixtureCase 'play-nonzero' 'Play' 'failure' 1 'exit 17'
    Invoke-FixtureCase 'editor-nonzero' 'Editor' 'failure' 1 'exit 17'
    Invoke-FixtureCase 'play-success' 'Play' 'success' 0 'Engine:'
    Invoke-FixtureCase 'editor-success' 'Editor' 'success' 0 'Engine:'

    $CallerScript = Join-Path $FixtureRoot 'Repeated Caller.ps1'
    Set-Content -LiteralPath $CallerScript -Value @'
param([string]$Action, [switch]$SkipBuild, [string]$EngineRoot, [int]$TestTimeoutSeconds)
$ErrorActionPreference = 'Stop'
$Prototype = Join-Path $PSScriptRoot 'Project With Spaces\Tools\Prototype.ps1'
$Completed = 0
foreach ($FrameRate in @(30, 120)) {
    & $env:ComSpec /c exit 17
    if ($LASTEXITCODE -ne 17) { throw 'Could not prepare a stale native exit code' }
    & $Prototype -Action $Action -SkipBuild:$SkipBuild -EngineRoot $EngineRoot `
        -TestFPS $FrameRate -TestTimeoutSeconds $TestTimeoutSeconds
    if ($LASTEXITCODE -ne 0) { throw "Successful test left LASTEXITCODE=$LASTEXITCODE" }
    ++$Completed
}
if ($Completed -ne 2) { throw 'The script exited its caller instead of returning' }
Write-Host 'PASS repeated caller receives zero after both tests'
'@
    Invoke-FixtureCase 'same-shell-success-code' 'Test' 'success' 0 'PASS repeated caller receives zero' $CallerScript

    $ScenarioCases = @(
        @{ Name = 'campaign'; Arguments = @('-Campaign'); Timeout = 4920 },
        @{ Name = 'approach'; Arguments = @('-Approach'); Timeout = 1560 },
        @{ Name = 'fuchimatoi-recovery-realtime'; Arguments = @('-Fuchimatoi', '-Recovery', '-Realtime'); Timeout = 840 },
        @{ Name = 'minedaki-gamepad'; Arguments = @('-Minedaki', '-Gamepad'); Timeout = 720 },
        @{ Name = 'magatsune-gamepad'; Arguments = @('-Magatsune', '-Gamepad'); Timeout = 600 },
        @{ Name = 'climbing'; Arguments = @('-Climbing'); Timeout = 720 },
        @{ Name = 'climbing-ik'; Arguments = @('-ClimbingIK'); Timeout = 720 },
        @{ Name = 'grab-motion-warp'; Arguments = @('-GrabMotionWarp'); Timeout = 720 },
        @{ Name = 'basin-scenario'; Arguments = @('-Basin', '-BasinScenario'); Timeout = 300 }
    )
    foreach ($Case in $ScenarioCases) {
        Invoke-FixtureCase -Name $Case.Name -Action Test -Mode success -ExpectedExit 0 `
            -ExpectedOutput "Test timeout: $($Case.Timeout) real seconds" -ExtraArguments $Case.Arguments -AutomaticTimeout
    }
    Invoke-FixtureCase -Name 'campaign-rejects-wrong-marker' -Action Test -Mode wrong-marker -ExpectedExit 1 `
        -ExpectedOutput 'did not report success' -ExtraArguments @('-Campaign')
    Invoke-FixtureCase -Name 'play-campaign-options' -Action Play -Mode success -ExpectedExit 0 `
        -ExpectedOutput 'Engine:' -ExtraArguments @('-Campaign', '-HighQuality', '-ProductionVisuals', '1')
    $LaunchArgs = @(Get-Content -LiteralPath (Join-Path $BinDir 'last-args.txt'))
    foreach ($Expected in @('/Game/Maps/L_Prototype_01?game=/Script/IshibashiriPrototype.CampaignGameMode',
        '-Campaign', '-ProductionVisuals=1', '-d3d12', '-sm6',
        '-ExecCmds=r.DynamicGlobalIlluminationMethod 1,r.ReflectionMethod 1,r.Shadow.Virtual.Enable 1,r.VolumetricFog 1,r.BloomQuality 4,r.DefaultFeature.AutoExposure 1')) {
        if ($LaunchArgs -cnotcontains $Expected) { throw "Play did not preserve argument: $Expected" }
    }
    Invoke-FixtureCase -Name 'editor-basin-options' -Action Editor -Mode success -ExpectedExit 0 `
        -ExpectedOutput 'Engine:' -ExtraArguments @('-Basin', '-HighQuality', '-ProductionVisuals', '1')
    $LaunchArgs = @(Get-Content -LiteralPath (Join-Path $BinDir 'last-args.txt'))
    foreach ($Expected in @('-BasinPrototype', '-ProductionVisuals=1', '-d3d12', '-sm6')) {
        if ($LaunchArgs -cnotcontains $Expected) { throw "Editor did not preserve argument: $Expected" }
    }

    $RecordFile = Join-Path $FixtureRoot 'argv with spaces.txt'
    $ExpectedArgs = @('--record', $RecordFile, '', 'space value', 'embedded"quote', 'C:\with spaces\', 'backslash\"quote')
    Invoke-TimedTest -Program $FixtureExe -Arguments $ExpectedArgs -TimeoutSeconds 5 -LogFile $RecordFile
    $RecordedArgs = @(Get-Content -LiteralPath $RecordFile)
    if ($RecordedArgs.Count -ne $ExpectedArgs.Count) { throw 'Native argument count changed' }
    for ($Index = 0; $Index -lt $ExpectedArgs.Count; ++$Index) {
        if ($RecordedArgs[$Index] -cne $ExpectedArgs[$Index]) { throw "Native argument $Index changed" }
    }
    Write-Host 'PASS native-argument-quoting'
    if ((Get-TestTimeoutSeconds '-PrototypeSmokeTest' $false 0 60) -ne 300 `
        -or (Get-TestTimeoutSeconds '-PrototypePlaythrough' $false 3600 60) -ne 7500 `
        -or (Get-TestTimeoutSeconds '-PrototypePlaythrough' $true 3600 240) -ne 29640 `
        -or (Get-TestTimeoutSeconds '-CampaignE2E' $true 0 240) -ne 19320) {
        throw 'Automatic timeout does not accommodate the requested duration and capture FPS'
    }
    Write-Host 'PASS automatic-timeout-duration'
    Write-Host 'All prototype tool fixtures passed.'
} finally {
    $env:PROTOTYPE_TOOL_FIXTURE_MODE = $PriorMode
    if ($Unrelated) {
        if (!$Unrelated.HasExited) { $Unrelated.Kill(); $null = $Unrelated.WaitForExit(5000) }
        $Unrelated.Dispose()
    }
    # Remove only the unique fixture directory, after checking its resolved path.
    if (Test-Path -LiteralPath $FixtureRoot) {
        $ResolvedFixture = (Resolve-Path -LiteralPath $FixtureRoot).Path
        $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
        if (!$ResolvedFixture.StartsWith($ResolvedTemp, [StringComparison]::OrdinalIgnoreCase) `
            -or (Split-Path -Leaf $ResolvedFixture) -notmatch '^Prototype Tool Tests [a-f0-9]{32}$') {
            throw "Refusing to remove unexpected fixture path: $ResolvedFixture"
        }
        Remove-Item -LiteralPath $ResolvedFixture -Recurse -Force
    }
}
