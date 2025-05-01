#include "CubeManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "MovableCube.h"

// Constructor
ACubeManager::ACubeManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

// BeginPlay: Spawn cubes and start periodic data sending
void ACubeManager::BeginPlay()
{
    Super::BeginPlay();

    for (int32 i = 0; i < NumCubes; ++i)
    {
        if (!CubeClass) continue;

        FVector SpawnLocation = FVector(0, 0.0f, 0); // Offset each cube
        FActorSpawnParameters SpawnParams;
        AActor* SpawnedCube = GetWorld()->SpawnActor<AActor>(CubeClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (SpawnedCube)
        {
            SpawnedCubes.Add(SpawnedCube);
        }
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Win"), FoundActors);
    if (FoundActors.Num() > 0)
    {
        WinActor = FoundActors[0];
        WinLocation = WinActor->GetActorLocation();
    }

    // Schedule ResetScene() after 35 seconds
    GetWorld()->GetTimerManager().SetTimer(
        ResetTimerHandle,
        this,
        &ACubeManager::ResetScene,
        35.0f,
        true  // not looping
    );
    // Set timer to send data every 1 second
}
void ACubeManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    FrameCounter++;
    if (FrameCounter % 5 == 0) 
    {
        BuildAndSendJSON();
    }
}
void ACubeManager::BuildAndSendJSON()
{
    TArray<TSharedPtr<FJsonValue>> CubeArray;

    for (int32 i = 0; i < SpawnedCubes.Num(); ++i)
    {
        AActor* Cube = SpawnedCubes[i];
        if (!Cube) continue;

        TSharedPtr<FJsonObject> CubeObj = MakeShareable(new FJsonObject);

        // Player_Location block
        FVector Loc = Cube->GetActorLocation();
        TSharedPtr<FJsonObject> PlayerLocationObj = MakeShareable(new FJsonObject);
        PlayerLocationObj->SetNumberField("x", Loc.X);
        PlayerLocationObj->SetNumberField("y", Loc.Y);
        PlayerLocationObj->SetNumberField("z", Loc.Z);
        CubeObj->SetObjectField("Player_Location", PlayerLocationObj);

        // Win_Location block
        TSharedPtr<FJsonObject> WinObj = MakeShareable(new FJsonObject);
        WinObj->SetNumberField("x", WinLocation.X);
        WinObj->SetNumberField("y", WinLocation.Y);
        WinObj->SetNumberField("z", WinLocation.Z);
        CubeObj->SetObjectField("win_location", WinObj);

        // ID
        CubeObj->SetNumberField("id", i);


        // Scan Results
        AMovableCube* MovableCube = Cast<AMovableCube>(Cube);
        if (MovableCube)
        {
            // ?? Add win/lose/playing status
            FString Status = TEXT("playing");
            if (MovableCube->Won)
                Status = TEXT("win");
            else if (MovableCube->Lost)
                Status = TEXT("lose");

            CubeObj->SetStringField("status", Status);

         
                const TArray<FScanHitResult> ScanResults = MovableCube->PerformTimedScan();
                TArray<TSharedPtr<FJsonValue>> ScanArray;

                for (const FScanHitResult& Hit : ScanResults)
                {
                    TSharedPtr<FJsonObject> HitObj = MakeShareable(new FJsonObject);
                    HitObj->SetNumberField("x", Hit.HitLocation.X);
                    HitObj->SetNumberField("y", Hit.HitLocation.Y);
                    HitObj->SetNumberField("z", Hit.HitLocation.Z);
                    HitObj->SetStringField("actor", Hit.ActorName);
                    ScanArray.Add(MakeShareable(new FJsonValueObject(HitObj)));
                }

                CubeObj->SetArrayField("scan_results", ScanArray);
        }

        CubeArray.Add(MakeShareable(new FJsonValueObject(CubeObj)));
    }

    // Final root object
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetArrayField("cubes", CubeArray);

    // Serialize JSON to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    // Send HTTP POST to Flask
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("http://127.0.0.1:8000/receive_data");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(OutputString);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool Success)
        {
            if (Success && Resp.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("Sent data to Flask, response: %s"), *Resp->GetContentAsString());
                PollPythonForMove();  // ?? Only pull move command if POST succeeded
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to send data to Flask"));
            }
        });

    Request->ProcessRequest();

}


void ACubeManager::PollPythonForMove()
{
    FString URL = TEXT("http://127.0.0.1:8000/get_move");

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(URL);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool Success)
        {
            if (Success && Resp.IsValid())
            {
                FString JsonStr = Resp->GetContentAsString();
                ReceiveMoveCommand(JsonStr);  // ?? parse and move
            }
        });

    Request->ProcessRequest();
}
void ACubeManager::ReceiveMoveCommand(const FString& JsonStr)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid move command JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Commands;
    if (!Root->TryGetArrayField("commands", Commands)) return;

    for (const TSharedPtr<FJsonValue>& CommandVal : *Commands)
    {
        TSharedPtr<FJsonObject> Cmd = CommandVal->AsObject();
        if (!Cmd.IsValid()) continue;

        int32 CubeID = Cmd->GetIntegerField("id");
        float X = Cmd->GetNumberField("x");
        float Y = Cmd->GetNumberField("y");
        float Yaw = Cmd->GetNumberField("yaw");

        if (SpawnedCubes.IsValidIndex(CubeID))
        {
            if (AMovableCube* Cube = Cast<AMovableCube>(SpawnedCubes[CubeID]))
            {
                Cube->MoveCube(X, Y, Yaw);
            }
        }
    }
}



// Optional: ResetScene logic
void ACubeManager::ResetScene()
{
    for (AActor* Cube : SpawnedCubes)
    {
        if (Cube)
        {
            Cube->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
        }
    }
}
