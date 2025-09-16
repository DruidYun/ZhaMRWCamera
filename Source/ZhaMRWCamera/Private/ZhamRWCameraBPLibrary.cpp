#include "ZhamRWCameraBPLibrary.h"

#include "HighResScreenshot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
void UZhamRWCameraBPLibrary::SaveCurrentPawnJson(const UObject* WorldContextObject, const FString& FilePath)
{
    if (!WorldContextObject) return;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    APawn* MyPawn = UGameplayStatics::GetPlayerPawn(World, 0);
    if (!MyPawn) return;

    FPawnJsonData Data;
    Data.Name = MyPawn->GetName();
    Data.Location = MyPawn->GetActorLocation();
    Data.Rotation = MyPawn->GetActorRotation();

    if (USpringArmComponent* SpringArm = MyPawn->FindComponentByClass<USpringArmComponent>())
    {
        Data.bHasSpringArm = true;
        Data.SpringArmLength = SpringArm->TargetArmLength;
    }
    else
    {
        Data.bHasSpringArm = false;
        Data.SpringArmLength = 0.f;
    }

    // --- JSON 保存 ---
    TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetStringField(TEXT("Name"), Data.Name);
    JsonObj->SetNumberField(TEXT("LocationX"), Data.Location.X);
    JsonObj->SetNumberField(TEXT("LocationY"), Data.Location.Y);
    JsonObj->SetNumberField(TEXT("LocationZ"), Data.Location.Z);
    JsonObj->SetNumberField(TEXT("RotationPitch"), Data.Rotation.Pitch);
    JsonObj->SetNumberField(TEXT("RotationYaw"), Data.Rotation.Yaw);
    JsonObj->SetNumberField(TEXT("RotationRoll"), Data.Rotation.Roll);
    JsonObj->SetBoolField(TEXT("HasSpringArm"), Data.bHasSpringArm);
    JsonObj->SetNumberField(TEXT("SpringArmLength"), Data.SpringArmLength);

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    JsonArray.Add(MakeShared<FJsonValueObject>(JsonObj));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonArray, Writer);

    FString FinalPath = FilePath.IsEmpty()
        ? FPaths::ProjectContentDir() / TEXT("Config/SaveCamera.json")
        : FilePath;

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(FinalPath), true);

    if (FFileHelper::SaveStringToFile(OutputString, *FinalPath))
    {
        UE_LOG(LogTemp, Log, TEXT("Saved current Pawn JSON to: %s"), *FinalPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save Pawn JSON"));
    }

    // --- 截图 ---
    FString ShotDir = FPaths::ProjectContentDir() / TEXT("Config/Shot/");
    IFileManager::Get().MakeDirectory(*ShotDir, true);

    FString ShotFile = ShotDir / (Data.Name + TEXT(".png"));

    if (GEngine && GEngine->GameViewport)
    {
        FHighResScreenshotConfig& HighResConfig = GetHighResScreenshotConfig();
        HighResConfig.SetResolution(1280, 720, 1.0f); // 720p
        HighResConfig.SetFilename(ShotFile);
        GEngine->GameViewport->Viewport->TakeHighResScreenShot();
        UE_LOG(LogTemp, Log, TEXT("Screenshot saved to: %s"), *ShotFile);
    }
}

TArray<FPawnJsonData> UZhamRWCameraBPLibrary::LoadPawnJsonArray(const FString& FilePath)
{
    TArray<FPawnJsonData> Result;

    FString FinalPath = FilePath.IsEmpty()
        ? FPaths::ProjectContentDir() / TEXT("Config/ReadCamera.json")
        : FilePath;

    if (!FPaths::FileExists(FinalPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("JSON file not found: %s"), *FinalPath);
        return Result;
    }

    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FinalPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to read JSON file: %s"), *FinalPath);
        return Result;
    }

    // 使用手动解析，适配扁平化字段
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    if (!FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON array"));
        return Result;
    }

    for (TSharedPtr<FJsonValue> Value : JsonArray)
    {
        TSharedPtr<FJsonObject> Obj = Value->AsObject();
        if (!Obj.IsValid()) continue;

        FPawnJsonData Data;
        Data.Name = Obj->GetStringField(TEXT("Name"));
        Data.Location.X = Obj->GetNumberField(TEXT("LocationX"));
        Data.Location.Y = Obj->GetNumberField(TEXT("LocationY"));
        Data.Location.Z = Obj->GetNumberField(TEXT("LocationZ"));
        Data.Rotation.Pitch = Obj->GetNumberField(TEXT("RotationPitch"));
        Data.Rotation.Yaw = Obj->GetNumberField(TEXT("RotationYaw"));
        Data.Rotation.Roll = Obj->GetNumberField(TEXT("RotationRoll"));
        Data.bHasSpringArm = Obj->GetBoolField(TEXT("HasSpringArm"));
        Data.SpringArmLength = Obj->GetNumberField(TEXT("SpringArmLength"));

        Result.Add(Data);
    }

    return Result;
}
void UZhamRWCameraBPLibrary::ReplaceWithSaveFirstByName(
    const FString& Name,
    const FString& SaveFilePath,
    const FString& ReadFilePath)
{
    // 默认路径
    FString SavePath = SaveFilePath.IsEmpty()
        ? FPaths::ProjectContentDir() / TEXT("Config/SaveCamera.json")
        : SaveFilePath;

    FString ReadPath = ReadFilePath.IsEmpty()
        ? FPaths::ProjectContentDir() / TEXT("Config/ReadCamera.json")
        : ReadFilePath;

    // 读取 SaveData
    FString SaveString;
    if (!FFileHelper::LoadFileToString(SaveString, *SavePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to read SaveCamera.json: %s"), *SavePath);
        return;
    }

    TSharedRef<TJsonReader<>> SaveReader = TJsonReaderFactory<>::Create(SaveString);
    TArray<TSharedPtr<FJsonValue>> SaveArray;
    if (!FJsonSerializer::Deserialize(SaveReader, SaveArray) || SaveArray.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveCamera.json is empty or invalid: %s"), *SavePath);
        return;
    }

    TSharedPtr<FJsonObject> FirstSaveObj = SaveArray[0]->AsObject();
    if (!FirstSaveObj.IsValid()) return;

    // 读取 ReadData
    FString ReadString;
    if (!FFileHelper::LoadFileToString(ReadString, *ReadPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to read ReadCamera.json: %s"), *ReadPath);
        return;
    }

    TSharedRef<TJsonReader<>> ReadReader = TJsonReaderFactory<>::Create(ReadString);
    TArray<TSharedPtr<FJsonValue>> ReadArray;
    if (!FJsonSerializer::Deserialize(ReadReader, ReadArray) || ReadArray.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReadCamera.json is empty or invalid: %s"), *ReadPath);
        return;
    }

    // 找到 Name 对应的对象
    for (int32 i = 0; i < ReadArray.Num(); ++i)
    {
        TSharedPtr<FJsonObject> Obj = ReadArray[i]->AsObject();
        if (!Obj.IsValid()) continue;

        FString ObjName = Obj->GetStringField(TEXT("Name"));

        if (ObjName == Name)
        {
            // 只替换其他字段，不替换 Name
            Obj->SetNumberField(TEXT("LocationX"), FirstSaveObj->GetNumberField(TEXT("LocationX")));
            Obj->SetNumberField(TEXT("LocationY"), FirstSaveObj->GetNumberField(TEXT("LocationY")));
            Obj->SetNumberField(TEXT("LocationZ"), FirstSaveObj->GetNumberField(TEXT("LocationZ")));

            Obj->SetNumberField(TEXT("RotationPitch"), FirstSaveObj->GetNumberField(TEXT("RotationPitch")));
            Obj->SetNumberField(TEXT("RotationYaw"), FirstSaveObj->GetNumberField(TEXT("RotationYaw")));
            Obj->SetNumberField(TEXT("RotationRoll"), FirstSaveObj->GetNumberField(TEXT("RotationRoll")));

            Obj->SetBoolField(TEXT("HasSpringArm"), FirstSaveObj->GetBoolField(TEXT("HasSpringArm")));
            Obj->SetNumberField(TEXT("SpringArmLength"), FirstSaveObj->GetNumberField(TEXT("SpringArmLength")));

            UE_LOG(LogTemp, Log, TEXT("Replaced ReadData object '%s' fields with first SaveData object."), *Name);
            break; // 替换一个就停
        }
    }

    // 写回 ReadCamera.json
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ReadArray, Writer);
    FFileHelper::EEncodingOptions Encode = FFileHelper::EEncodingOptions::ForceUTF8;
    if (FFileHelper::SaveStringToFile(OutputString, *ReadPath,Encode))
    {
        UE_LOG(LogTemp, Log, TEXT("ReadCamera.json updated successfully: %s"), *ReadPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save updated ReadCamera.json: %s"), *ReadPath);
    }
}