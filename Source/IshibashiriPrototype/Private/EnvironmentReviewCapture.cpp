#include "EnvironmentReviewCapture.h"
#include "IshibashiriApproachGameMode.h"
#include "IshibashiriApproachArena.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AEnvironmentReviewCapture::AEnvironmentReviewCapture()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AEnvironmentReviewCapture::BeginPlay()
{
    Super::BeginPlay();
#if UE_BUILD_SHIPPING
    Destroy();
    return;
#endif
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    Directory = FPaths::ProjectSavedDir() / TEXT("Screenshots/EnvironmentReview") / RunId;
    IFileManager::Get().MakeDirectory(*Directory, true);
    if (auto* Approach = GetWorld()->GetAuthGameMode<AIshibashiriApproachGameMode>(); Approach && Approach->GetApproachArena())
    {
        Player = Approach->GetPlayer();
        const auto& Points = Approach->GetApproachArena()->GetPathPoints();
        const int32 Indices[] = {0, 2, 3, 6, 8};
        Names = {TEXT("01-ApproachEntrance"), TEXT("02-CedarEnclosure"), TEXT("03-BoundaryClearing"),
            TEXT("04-DamagedGrove"), TEXT("05-RevealGap")};
        for (int32 I : Indices)
        {
            const FVector Along = (Points[I + 1] - Points[I]).GetSafeNormal2D();
            PlayerLocations.Add(Points[I] + FVector(0, 0, 100));
            CameraLocations.Add(Points[I] - Along * 750.f + FVector(0, 0, 450));
            CameraTargets.Add(Points[I] + Along * 1600.f + FVector(0, 0, 300));
        }
    }
    else if (auto* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>())
    {
        Player = Mode->GetPlayer();
        if (Mode->GetBoss()) Mode->GetBoss()->SetActorTickEnabled(false);
        Names = {TEXT("06-BasinEntrance"), TEXT("07-BasinWide")};
        PlayerLocations = {{-3000, 0, 100}, {-2200, -600, 100}};
        CameraLocations = {{-3700, -400, 550}, {-3500, -2900, 1900}};
        CameraTargets = {{700, 0, 550}, {500, 200, 500}};
    }
    if (!Player || !Names.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("ENVIRONMENT_REVIEW_FAIL %s missing scene"), *RunId);
        bFinished = true;
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    ReviewCamera = GetWorld()->SpawnActor<ACameraActor>();
    if (!ReviewCamera)
    {
        UE_LOG(LogTemp, Error, TEXT("ENVIRONMENT_REVIEW_FAIL %s camera spawn failed"), *RunId);
        bFinished = true;
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    ReviewCamera->GetCameraComponent()->SetFieldOfView(75.f);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->DisableMovement();
    PlaceView();
}

void AEnvironmentReviewCapture::PlaceView()
{
    Player->SetActorLocation(PlayerLocations[ViewIndex], false, nullptr, ETeleportType::TeleportPhysics);
    const FRotator Rotation = (CameraTargets[ViewIndex] - CameraLocations[ViewIndex]).Rotation();
    Player->SetActorRotation(FRotator(0, Rotation.Yaw, 0));
    ReviewCamera->SetActorLocationAndRotation(CameraLocations[ViewIndex], Rotation);
    if (auto* PC = Cast<APlayerController>(Player->GetController()))
    {
        PC->SetControlRotation(Rotation);
        PC->SetViewTarget(ReviewCamera);
    }
    UE_LOG(LogTemp, Display, TEXT("ENVIRONMENT_REVIEW_VIEW %s %s arranged=true camera=%s rotation=%s fov=75"),
        *RunId, *Names[ViewIndex], *CameraLocations[ViewIndex].ToString(), *Rotation.ToString());
    Elapsed = 0.f;
    bRequested = false;
}

void AEnvironmentReviewCapture::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished || !Player || !ReviewCamera) return;
    Elapsed += DeltaSeconds;
    const FString File = Directory / (Names[ViewIndex] + TEXT(".png"));
    if (!bRequested && Elapsed >= 1.5f)
    {
        FScreenshotRequest::RequestScreenshot(File, true, false);
        bRequested = true;
    }
    if (!bRequested || Elapsed < 2.0f) return;
    if (IFileManager::Get().FileSize(*File) <= 100)
    {
        if (Elapsed < 15.f) return;
        UE_LOG(LogTemp, Error, TEXT("ENVIRONMENT_REVIEW_FAIL %s missing capture=%s"), *RunId, *File);
        bFinished = true;
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    if (++ViewIndex < Names.Num()) { PlaceView(); return; }
    UE_LOG(LogTemp, Display, TEXT("ENVIRONMENT_REVIEW_PASS %s arranged=true images=%d"), *RunId, Names.Num());
    bFinished = true;
    FPlatformMisc::RequestExitWithStatus(false, 0);
}
