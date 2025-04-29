#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeManager.generated.h"

UCLASS()
class RL_API ACubeManager : public AActor
{
    GENERATED_BODY()

public:
    ACubeManager();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> CubeClass;

    UPROPERTY(EditAnywhere)
    int32 NumCubes = 100;

    UPROPERTY(EditAnywhere)
    int32 StartingPort = 5001;


    UPROPERTY(EditAnywhere)
    int32 SpeedManager = 100;



    UFUNCTION()
    void ResetScene();  // <-- Reset function here

private:
    FTimerHandle ResetTimerHandle;  // <-- Timer handle
    TArray<AActor*> SpawnedCubes;    // <-- Store spawned cubes
   
};
