[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$UpdateReviewDocument,
    [string]$EvidenceRoot,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Prototype = Join-Path $PSScriptRoot 'Prototype.ps1'
$ProjectFile = Join-Path $ProjectRoot 'IshibashiriPrototype.uproject'
$MapFile = Join-Path $ProjectRoot 'Content\Maps\L_Prototype_01.umap'
$ControlRigFile = Join-Path $ProjectRoot 'Content\Characters\Rigged\Shirotsura\CR_Shirotsura_Climbing.uasset'
$SourceSha = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
if (!$SourceSha) { $SourceSha = 'UNKNOWN' }
$Stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
if (!$EvidenceRoot) { $EvidenceRoot = Join-Path $ProjectRoot "Artifacts\IshibashiriReviewGate\$Stamp-$($SourceSha.Substring(0,[Math]::Min(12,$SourceSha.Length)))" }
$EvidenceRoot = [IO.Path]::GetFullPath($EvidenceRoot)
$Folders = @('logs','screenshots','performance','campaign','approach','climbing','ik','fuchimatoi','minedaki','magatsune','package')
New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null
foreach ($Folder in $Folders) { New-Item -ItemType Directory -Force -Path (Join-Path $EvidenceRoot $Folder) | Out-Null }

$Results = [ordered]@{
    build='NOT_RUN'; approach60='NOT_RUN'; approach30='NOT_RUN'; ishibashiri60='NOT_RUN'
    ishibashiri30='NOT_RUN'; gamepad='NOT_RUN'; retry='NOT_RUN'; senseReset='NOT_RUN'
    controlRig='NOT_RUN'; legacy='NOT_RUN'; highQuality='NOT_RUN'; campaign60='NOT_RUN'
    campaign30='NOT_RUN'; campaignGamepad='NOT_RUN'; package='NOT_RUN'
    fuchimatoi60='NOT_RUN'; fuchimatoi30='NOT_RUN'; fuchimatoiGamepad='NOT_RUN'
    minedaki60='NOT_RUN'; minedaki30='NOT_RUN'; minedakiGamepad='NOT_RUN'
    magatsune60='NOT_RUN'; magatsune30='NOT_RUN'; magatsuneGamepad='NOT_RUN'
}
$Environment = [ordered]@{
    timestampUtc=$Stamp; sourceSha=$SourceSha; platform=[Environment]::OSVersion.VersionString
    powershell=$PSVersionTable.PSVersion.ToString(); isWindows=$false; engineRoot=$null; ueVersion=$null
    unrealEditor=$null; unrealEditorCmd=$null; buildBat=$null; visualStudioCpp=$null
    windowsSdk=$null; projectFile=$ProjectFile; projectFilePresent=(Test-Path -LiteralPath $ProjectFile)
    map=$MapFile; mapPresent=(Test-Path -LiteralPath $MapFile); freeDiskBytes=$null
}
$RunRecords = [Collections.Generic.List[object]]::new()
$Reasons = [ordered]@{}

function Write-JsonFile([object]$Value, [string]$Path) {
    $Value | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Save-Summary([string]$Verdict) {
    $Summary = [ordered]@{ schemaVersion=2; gate='STEP 4C — Windows UE Validation Runner / Evidence Pipeline'; dryRun=[bool]$DryRun; sourceSha=$SourceSha; ueVersion=$Environment.ueVersion; results=$Results; reasons=$Reasons; runs=$RunRecords; verdict=$Verdict }
    # Flat aliases retain the review-gate example's machine-friendly shape.
    foreach ($Item in $Results.GetEnumerator()) { $Summary[$Item.Key] = $Item.Value }
    Write-JsonFile $Environment (Join-Path $EvidenceRoot 'environment.json')
    Write-JsonFile $Summary (Join-Path $EvidenceRoot 'summary.json')
    $Lines = @('# Ishibashiri Review Gate Evidence','',"- Source SHA: ``$SourceSha``","- UE version: ``$($Environment.ueVersion)``","- Verdict: **$Verdict**",'', '| Gate | Result |','|---|---|')
    foreach ($Item in $Results.GetEnumerator()) { $Lines += "| $($Item.Key) | **$($Item.Value)** |" }
    $Lines += @('', '## Execution plan', '', '| Order | Run | Result | Exit code | Command | Required inputs | Expected artifacts |', '|---:|---|---|---:|---|---|---|')
    $Order = 0
    foreach ($Run in $RunRecords) {
        $Order++
        $ExitCode = if ($null -eq $Run.exitCode) { 'NOT_RUN' } else { [string]$Run.exitCode }
        $Lines += "| $Order | $($Run.name) | $($Run.result) | $ExitCode | ``$($Run.command)`` | $($Run.requiredInputs -join '<br>') | $($Run.expectedArtifacts -join '<br>') |"
    }
    $Lines | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'summary.md') -Encoding UTF8
    if ($UpdateReviewDocument) { Copy-Item -LiteralPath (Join-Path $EvidenceRoot 'summary.json') -Destination (Join-Path $ProjectRoot 'Docs\IshibashiriReviewGate.json') -Force }
}

function Resolve-Engine {
    if ($EngineRoot) { return [IO.Path]::GetFullPath($EngineRoot) }
    $Candidates = @("$env:ProgramFiles\Epic Games\UE_5.6", "$env:USERPROFILE\UnrealEngine\UE_5.6")
    $Registry = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.6' -ErrorAction SilentlyContinue
    if ($Registry -and $Registry.InstalledDirectory) { $Candidates = @($Registry.InstalledDirectory) + $Candidates }
    foreach ($Candidate in $Candidates) { if ($Candidate -and (Test-Path -LiteralPath (Join-Path $Candidate 'Engine\Build\Build.version'))) { return $Candidate } }
    return $null
}

function Copy-NewEvidence([datetime]$Since, [string]$Area) {
    $Copied = [Collections.Generic.List[string]]::new()
    foreach ($Source in @((Join-Path $ProjectRoot 'Saved\Logs'), (Join-Path $ProjectRoot 'Saved\Screenshots'))) {
        if (!(Test-Path -LiteralPath $Source)) { continue }
        $Destination = if ($Source -like '*Screenshots') { Join-Path $EvidenceRoot 'screenshots' } else { Join-Path $EvidenceRoot "logs\$Area" }
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        Get-ChildItem -LiteralPath $Source -File -Recurse | Where-Object { $_.LastWriteTimeUtc -ge $Since.ToUniversalTime() } | ForEach-Object {
            $Relative = $_.FullName.Substring($Source.Length).TrimStart('\','/')
            $Target = Join-Path $Destination $Relative
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Target) | Out-Null
            Copy-Item -LiteralPath $_.FullName -Destination $Target -Force
            $Copied.Add($Target.Substring($EvidenceRoot.Length).TrimStart('\','/').Replace('\','/'))
        }
    }
    return @($Copied)
}

function Invoke-Gate([string]$Name, [string]$Area, [string[]]$Arguments) {
    $Started = Get-Date
    $Stdout = Join-Path $EvidenceRoot "logs\$Name.stdout.log"
    $Stderr = Join-Path $EvidenceRoot "logs\$Name.stderr.log"
    $Command = "& '$Prototype' " + (($Arguments | ForEach-Object { if ($_ -match '\s') { "'$_'" } else { $_ } }) -join ' ')
    $CommandFile = Join-Path $EvidenceRoot "logs\$Name.command.txt"
    $Command | Set-Content -LiteralPath $CommandFile -Encoding UTF8
    $RequiredInputs = @('Windows host', 'Unreal Engine 5.6.1', 'MSVC x64 toolchain', 'Windows SDK', "project at $ProjectFile", "source commit $SourceSha")
    $ExpectedArtifacts = @("logs/$Name.command.txt", "logs/$Name.stdout.log", "logs/$Name.stderr.log", "logs/$Name.exitcode.txt")
    if ($Area -ne 'build' -and $Area -ne 'package') { $ExpectedArtifacts += @("logs/$Area/", 'screenshots/ (when -Capture is present)') }
    if ($Area -eq 'package') { $ExpectedArtifacts += 'package/' }
    if ($DryRun) {
        $RunRecords.Add([ordered]@{ name=$Name; area=$Area; command=$Command; arguments=$Arguments; result='NOT_RUN'; exitCode=$null; startedUtc=$null; stdout="logs/$Name.stdout.log"; stderr="logs/$Name.stderr.log"; requiredInputs=$RequiredInputs; expectedArtifacts=$ExpectedArtifacts })
        return 'NOT_RUN'
    }
    $InvokeArgs = @('-NoLogo','-NoProfile','-ExecutionPolicy','Bypass','-File',$Prototype) + $Arguments
    if ($EngineRoot) { $InvokeArgs += @('-EngineRoot',$EngineRoot) }
    $QuotedArgs = ($InvokeArgs | ForEach-Object { '"' + ($_ -replace '"','\"') + '"' }) -join ' '
    $Process = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $QuotedArgs -Wait -PassThru -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr
    $Process.ExitCode | Set-Content -LiteralPath (Join-Path $EvidenceRoot "logs\$Name.exitcode.txt")
    $CopiedEvidence = @(Copy-NewEvidence $Started $Area)
    $RunResult = if ($Process.ExitCode -eq 0) { 'PASS' } else { 'FAIL' }
    $ActualArtifacts = @("logs/$Name.command.txt", "logs/$Name.stdout.log", "logs/$Name.stderr.log", "logs/$Name.exitcode.txt") + $CopiedEvidence
    $RunRecords.Add([ordered]@{ name=$Name; area=$Area; command=$Command; arguments=$Arguments; result=$RunResult; exitCode=$Process.ExitCode; startedUtc=$Started.ToUniversalTime().ToString('o'); stdout="logs/$Name.stdout.log"; stderr="logs/$Name.stderr.log"; requiredInputs=$RequiredInputs; expectedArtifacts=$ExpectedArtifacts; artifacts=$ActualArtifacts })
    if ($Process.ExitCode -eq 0) { return 'PASS' }
    return 'FAIL'
}

try {
    $Environment.isWindows = $env:OS -eq 'Windows_NT'
    $Drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($EvidenceRoot)); $Environment.freeDiskBytes = $Drive.AvailableFreeSpace
    if ($DryRun) {
        $Reasons.dryRun = 'Planning only: no Unreal build, gameplay, capture, or package process was started.'
        $null = Invoke-Gate 'build' 'build' @('-Action','Build')
        $null = Invoke-Gate 'approach60' 'approach' @('-Action','Test','-Approach','-TestFPS','60')
        $null = Invoke-Gate 'approach30' 'approach' @('-Action','Test','-Approach','-TestFPS','30')
        $null = Invoke-Gate 'ishibashiri60' 'climbing' @('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'ishibashiri30' 'climbing' @('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','30')
        $null = Invoke-Gate 'gamepad' 'climbing' @('-Action','Test','-ClimbingGamepad','-Basin','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'climbingIK' 'ik' @('-Action','Test','-ClimbingIK','-Capture','-Basin','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'legacy' 'climbing' @('-Action','Test','-Climbing','-Capture','-Basin','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'highQuality' 'climbing' @('-Action','Test','-Climbing','-Capture','-Basin','-HighQuality','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'campaign60' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'campaign30' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-TestFPS','30')
        $null = Invoke-Gate 'campaignGamepad' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-Gamepad','-TestFPS','60')
        $null = Invoke-Gate 'fuchimatoi60' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-Capture','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'fuchimatoi30' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-SkipBuild','-TestFPS','30')
        $null = Invoke-Gate 'fuchimatoiGamepad' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-Gamepad','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'minedaki60' 'minedaki' @('-Action','Test','-Minedaki','-Capture','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'minedaki30' 'minedaki' @('-Action','Test','-Minedaki','-SkipBuild','-TestFPS','30')
        $null = Invoke-Gate 'minedakiGamepad' 'minedaki' @('-Action','Test','-Minedaki','-Gamepad','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'magatsune60' 'magatsune' @('-Action','Test','-Magatsune','-Capture','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'magatsune30' 'magatsune' @('-Action','Test','-Magatsune','-SkipBuild','-TestFPS','30')
        $null = Invoke-Gate 'magatsuneGamepad' 'magatsune' @('-Action','Test','-Magatsune','-Gamepad','-SkipBuild','-TestFPS','60')
        $null = Invoke-Gate 'package' 'package' @('-Action','Package')
        Save-Summary 'NOT_RUN'
        Write-Host "DryRun only; Unreal validation was NOT_RUN. Evidence: $EvidenceRoot"
        exit 0
    }
    if (!$Environment.isWindows) { Save-Summary 'UNVERIFIED — WINDOWS UE 5.6.1 NOT AVAILABLE'; Write-Host "Evidence: $EvidenceRoot"; exit 2 }
    $ResolvedEngine = Resolve-Engine
    $Environment.engineRoot = $ResolvedEngine
    if ($ResolvedEngine) {
        $Environment.unrealEditor = Join-Path $ResolvedEngine 'Engine\Binaries\Win64\UnrealEditor.exe'
        $Environment.unrealEditorCmd = Join-Path $ResolvedEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        $Environment.buildBat = Join-Path $ResolvedEngine 'Engine\Build\BatchFiles\Build.bat'
        $VersionFile = Join-Path $ResolvedEngine 'Engine\Build\Build.version'
        if (Test-Path -LiteralPath $VersionFile) { $V = Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json; $Environment.ueVersion = "$($V.MajorVersion).$($V.MinorVersion).$($V.PatchVersion)" }
    }
    $VSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $VSWhere) { $Environment.visualStudioCpp = (& $VSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1) }
    $SdkRoots = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
    if ($SdkRoots) { $Environment.windowsSdk = $SdkRoots.KitsRoot10 }
    $RequiredFiles = @($Environment.unrealEditor,$Environment.unrealEditorCmd,$Environment.buildBat)
    $MapCanBeCreated = Test-Path -LiteralPath (Join-Path $PSScriptRoot 'CreatePrototypeMap.py')
    $PreflightOk = $ResolvedEngine -and $Environment.ueVersion -eq '5.6.1' -and $Environment.visualStudioCpp -and $Environment.windowsSdk -and $Environment.projectFilePresent -and ($Environment.mapPresent -or $MapCanBeCreated) -and $Environment.freeDiskBytes -ge 20GB -and !($RequiredFiles | Where-Object { !$_ -or !(Test-Path -LiteralPath $_) })
    Write-JsonFile $Environment (Join-Path $EvidenceRoot 'environment.json')
    if (!$PreflightOk) { Save-Summary 'UNVERIFIED — WINDOWS UE 5.6.1 NOT AVAILABLE'; Write-Host "Evidence: $EvidenceRoot"; exit 2 }

    $Results.build = Invoke-Gate 'build' 'build' @('-Action','Build')
    if ($Results.build -ne 'PASS') { Save-Summary 'FAIL'; Write-Host "Evidence: $EvidenceRoot"; exit 1 }
    if (!$Environment.mapPresent) {
        $MapSetup = Invoke-Gate 'mapSetup' 'build' @('-Action','Setup','-SkipBuild')
        $Environment.mapPresent = Test-Path -LiteralPath $MapFile
        if ($MapSetup -ne 'PASS' -or !$Environment.mapPresent) { $Reasons.map='Map generation failed after the successful Build gate'; Save-Summary 'FAIL'; Write-Host "Evidence: $EvidenceRoot"; exit 1 }
    }

    $Results.approach60 = Invoke-Gate 'approach60' 'approach' @('-Action','Test','-Approach','-TestFPS','60')
    $Results.approach30 = Invoke-Gate 'approach30' 'approach' @('-Action','Test','-Approach','-TestFPS','30')
    $Results.ishibashiri60 = Invoke-Gate 'ishibashiri60' 'climbing' @('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','60')
    $Results.ishibashiri30 = Invoke-Gate 'ishibashiri30' 'climbing' @('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','30')
    $Results.gamepad = Invoke-Gate 'gamepad' 'climbing' @('-Action','Test','-ClimbingGamepad','-Basin','-SkipBuild','-TestFPS','60')
    $GameplayLogs = (Get-ChildItem -LiteralPath (Join-Path $EvidenceRoot 'logs') -File -Recurse | Get-Content -ErrorAction SilentlyContinue) -join "`n"
    if ($Results.ishibashiri60 -eq 'PASS' -and $GameplayLogs -match 'retry=[1-9]|BASIN_RETRY_RESET|GAMEPAD_RETRY') { $Results.retry='PASS' } elseif ($Results.ishibashiri60 -eq 'FAIL') { $Results.retry='FAIL' }
    if ($Results.approach60 -eq 'PASS' -and $GameplayLogs -match 'PLAYER_SENSE reset' -and $GameplayLogs -match 'sense=boundary,corruption') { $Results.senseReset='PASS' } elseif ($Results.approach60 -eq 'FAIL') { $Results.senseReset='FAIL' }
    if (Test-Path -LiteralPath $ControlRigFile) { $Results.controlRig = Invoke-Gate 'climbingIK' 'ik' @('-Action','Test','-ClimbingIK','-Capture','-Basin','-SkipBuild','-TestFPS','60') } else { $Results.controlRig='BLOCKED'; $Reasons.controlRig='BLOCKED — CONTROL RIG ASSET ABSENT' }
    $Results.legacy = Invoke-Gate 'legacy' 'climbing' @('-Action','Test','-Climbing','-Capture','-Basin','-SkipBuild','-TestFPS','60')
    $HqRun = Invoke-Gate 'highQuality' 'climbing' @('-Action','Test','-Climbing','-Capture','-Basin','-HighQuality','-SkipBuild','-TestFPS','60')
    $HqText = (Get-ChildItem -LiteralPath (Join-Path $EvidenceRoot 'logs') -File -Recurse | Where-Object Name -match 'highQuality|ClimbingTest-60' | Get-Content -ErrorAction SilentlyContinue) -join "`n"
    $HqVerified = $HqText -match 'D3D12' -and $HqText -match 'SM6|PCD3D_SM6' -and $HqText -match 'DynamicGlobalIlluminationMethod[^\r\n]*1|Lumen.*GI' -and $HqText -match 'ReflectionMethod[^\r\n]*1|Lumen.*Reflection' -and $HqText -match 'Shadow.Virtual.Enable[^\r\n]*1|Virtual Shadow Map'
    $Results.highQuality = if ($HqRun -eq 'FAIL') {'FAIL'} elseif ($HqVerified) {'PASS'} else { $Reasons.highQuality='HighQuality process completed but D3D12, SM6, Lumen GI, Lumen Reflections, and Virtual Shadow Maps were not all evidenced in logs'; 'BLOCKED' }
    $Results.campaign60 = Invoke-Gate 'campaign60' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')
    $Results.campaign30 = Invoke-Gate 'campaign30' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-TestFPS','30')
    $Results.campaignGamepad = Invoke-Gate 'campaignGamepad' 'campaign' @('-Action','Test','-Campaign','-SkipBuild','-Gamepad','-TestFPS','60')
    # Recovery and capture share the 60 FPS run so equivalent long encounters are not repeated.
    $Results.fuchimatoi60 = Invoke-Gate 'fuchimatoi60' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-Capture','-SkipBuild','-TestFPS','60')
    $Results.fuchimatoi30 = Invoke-Gate 'fuchimatoi30' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-SkipBuild','-TestFPS','30')
    $Results.fuchimatoiGamepad = Invoke-Gate 'fuchimatoiGamepad' 'fuchimatoi' @('-Action','Test','-Fuchimatoi','-Recovery','-Gamepad','-SkipBuild','-TestFPS','60')
    # Minedaki and Magatsune integration drivers include their fall/recovery paths.
    $Results.minedaki60 = Invoke-Gate 'minedaki60' 'minedaki' @('-Action','Test','-Minedaki','-Capture','-SkipBuild','-TestFPS','60')
    $Results.minedaki30 = Invoke-Gate 'minedaki30' 'minedaki' @('-Action','Test','-Minedaki','-SkipBuild','-TestFPS','30')
    $Results.minedakiGamepad = Invoke-Gate 'minedakiGamepad' 'minedaki' @('-Action','Test','-Minedaki','-Gamepad','-SkipBuild','-TestFPS','60')
    $Results.magatsune60 = Invoke-Gate 'magatsune60' 'magatsune' @('-Action','Test','-Magatsune','-Capture','-SkipBuild','-TestFPS','60')
    $Results.magatsune30 = Invoke-Gate 'magatsune30' 'magatsune' @('-Action','Test','-Magatsune','-SkipBuild','-TestFPS','30')
    $Results.magatsuneGamepad = Invoke-Gate 'magatsuneGamepad' 'magatsune' @('-Action','Test','-Magatsune','-Gamepad','-SkipBuild','-TestFPS','60')
    $Major = @('approach60','approach30','ishibashiri60','ishibashiri30','gamepad','retry','senseReset','legacy','highQuality','campaign60','campaign30','campaignGamepad','fuchimatoi60','fuchimatoi30','fuchimatoiGamepad','minedaki60','minedaki30','minedakiGamepad','magatsune60','magatsune30','magatsuneGamepad')
    if (!($Major | Where-Object { $Results[$_] -ne 'PASS' })) {
        $Results.package = Invoke-Gate 'package' 'package' @('-Action','Package')
        $PackageSource = Join-Path $ProjectRoot 'Artifacts\Windows'
        if ($Results.package -eq 'PASS' -and (Test-Path -LiteralPath $PackageSource)) {
            Copy-Item -LiteralPath $PackageSource -Destination (Join-Path $EvidenceRoot 'package') -Recurse -Force
        }
    }
    $States = @($Results.Values)
    $Verdict = if ($States -contains 'FAIL') {'FAIL'} elseif ($States -contains 'BLOCKED' -or $States -contains 'NOT_RUN') {'PARTIAL'} else {'PASS'}
    Save-Summary $Verdict
    Write-Host "Evidence: $EvidenceRoot"
    if ($Verdict -eq 'PASS') { exit 0 } else { exit 1 }
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'logs\runner-error.log') -Encoding UTF8
    Save-Summary 'FAIL'
    Write-Error $_ -ErrorAction Continue
    exit 1
}
