#pragma once

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"
#include "Json.h"
#include "JsonUtilities.h"

#include "CubeManager.generated.h"  

UCLASS()
class RL_API ACubeManager : public AActor
{
    GENERATED_BODY()

public:
    ACubeManager();

protected:
    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

public:
    UFUNCTION()
    void ResetScene();


    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> CubeClass;

    UPROPERTY(EditAnywhere)
    int32 NumCubes = 100;

    UPROPERTY(EditAnywhere)
    int32 StartingPort = 5001;

    UPROPERTY(EditAnywhere)
    int32 SpeedManager = 100;

    void BuildAndSendJSON();

    void PollPythonForMove();

    void ReceiveMoveCommand(const FString& JsonStr);

private:
    FTimerHandle ResetTimerHandle;
    TArray<AActor*> SpawnedCubes;
    FVector WinLocation;
    AActor* WinActor = nullptr;
    int32 FrameCounter = 0;
};
