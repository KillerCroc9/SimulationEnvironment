#include "MovableCube.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"


AMovableCube::AMovableCube()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AMovableCube::BeginPlay()
{
    Super::BeginPlay();

    MeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
        MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
        MeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
        MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    }

}

void AMovableCube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Won || Lost)
    {
        MeshComponent->SetSimulatePhysics(false);
        ResetCube();
        return;
    }

}

TArray<FScanHitResult> AMovableCube::PerformTimedScan() const
{
    TArray<FScanHitResult> LocalScanResults;

    FVector Origin = GetActorLocation();
    UWorld* World = GetWorld();
    if (!World) return LocalScanResults;

    float AngleStep = 360.0f / RaysPerFrame;
    float AngleOffset = CurrentScanAngle;  // optional — don't modify global value

    for (int32 i = 0; i < RaysPerFrame; ++i)
    {
        float Angle = FMath::Fmod(CurrentScanAngle + i * AngleStep, 360.0f);
        FVector Dir = FVector(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0);
        FVector End = Origin + Dir * RayDistance;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        if (World->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Params))
        {
            FScanHitResult Result;
            Result.HitLocation = Hit.ImpactPoint;

            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor->Tags.Num() > 0)
            {
                Result.ActorName = HitActor->Tags[0].ToString();
            }
            else
            {
                Result.ActorName = TEXT("None");
            }

            LocalScanResults.Add(Result);
        }
    }

    return LocalScanResults;
}

void AMovableCube::MoveCube(float X, float Y, float Yaw)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetWorld() is null in MoveCube()"));
        return;
    }
    float DeltaTime = World->GetDeltaSeconds();

    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();
    FVector Direction = (Forward * X + Right * Y).GetSafeNormal();

    float Force = Speed * DeltaTime * 1000.0f;
    MeshComponent->AddForce(Direction * Force);
}

void AMovableCube::ResetCube()
{
    SetActorLocation(FVector::ZeroVector);
    SetActorRotation(FRotator::ZeroRotator);

    if (MeshComponent)
    {
        MeshComponent->SetSimulatePhysics(false);
        MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
        MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        MeshComponent->SetSimulatePhysics(true);
    }

    Won = false;
    Lost = false;
    FrameCounter = 0;
}

void AMovableCube::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
