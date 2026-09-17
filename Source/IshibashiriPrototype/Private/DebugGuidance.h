#pragma once

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// -DebugGuidance is read by HUDs and boss presentation every frame; the command
// line never changes after launch, so parse it once.
inline bool IsDebugGuidanceEnabled()
{
    static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("DebugGuidance"));
    return bEnabled;
}
