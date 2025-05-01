#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovableCube.generated.h"

USTRUCT(BlueprintType)
struct FScanHitResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector HitLocation;

    UPROPERTY(BlueprintReadOnly)
    FString ActorName;
};

UCLASS()
class RL_API AMovableCube : public AActor
{
    GENERATED_BODY()

public:
    AMovableCube();
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void MoveCube(float X, float Y, float Yaw);
    void ResetCube();
    TArray<FScanHitResult> PerformTimedScan() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    bool Won = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    bool Lost = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float Speed = 100.0f;
    TArray<FScanHitResult> ScanResults;


private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComponent;

    float CurrentScanAngle = 0.0f;
    int32 RaysPerFrame = 36;
    float RayDistance = 10000.0f;
    int32 FrameCounter = 0;
};
