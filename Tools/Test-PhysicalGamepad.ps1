[CmdletBinding()]
param([ValidateRange(1,60)][int]$Seconds = 10)
$ErrorActionPreference = 'Stop'
if (!('PhysicalPadProbe' -as [type])) {
    Add-Type @'
using System.Runtime.InteropServices;
public static class PhysicalPadProbe {
    [StructLayout(LayoutKind.Sequential)]
    public struct Pad { public ushort Buttons; public byte LT, RT; public short LX, LY, RX, RY; }
    [StructLayout(LayoutKind.Sequential)]
    public struct State { public uint Packet; public Pad Gamepad; }
    [DllImport("xinput1_4.dll")] public static extern uint XInputGetState(uint index, out State state);
}
'@
}
$events = [System.Collections.Generic.List[object]]::new()
$last = @{}
$until = [DateTime]::UtcNow.AddSeconds($Seconds)
do {
    foreach ($slot in 0..3) {
        $state = [PhysicalPadProbe+State]::new()
        $code = [PhysicalPadProbe]::XInputGetState($slot, [ref]$state)
        $signature = "$code/$($state.Packet)"
        if ($last[$slot] -ne $signature) {
            $event = [pscustomobject]@{ Time=[DateTimeOffset]::Now.ToString('o'); Slot=$slot; Result=$code; Packet=$state.Packet; Buttons=$state.Gamepad.Buttons; LT=$state.Gamepad.LT; RT=$state.Gamepad.RT; LX=$state.Gamepad.LX; LY=$state.Gamepad.LY; RX=$state.Gamepad.RX; RY=$state.Gamepad.RY }
            $events.Add($event)
            $event | ConvertTo-Json -Compress | Write-Host
            $last[$slot] = $signature
        }
    }
    Start-Sleep -Milliseconds 50
} while ([DateTime]::UtcNow -lt $until)
$dir = Join-Path $PSScriptRoot '../Saved/FuchimatoiPerformance'
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$path = Join-Path $dir ('PhysicalGamepad-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
ConvertTo-Json -InputObject @($events.ToArray()) -Depth 4 | Set-Content -LiteralPath $path -Encoding utf8
Write-Host "Physical XInput observations: $path"
