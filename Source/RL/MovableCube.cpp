// MovableCube.cpp
#include "MovableCube.h"
#include "DrawDebugHelpers.h"


AMovableCube::AMovableCube()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AMovableCube::BeginPlay()
{
    Super::BeginPlay();
    if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(GetRootComponent()))
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionObjectType(ECC_GameTraceChannel1);  // Custom channel
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);  // Ignore other cubes
        Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    }
    GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &AMovableCube::PerformTimedScan, 1.f, true);

}

void AMovableCube::PerformTimedScan()
{
    FVector Origin = GetActorLocation();
    UWorld* World = GetWorld();
    if (!World) return;

    float AngleStep = 360.0f / (360.0f / RaysPerFrame);  // = 24 deg if 15 rays/frame

    for (int32 i = 0; i < RaysPerFrame; ++i)
    {
        float Angle = CurrentScanAngle + i * AngleStep;
        float Radians = FMath::DegreesToRadians(FMath::Fmod(Angle, 360.0f));
        FVector Direction = FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f);
        FVector End = Origin + Direction * RayDistance;

        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        bool bHit = World->LineTraceSingleByChannel(HitResult, Origin, End, ECC_Visibility, Params);

        if (bHit)
        {
            FVector HitLocation = HitResult.ImpactPoint;
            UE_LOG(LogTemp, Log, TEXT("Scan Hit: %s (From: %s)"), *HitLocation.ToString(), *Origin.ToString());

            DrawDebugLine(World, Origin, HitLocation, FColor::Green, true, -1.0f, 0, 2.0f);
            DrawDebugPoint(World, HitLocation, 10.f, FColor::Red, true, -1.0f);  // or even 1.0f
        }
        else
        {
            DrawDebugLine(World, Origin, End, FColor::Blue, true, -1.0f, 0, 2.f);
        }
    }

    // Advance scan angle
    CurrentScanAngle = FMath::Fmod(CurrentScanAngle + RaysPerFrame * AngleStep, 360.0f);
}

void AMovableCube::StartTCPReceiver(int32 Port)
{
    CubeSocketPort = Port;
    FIPv4Endpoint Endpoint(FIPv4Address::Any, Port);

    ListenerSocket = FTcpSocketBuilder(*SocketName)
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .Listening(8);

    if (ListenerSocket)
    {
        GetWorld()->GetTimerManager().SetTimer(SocketTimerHandle, this, &AMovableCube::TCPSocketListener, 0.01f, true);
        UE_LOG(LogTemp, Warning, TEXT("Cube [%d] listening on port %d"), GetUniqueID(), Port);
    }
}

void AMovableCube::TCPSocketListener()
{
    // ✅ Check if existing socket has disconnected
    if (ConnectionSocket)
    {
        if (ConnectionSocket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
        {
            UE_LOG(LogTemp, Warning, TEXT("Python disconnected. Closing socket."));
            ConnectionSocket->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
            ConnectionSocket = nullptr;
        }
    }

    // ✅ Try to accept a new connection if we don't have one
    if (!ConnectionSocket)
    {
        bool bPending;
        if (ListenerSocket && ListenerSocket->HasPendingConnection(bPending) && bPending)
        {
            ConnectionSocket = ListenerSocket->Accept(TEXT("PythonConnection"));
            UE_LOG(LogTemp, Warning, TEXT("Accepted new connection from Python!"));
        }
    }

    // ✅ Process data if connected
    if (ConnectionSocket && ConnectionSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
    {
        TArray<uint8> ReceivedData;
        uint32 Size;
        while (ConnectionSocket->HasPendingData(Size))
        {
            ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
            int32 Read = 0;
            ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);

            FString ReceivedString = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
            UE_LOG(LogTemp, Warning, TEXT("[Python] Received: %s"), *ReceivedString);

            // Optional: parse input
            TArray<FString> Parsed;
            ReceivedString.ParseIntoArray(Parsed, TEXT(" "), true);
            if (Parsed.Num() == 3)
            {
                float X = FCString::Atof(*Parsed[0]);
                float Y = FCString::Atof(*Parsed[1]);
                float Yaw = FCString::Atof(*Parsed[2]);

                MoveCube(X, Y, Yaw);
            }

            // ✅ Send reply
            const TCHAR* Response = TEXT("OK\n");
            FTCHARToUTF8 Convert(*FString(Response));
            int32 BytesSent = 0;
            ConnectionSocket->Send((uint8*)Convert.Get(), Convert.Length(), BytesSent);
        }
    }
}

void AMovableCube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AddActorWorldOffset(MovementInput * DeltaTime, true);
    AddActorLocalRotation(RotationInput * DeltaTime);
}

void AMovableCube::MoveCube(float X, float Y, float Yaw)
{
    FVector Force = FVector(X * 1000000.0f, Y * 1000000.0f, 0.0f); // Increase by 10×
    FVector Torque = FVector(0.0f, 0.0f, Yaw * 1000000.0f);      // Increase by 10×


    if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(GetRootComponent()))
    {
        Mesh->AddForce(Force);
        Mesh->AddTorqueInRadians(Torque);
    }
}


void AMovableCube::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ConnectionSocket)
    {
        ConnectionSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
        ConnectionSocket = nullptr;
    }

    if (ListenerSocket)
    {
        ListenerSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
        ListenerSocket = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}



void AMovableCube::Perform360RayScan(float Distance, int32 NumRays)
{
    FVector Origin = GetActorLocation();
    FRotator Rotation = GetActorRotation();
    UWorld* World = GetWorld();

    if (!World) return;

    float AngleStep = 360.0f / NumRays;

    for (int32 i = 0; i < NumRays; ++i)
    {
        float Angle = i * AngleStep;
        float Radians = FMath::DegreesToRadians(Angle);
        FVector Direction = FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f);

        FVector End = Origin + Direction * Distance;

        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this); // Don't hit yourself

        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            Origin,
            End,
            ECC_Visibility,
            Params
        );

        if (bHit)
        {
            FVector HitLocation = HitResult.ImpactPoint;

            UE_LOG(LogTemp, Log, TEXT("Ray %d Hit at: %s (From: %s)"),
                i, *HitLocation.ToString(), *Origin.ToString());

            // Optional: draw hit
            DrawDebugLine(World, Origin, HitLocation, FColor::Green, false, 0.1f, 0, 1.0f);
            DrawDebugPoint(World, HitLocation, 10.f, FColor::Red, false, 0.1f);
        }
        else
        {
            // Optional: draw a miss line
            DrawDebugLine(World, Origin, End, FColor::Blue, false, 0.1f, 0, 0.5f);
        }
    }
}