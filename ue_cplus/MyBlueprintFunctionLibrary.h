// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class NORMAL_START_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void ReadTextureDataByFile(FString texturePath, TArray<uint8>& colors, int32& TextureWidth, int32& TextureHeight);

	UFUNCTION(BlueprintCallable)
	static void ReadTextureDataByTexture2D(UTexture2D* TextureToRead, TArray<uint8>& colors, int32& TextureWidth, int32& TextureHeight, int MipLevel = 0);

	UFUNCTION(BlueprintCallable)
	static UTexture2D* WriteTexture2DData(UTexture2D* TextureToWrite, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight, int MipLevel = 0);

	UFUNCTION(BlueprintCallable)
	static void SaveTextureAssets(FString PackageName, FString TextureName, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight);
	
	UFUNCTION(BlueprintCallable)
	static bool SavePNGImage(FString ImagePath, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight);
};
