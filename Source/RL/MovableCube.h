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
    void StartTCPReceiver(int32 Port);  // new
    int32 CubeSocketPort;




protected:
    virtual void BeginPlay() override;

private:
    FVector MovementInput;
    FRotator RotationInput;

    FSocket* ListenerSocket;
    FSocket* ConnectionSocket;
    FString SocketName = TEXT("PythonSocket");
    void TCPSocketListener();
    FTimerHandle SocketTimerHandle;
    void Perform360RayScan(float Distance = 1000.0f, int32 NumRays = 36); // 360° scan

    FTimerHandle ScanTimerHandle;

    float CurrentScanAngle = 0.0f;
    int32 RaysPerFrame = 15;
    float RayDistance = 1000.0f;

    void PerformTimedScan(); // Called from timer
};