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

    for (int32 i = 0; i < NumCubes; ++i)
    {
        FVector SpawnLocation = FVector(0, 0, 0);  // spread out
        FActorSpawnParameters SpawnParams;
        AMovableCube* Cube = GetWorld()->SpawnActor<AMovableCube>(CubeClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (Cube)
        {
            int32 AssignedPort = StartingPort + i;
            Cube->StartTCPReceiver(AssignedPort);
            UE_LOG(LogTemp, Warning, TEXT("Spawned cube %d on port %d"), i, AssignedPort);
            Cube->Speed = SpeedManager;
        }
    }
}
