[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$EvidenceRoot,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Prototype = Join-Path $PSScriptRoot 'Prototype.ps1'
$SourceSha = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
if (!$SourceSha) { $SourceSha = 'UNKNOWN' }
$Stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
if (!$EvidenceRoot) { $EvidenceRoot = Join-Path $ProjectRoot "Artifacts\IshibashiriDemoReviewGate\$Stamp-$($SourceSha.Substring(0,[Math]::Min(12,$SourceSha.Length)))" }
$EvidenceRoot = [IO.Path]::GetFullPath($EvidenceRoot)
foreach ($Folder in @('logs','screenshots','performance','demo','campaign-regression','package')) {
    New-Item -ItemType Directory -Force -Path (Join-Path $EvidenceRoot $Folder) | Out-Null
}
$Results = [ordered]@{
    build='NOT_RUN'; demo60='NOT_RUN'; demo30='NOT_RUN'; demoGamepad='NOT_RUN'
    demoCapture='NOT_RUN'; demoHighQuality='NOT_RUN'; demoCompletion='NOT_RUN'
    saveIsolation='NOT_RUN'; retry='NOT_RUN'; senseReset='NOT_RUN'
    normalCampaignRegression='NOT_RUN'; package='NOT_RUN'
}
$Runs = [Collections.Generic.List[object]]::new()
$Reasons = [ordered]@{}
$Environment = [ordered]@{
    timestampUtc=$Stamp; sourceSha=$SourceSha; platform=[Environment]::OSVersion.VersionString
    powershell=$PSVersionTable.PSVersion.ToString(); isWindows=($env:OS -eq 'Windows_NT')
    engineRoot=$null; ueVersion=$null; projectFile=(Join-Path $ProjectRoot 'IshibashiriPrototype.uproject')
}

function Write-Json([object]$Value,[string]$Path) { $Value | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Path -Encoding UTF8 }
function Save-Summary([ValidateSet('PASS','PARTIAL','FAIL','NOT_RUN')][string]$Verdict) {
    $Summary = [ordered]@{ schemaVersion=1; gate='Ishibashiri Demo Review Gate'; sourceSha=$SourceSha; ueVersion=$Environment.ueVersion; dryRun=[bool]$DryRun; results=$Results; reasons=$Reasons; runs=$Runs; verdict=$Verdict }
    Write-Json $Environment (Join-Path $EvidenceRoot 'environment.json'); Write-Json $Summary (Join-Path $EvidenceRoot 'summary.json')
    $Lines = @('# Ishibashiri Demo Review Gate Evidence','',"- Source SHA: ``$SourceSha``","- UE version: ``$($Environment.ueVersion)``","- Verdict: **$Verdict**",'', '| Gate | Result |','|---|---|')
    foreach ($Pair in $Results.GetEnumerator()) { $Lines += "| $($Pair.Key) | **$($Pair.Value)** |" }
    $Lines += @('','## Runs','', '| Order | Run | Result | Exit code | Command | Expected artifact | Actual artifact |','|---:|---|---|---:|---|---|---|')
    $Order=0; foreach ($Run in $Runs) { $Order++; $Exit=if($null -eq $Run.exitCode){'NOT_RUN'}else{$Run.exitCode}; $Lines += "| $Order | $($Run.name) | $($Run.result) | $Exit | ``$($Run.command)`` | $($Run.expectedArtifact -join '<br>') | $($Run.actualArtifact -join '<br>') |" }
    $Lines | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'summary.md') -Encoding UTF8
}
function Copy-Evidence([datetime]$Since,[string]$Area) {
    $Actual=[Collections.Generic.List[string]]::new()
    foreach ($SourceName in @('Saved\Logs','Saved\Screenshots')) {
        $Source=Join-Path $ProjectRoot $SourceName; if(!(Test-Path -LiteralPath $Source)){continue}
        $Dest=if($SourceName -like '*Screenshots'){Join-Path $EvidenceRoot 'screenshots'}else{Join-Path $EvidenceRoot $Area}
        Get-ChildItem -LiteralPath $Source -File -Recurse | Where-Object LastWriteTimeUtc -ge $Since.ToUniversalTime() | ForEach-Object {
            $Relative=$_.FullName.Substring($Source.Length).TrimStart('\','/'); $Target=Join-Path $Dest $Relative
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Target) | Out-Null; Copy-Item $_.FullName $Target -Force
            $Actual.Add($Target.Substring($EvidenceRoot.Length).TrimStart('\','/').Replace('\','/'))
        }
    }; return @($Actual)
}
function Invoke-Gate([string]$Name,[string]$Area,[string[]]$Arguments) {
    $Command=".\Tools\Prototype.ps1 " + ($Arguments -join ' ')
    $Stdout=Join-Path $EvidenceRoot "logs\$Name.stdout.log"; $Stderr=Join-Path $EvidenceRoot "logs\$Name.stderr.log"
    $Expected=@("logs/$Name.stdout.log","logs/$Name.stderr.log","logs/$Name.exitcode.txt")
    if($Arguments -contains '-Capture'){$Expected += 'screenshots/IshibashiriDemo/<RunId>/01-Title.png..14-ReturnToTitle.png'}
    if($Area -eq 'package'){$Expected += 'package/Windows'}
    if($DryRun){ $Runs.Add([ordered]@{name=$Name;command=$Command;startTime=$null;exitCode=$null;stdout="logs/$Name.stdout.log";stderr="logs/$Name.stderr.log";expectedArtifact=$Expected;actualArtifact=@();result='NOT_RUN'}); Write-Host "[$($Runs.Count)] $Command"; return 'NOT_RUN' }
    $Started=Get-Date; $Args=@('-NoLogo','-NoProfile','-ExecutionPolicy','Bypass','-File',$Prototype)+$Arguments
    if($EngineRoot){$Args+=@('-EngineRoot',$EngineRoot)}
    $Process=Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList (($Args|ForEach-Object{'"'+($_-replace '"','\"')+'"'}) -join ' ') -Wait -PassThru -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr
    $Process.ExitCode | Set-Content (Join-Path $EvidenceRoot "logs\$Name.exitcode.txt")
    $Actual=@("logs/$Name.stdout.log","logs/$Name.stderr.log","logs/$Name.exitcode.txt") + @(Copy-Evidence $Started $Area)
    $Result=if($Process.ExitCode -eq 0){'PASS'}else{'FAIL'}
    $Runs.Add([ordered]@{name=$Name;command=$Command;startTime=$Started.ToUniversalTime().ToString('o');exitCode=$Process.ExitCode;stdout="logs/$Name.stdout.log";stderr="logs/$Name.stderr.log";expectedArtifact=$Expected;actualArtifact=$Actual;result=$Result})
    return $Result
}
function Get-EvidenceText([string[]]$Names) {
    (($Names | ForEach-Object { $Path=Join-Path $EvidenceRoot "logs\$_.stdout.log"; if(Test-Path $Path){Get-Content $Path}; $Path=Join-Path $EvidenceRoot "logs\$_.stderr.log"; if(Test-Path $Path){Get-Content $Path}; Get-ChildItem (Join-Path $EvidenceRoot 'demo') -File -Recurse -ErrorAction SilentlyContinue | Where-Object Name -like '*.log' | Get-Content -ErrorAction SilentlyContinue }) -join "`n")
}

try {
    $Plan = @(
        @('build','logs',@('-Action','Build')),
        @('demo60','demo',@('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','60')),
        @('demo30','demo',@('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','30')),
        @('demoGamepad','demo',@('-Action','Test','-IshibashiriDemo','-Gamepad','-SkipBuild','-TestFPS','60')),
        @('demoCapture','demo',@('-Action','Test','-IshibashiriDemo','-Capture','-SkipBuild','-TestFPS','60')),
        @('demoHighQuality','demo',@('-Action','Test','-IshibashiriDemo','-Capture','-HighQuality','-SkipBuild','-TestFPS','60')),
        @('normalCampaignRegression','campaign-regression',@('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')),
        @('package','package',@('-Action','Package'))
    )
    if($DryRun){ foreach($Step in $Plan){$null=Invoke-Gate $Step[0] $Step[1] $Step[2]}; $Reasons.dryRun='Planning only; no build, Unreal process, screenshot, or package was started.'; Save-Summary 'NOT_RUN'; Write-Host "DryRun only: all gates NOT_RUN. Evidence: $EvidenceRoot"; exit 0 }
    if(!$Environment.isWindows){$Reasons.environment='WINDOWS UE 5.6.1 REQUIRED'; Save-Summary 'NOT_RUN'; exit 2}
    $Resolved=if($EngineRoot){[IO.Path]::GetFullPath($EngineRoot)}else{Join-Path $env:ProgramFiles 'Epic Games\UE_5.6'}; $Environment.engineRoot=$Resolved
    $VersionFile=Join-Path $Resolved 'Engine\Build\Build.version'; if(Test-Path $VersionFile){$V=Get-Content $VersionFile -Raw|ConvertFrom-Json;$Environment.ueVersion="$($V.MajorVersion).$($V.MinorVersion).$($V.PatchVersion)"}
    if($Environment.ueVersion -ne '5.6.1'){$Reasons.environment='Exact UE 5.6.1 is required';Save-Summary 'NOT_RUN';exit 2}
    $Results.build=Invoke-Gate 'build' 'logs' @('-Action','Build')
    if($Results.build -ne 'PASS'){Save-Summary 'FAIL';exit 1}
    $Results.demo60=Invoke-Gate 'demo60' 'demo' @('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','60')
    $Results.demo30=Invoke-Gate 'demo30' 'demo' @('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','30')
    $Results.demoGamepad=Invoke-Gate 'demoGamepad' 'demo' @('-Action','Test','-IshibashiriDemo','-Gamepad','-SkipBuild','-TestFPS','60')
    $Results.demoCapture=Invoke-Gate 'demoCapture' 'demo' @('-Action','Test','-IshibashiriDemo','-Capture','-SkipBuild','-TestFPS','60')
    $HqRun=Invoke-Gate 'demoHighQuality' 'demo' @('-Action','Test','-IshibashiriDemo','-Capture','-HighQuality','-SkipBuild','-TestFPS','60')
    $Text=Get-EvidenceText @('demo60','demo30','demoGamepad','demoCapture','demoHighQuality')
    $RequiredCompletion=@('CAMPAIGN_E2E Ishibashiri Started','CLIMB_TEST_PASS','CAMPAIGN_E2E Ishibashiri Started Completed','CAMPAIGN_E2E Interlude1','ISHIBASHIRI_DEMO_COMPLETE','CAMPAIGN_E2E Title','ISHIBASHIRI_DEMO_E2E_PASS')
    $Results.demoCompletion=if($Results.demo60 -eq 'FAIL'){'FAIL'}elseif(!($RequiredCompletion|Where-Object{$Text -notmatch [regex]::Escape($_)})){'PASS'}else{$Reasons.demoCompletion='Required start, Kakon 3/3, Calm, encounter completion, demo ending, Title return, and E2E markers were not all present';'FAIL'}
    $ForbiddenPersistence='CAMPAIGN_SAVED|CAMPAIGN_CONTINUE|CAMPAIGN_SAVE_CLEAR_FAILED'
    $Results.saveIsolation=if($Text -match 'ISHIBASHIRI_DEMO_E2E_PASS' -and $Text -notmatch $ForbiddenPersistence){'PASS'}else{$Reasons.saveIsolation='Demo E2E proof was missing or a normal MagabaraiCampaign persistence marker was observed';'FAIL'}
    $Results.retry=if($Text -match 'CLIMB_TEST_PASS .*routes=2 retry=1 sense=boundary,corruption'){'PASS'}else{$Reasons.retry='The full route did not pass again after Retry';'FAIL'}
    $Results.senseReset=if($Text -match 'PLAYER_SENSE reset' -and $Text -match 'sense=boundary,corruption'){'PASS'}else{$Reasons.senseReset='Boundary/Corruption Sense reset evidence is incomplete';'FAIL'}
    $HqText=Get-EvidenceText @('demoHighQuality'); $HqEvidence=$HqText -match 'D3D12' -and $HqText -match 'SM6|PCD3D_SM6' -and $HqText -match 'DynamicGlobalIlluminationMethod[^\r\n]*1|Lumen.*GI' -and $HqText -match 'ReflectionMethod[^\r\n]*1|Lumen.*Reflection' -and $HqText -match 'Shadow.Virtual.Enable[^\r\n]*1|Virtual Shadow Map'
    $Results.demoHighQuality=if($HqRun -eq 'FAIL'){'FAIL'}elseif($HqEvidence){'PASS'}else{$Reasons.demoHighQuality='D3D12, SM6, Lumen GI, Lumen Reflections, and Virtual Shadow Maps require actual log evidence';'FAIL'}
    $Results.normalCampaignRegression=Invoke-Gate 'normalCampaignRegression' 'campaign-regression' @('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')
    $Required=@('build','demo60','demo30','demoGamepad','demoCapture','demoHighQuality','demoCompletion','saveIsolation','retry','senseReset','normalCampaignRegression')
    if(!($Required|Where-Object{$Results[$_] -ne 'PASS'})){$Results.package=Invoke-Gate 'package' 'package' @('-Action','Package');if($Results.package -eq 'PASS' -and (Test-Path (Join-Path $ProjectRoot 'Artifacts\Windows'))){Copy-Item (Join-Path $ProjectRoot 'Artifacts\Windows') (Join-Path $EvidenceRoot 'package') -Recurse -Force}}
    $Reasons.controlRig='BLOCKED / OPTIONAL QUALITY GATE — absence never fabricates runtime PASS and does not block demo packaging.'
    $Verdict=if($Results.Values -contains 'FAIL'){'FAIL'}elseif($Results.Values -contains 'NOT_RUN'){'PARTIAL'}else{'PASS'};Save-Summary $Verdict;Write-Host "Evidence: $EvidenceRoot";if($Verdict -eq 'PASS'){exit 0}else{exit 1}
} catch { $_|Out-String|Set-Content (Join-Path $EvidenceRoot 'logs\runner-error.log');Save-Summary 'FAIL';Write-Error $_ -ErrorAction Continue;exit 1 }
