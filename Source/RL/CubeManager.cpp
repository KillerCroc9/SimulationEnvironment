#include "CubeManager.h"
#include "MovableCube.h"
#include "Engine/World.h"

ACubeManager::ACubeManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACubeManager::BeginPlay()
{
    Super::BeginPlay();


    UWorld* World = GetWorld();
    if (!World || !CubeClass) return;

    for (int32 i = 0; i < NumCubes; ++i)
    {
        FVector SpawnLocation = FVector(0, 0, 0);  // spread out
        FActorSpawnParameters SpawnParams;
        AMovableCube* Cube = GetWorld()->SpawnActor<AMovableCube>(CubeClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (Cube)
        {
            SpawnedCubes.Add(Cube);
            int32 AssignedPort = StartingPort + i;
            Cube->StartTCPReceiver(AssignedPort);
            UE_LOG(LogTemp, Warning, TEXT("Spawned cube %d on port %d"), i, AssignedPort);
            Cube->Speed = SpeedManager;
        }
    }

    // Start timer to reset scene after 35 seconds
    World->GetTimerManager().SetTimer(ResetTimerHandle, this, &ACubeManager::ResetScene, 35.0f, true);


}


void ACubeManager::ResetScene()
{
    for (AActor* Cube : SpawnedCubes)
    {
        if (AMovableCube* Movable = Cast<AMovableCube>(Cube))
        {
            // Reset physics
            if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Movable->GetRootComponent()))
            {
                MeshComp->SetSimulatePhysics(false);
                MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
                MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
                MeshComp->SetSimulatePhysics(true);
            }

            Movable->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
            Movable->SetActorRotation(FRotator::ZeroRotator);

            // Reset internal states
            Movable->Won = false;
            Movable->Lost = false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Scene reset complete."));
}