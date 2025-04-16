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
        MeshComponent = Mesh;
    }
}

void AMovableCube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if(Won || Lost)
    {
        if (ConnectionSocket && ConnectionSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
        {
            FString StatusMessage = Won ? TEXT("Win\n") : TEXT("Lose\n");
            FTCHARToUTF8 Convert(*StatusMessage);
            int32 BytesSent = 0;
            ConnectionSocket->Send((uint8*)Convert.Get(), Convert.Length(), BytesSent);
        }
        MeshComponent->SetSimulatePhysics(false);
        return;  // skip the rest of Tick
    }
    FrameCounter++;
    if (FrameCounter % 5 == 0)
    {
        PerformTimedScan();  // every 5th frame
    }

    if (FrameCounter % 5 == 0 && ConnectionSocket && ConnectionSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
    {
        FString Response;
        for (const FScanHitResult& Result : ScanResults)
        {
            Response += FString::Printf(TEXT("%.2f %.2f %.2f %s\n"),
                Result.HitLocation.X,
                Result.HitLocation.Y,
                Result.HitLocation.Z,
                *Result.ActorName);
        }

        Response += TEXT("\n");  // Optional double newline to indicate end of batch

        FTCHARToUTF8 Convert(*Response);
        int32 BytesSent = 0;
        ConnectionSocket->Send((uint8*)Convert.Get(), Convert.Length(), BytesSent);

        ScanResults.Empty();  // Clear after sending
    }
}

TArray<FScanHitResult> AMovableCube::PerformTimedScan()
{
    FVector Origin = GetActorLocation();
    UWorld* World = GetWorld();

    if (!World)
        return ScanResults;

    float AngleStep = 360.0f / (360.0f / RaysPerFrame);

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
            FScanHitResult ScanHit;
            ScanHit.HitLocation = HitResult.ImpactPoint;
            ScanHit.ActorName = HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None");

            ScanResults.Add(ScanHit);

            DrawDebugPoint(World, HitResult.ImpactPoint, 10.f, FColor::Red, true, -1.0f);
        }
    }

    CurrentScanAngle = FMath::Fmod(CurrentScanAngle + RaysPerFrame * AngleStep, 360.0f);
    return ScanResults;
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

        
        }
    }
}


void AMovableCube::MoveCube(float X, float Y, float Yaw)
{

    float DeltaTime = GetWorld()->GetDeltaSeconds();

    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();

    // Calculate the direction from input
    FVector ForceDirection = (Forward * X + Right * Y).GetSafeNormal();

    // Scale force by DeltaTime and Speed for consistent behavior
    float ForceStrength = Speed * DeltaTime * 1000.0f; // 1000 = tuning multiplier
    FVector ForceToApply = ForceDirection * ForceStrength;

    MeshComponent->AddForce(ForceToApply);

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



