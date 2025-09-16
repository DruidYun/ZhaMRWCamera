#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZhamRWCameraBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FPawnJsonData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	UPROPERTY(BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(BlueprintReadWrite)
	bool bHasSpringArm;

	UPROPERTY(BlueprintReadWrite)
	float SpringArmLength;
};

UCLASS()
class ZHAMRWCAMERA_API UZhamRWCameraBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** 保存当前 PlayerPawn 数据到 JSON */
	UFUNCTION(BlueprintCallable, Category="ZhamRWCamera")
	static void SaveCurrentPawnJson(const UObject* WorldContextObject, const FString& FilePath = TEXT(""));

	/** 从 JSON 文件读取 Pawn 数据数组 */
	UFUNCTION(BlueprintCallable, Category="ZhamRWCamera")
	static TArray<FPawnJsonData> LoadPawnJsonArray(const FString& FilePath = TEXT(""));

	/**
	 * 将 SaveData 的第一个对象替换到 ReadData 中指定 Name 对应的位置
	 * @param Name - 要替换的对象名字
	 * @param SaveFilePath - 保存文件路径，可选，默认 Content/Config/SaveCamera.json
	 * @param ReadFilePath - 读取文件路径，可选，默认 Content/Config/ReadCamera.json
	 */
	UFUNCTION(BlueprintCallable, Category="ZhamRWCamera")
	static void ReplaceWithSaveFirstByName(
		const FString& Name,
		const FString& SaveFilePath = TEXT(""),
		const FString& ReadFilePath = TEXT("")
	);
};
