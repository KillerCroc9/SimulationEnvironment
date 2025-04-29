// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/RunnableThread.h"
#include "Misc/DateTime.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "MovableCube.generated.h"

UCLASS()
class RL_API AMovableCube : public AActor
{
    GENERATED_BODY()

public:
    AMovableCube();
    virtual void Tick(float DeltaTime) override;
    void MoveCube(float X, float Y, float Yaw); // from socket

    void EndPlay(const EEndPlayReason::Type EndPlayReason);
    void ResetCube();
    void StartTCPReceiver(int32 Port);  // new
    int32 CubeSocketPort;
    float Speed = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    bool Won = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
    bool Lost = false;

protected:
    virtual void BeginPlay() override;

private:


    FSocket* ListenerSocket;
    FSocket* ConnectionSocket;
    FString SocketName = TEXT("PythonSocket");
    void TCPSocketListener();
    FTimerHandle SocketTimerHandle;

    FTimerHandle ScanTimerHandle;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComponent;


    float CurrentScanAngle = 0.0f;
    int32 RaysPerFrame = 36;
    float RayDistance = 10000.0f;

    int32 FrameCounter = 0;
    TArray<FScanHitResult> PerformTimedScan();
    TArray<FScanHitResult> ScanResults;

    UPROPERTY()
    AActor* WinActor = nullptr;

    FVector WinLocation;
};

USTRUCT(BlueprintType)
struct FScanHitResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector HitLocation;

    UPROPERTY(BlueprintReadOnly)
    FString ActorName;
};