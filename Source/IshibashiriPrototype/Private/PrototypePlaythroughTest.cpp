#include "PrototypePlaythroughTest.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
    int32 CountPlaythroughActors(UWorld* World)
    {
        int32 Count = 0;
        for (TActorIterator<AActor> It(World); It; ++It) ++Count;
        return Count;
    }
}

APrototypePlaythroughTest::APrototypePlaythroughTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APrototypePlaythroughTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestSeconds="), MinimumSeconds);
    MinimumSeconds = FMath::Clamp(MinimumSeconds, 0, 3600);
    int32 FPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(FPS, 15, 240));
    bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Require(Mode && Mode->GetPlayer() && Mode->GetBoss()
        && Cast<APlayerController>(Mode->GetPlayer()->GetController()), TEXT("Playable encounter exists"))) return;
    InitialActorCount = CountPlaythroughActors(GetWorld());
    AddTickPrerequisiteActor(Mode->GetBoss());
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_PLAYTHROUGH_BEGIN %s FPS=%d minimumSeconds=%d"), *RunId, FPS, MinimumSeconds);
}

void APrototypePlaythroughTest::SendKey(const FKey& Key, EInputEvent Event, float Amount)
{
    APlayerController* Controller = Cast<APlayerController>(Mode->GetPlayer()->GetController());
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    if (Event == IE_Axis)
        Controller->InputKey(FInputKeyParams(Key, static_cast<double>(Amount), GetWorld()->GetDeltaSeconds(), 1));
    else
        Controller->InputKey(FInputKeyParams(Key, Event, static_cast<double>(Amount)));
#else
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Amount));
#endif
}

void APrototypePlaythroughTest::HoldKey(const FKey& Key, bool bDown)
{
    if (HeldKeys.Contains(Key) == bDown) return;
    SendKey(Key, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f);
    if (bDown) HeldKeys.Add(Key); else HeldKeys.Remove(Key);
}

void APrototypePlaythroughTest::TapKey(const FKey& Key)
{
    SendKey(Key, IE_Pressed);
    PendingReleases.AddUnique(Key);
}

void APrototypePlaythroughTest::ReleaseMovement()
{
    HoldKey(EKeys::W, false);
    HoldKey(EKeys::A, false);
    HoldKey(EKeys::D, false);
}

float APrototypePlaythroughTest::AimAt(const FVector& Position)
{
    const APrototypePlayer* Player = Mode->GetPlayer();
    const float DesiredYaw = (Position - Player->GetActorLocation()).Rotation().Yaw;
    const float Error = FMath::FindDeltaAngleDegrees(Player->GetController()->GetControlRotation().Yaw, DesiredYaw);
    // Closed-loop mouse movement: observe the next frame instead of setting rotation.
    SendKey(EKeys::MouseX, IE_Axis, FMath::Clamp(Error * 5.f, -300.f, 300.f));
    return FMath::Abs(Error);
}

void APrototypePlaythroughTest::WalkTo(const FVector& Position, float StopDistance)
{
    const float Error = AimAt(Position);
    HoldKey(EKeys::W, Error < 15.f && FVector::Dist2D(Position, Mode->GetPlayer()->GetActorLocation()) > StopDistance);
}

bool APrototypePlaythroughTest::Require(bool Condition, const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_FAIL %s: %s (round=%d time=%.2f counters=%d dodges=%d)"),
            *RunId, Message, CompletedEncounters + 1, EncounterSeconds, RoundCounters, RoundDodges);
        Finish(false);
    }
    return Condition;
}

void APrototypePlaythroughTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDone || !Mode) return;
    for (const FKey& Key : PendingReleases) SendKey(Key, IE_Released, 0.f);
    PendingReleases.Reset();
    TotalSeconds += DeltaSeconds;
    EncounterSeconds += DeltaSeconds;
    AttackWait = FMath::Max(0.f, AttackWait - DeltaSeconds);
    if (!Require(EncounterSeconds < 60.f && TotalSeconds < FMath::Max(180.f, MinimumSeconds + 90.f), TEXT("Input-only playthrough finishes without stalling"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    if (!Require(Player->GetHealth() == 3, TEXT("Sideways dodge avoids charge damage throughout the encounter"))) return;

    if (bRestarting)
    {
        if (!Require(Mode->IsEncounterActive() && Boss->GetHealth() == 3
            && Player->GetDodgeCooldown() == 0.f && CountPlaythroughActors(GetWorld()) == InitialActorCount,
            TEXT("R starts a clean encounter without accumulating actors"))) return;
        ++CompletedEncounters;
        UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_PLAYTHROUGH_ROUND %s round=%d seconds=%.2f counters=%d dodges=%d HP=%d"),
            *RunId, CompletedEncounters, EncounterSeconds, RoundCounters, RoundDodges, Player->GetHealth());
        if (CompletedEncounters >= 3 && TotalSeconds >= MinimumSeconds) { Finish(true); return; }
        EncounterSeconds = VictorySeconds = AttackWait = StateSeconds = 0.f;
        RoundCounters = RoundDodges = 0;
        LastBossHealth = 3;
        LastState = -1;
        bRestarting = false;
    }

    if (Boss->GetHealth() < LastBossHealth)
    {
        ++RoundCounters;
        LastBossHealth = Boss->GetHealth();
        UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_PLAYTHROUGH_COUNTER %s counter=%d distance=%.1f inputRecoveryRemaining=%.2f"),
            *RunId, RoundCounters, FVector::Dist2D(Player->GetActorLocation(), Boss->GetActorLocation()), LastAttackWindow);
    }
    if (!Mode->IsEncounterActive())
    {
        ReleaseMovement();
        if (!Require(Mode->GetResult() == EEncounterResult::Victory && RoundCounters == 3 && RoundDodges >= 3,
            TEXT("Three input-driven dodge/approach/counter cycles reach victory"))) return;
        VictorySeconds += DeltaSeconds;
        if (bCapture && !bCaptured && VictorySeconds > 0.05f)
        {
            const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Prototype") / RunId);
            IFileManager::Get().MakeDirectory(*Directory, true);
            FScreenshotRequest::RequestScreenshot(Directory / TEXT("08-InputVictory.png"), false, false);
            bCaptured = true;
        }
        if (VictorySeconds >= 0.2f) { TapKey(EKeys::R); bRestarting = true; }
        return;
    }

    const EIshibashiriState State = Boss->GetState();
    if (LastState != static_cast<int32>(State))
    {
        if (LastState == static_cast<int32>(EIshibashiriState::Recover)
            && !Require(RoundCounters == RoundDodges, TEXT("Normal movement reaches the boss before every recovery expires"))) return;
        StateSeconds = 0.f;
        if (State == EIshibashiriState::Telegraph)
        {
            const FVector Right = FRotator(0.f, Player->GetController()->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
            SideKey = FVector::DotProduct(Right, -Player->GetActorLocation()) >= 0.f ? EKeys::D : EKeys::A;
        }
        if (State == EIshibashiriState::Charge) bDodgedThisCharge = false;
        LastState = static_cast<int32>(State);
    }
    StateSeconds += DeltaSeconds;
    if (State == EIshibashiriState::Chase || (State == EIshibashiriState::Recover && !Boss->CanBeCountered()))
    {
        HoldKey(EKeys::A, false);
        HoldKey(EKeys::D, false);
        const FVector Bait = Boss->GetActorLocation() + (-Boss->GetActorLocation()).GetSafeNormal2D() * 850.f;
        WalkTo(Bait, 70.f);
    }
    else if (State == EIshibashiriState::Telegraph)
    {
        HoldKey(EKeys::W, false);
        AimAt(Boss->GetActorLocation());
        HoldKey(SideKey, Boss->GetStateTimeRemaining() < 0.1f);
    }
    else if (State == EIshibashiriState::Charge)
    {
        HoldKey(EKeys::W, false);
        if (StateSeconds >= 0.17f && StateSeconds < 0.3f
            && !Require(Player->IsDodging(), TEXT("Dodge key starts a real dodge during every charge"))) return;
        if (StateSeconds < 0.42f)
        {
            HoldKey(SideKey, true);
            if (StateSeconds >= 0.1f && !bDodgedThisCharge)
            {
                TapKey(EKeys::LeftShift);
                bDodgedThisCharge = true;
                ++RoundDodges;
            }
        }
        else
        {
            HoldKey(SideKey, false);
            if (FVector::DotProduct(Player->GetActorLocation() - Boss->GetActorLocation(), Boss->GetChargeDirection()) < -250.f)
                WalkTo(Boss->GetActorLocation(), 300.f);
        }
    }
    else if (State == EIshibashiriState::Recover)
    {
        HoldKey(SideKey, false);
        const float Error = AimAt(Boss->GetActorLocation());
        const float Distance = FVector::Dist2D(Player->GetActorLocation(), Boss->GetActorLocation());
        HoldKey(EKeys::W, Error < 15.f && Distance > 300.f);
        if (Distance < 350.f && Error < 8.f && AttackWait <= 0.f)
        {
            LastAttackWindow = Boss->GetStateTimeRemaining();
            TapKey(EKeys::LeftMouseButton);
            AttackWait = 0.5f;
        }
    }
}

void APrototypePlaythroughTest::Finish(bool bSuccess)
{
    bDone = true;
    if (Mode && Mode->GetPlayer() && Mode->GetPlayer()->GetController()) ReleaseMovement();
    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_PLAYTHROUGH_PASS %s encounters=%d simulatedSeconds=%.2f"), *RunId, CompletedEncounters, TotalSeconds);
        UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_TEST_PASS %s"), *RunId);
    }
    FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}
