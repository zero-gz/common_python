// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"



UENUM(Meta = (Bitflags))
enum class EColorBits
{
	ECB_Red,
	ECB_Green,
	ECB_Blue
};

UENUM(BlueprintType)
enum class EColor : uint8
{
	Red,
	Green,
	Blue
};

USTRUCT(BlueprintType)
struct FStructTest
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, Meta = (DisplayName = "first param"))
		float x = 0;

	UPROPERTY(EditAnywhere, Meta = (DisplayName = "second param"))
		bool y = false;

	UPROPERTY(EditAnywhere)
		FName Name = FName("test_name");
};

UCLASS()
class NORMAL_START_API AMyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Bitmask, BitmaskEnum = "EColorBits"))
		int32 ColorFlags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EColor EColorData;

	UPROPERTY(EditAnywhere)
		FStructTest testStruct;

	UPROPERTY(BlueprintReadWrite)
		uint8 bIsHungry : 1;

	UPROPERTY(BlueprintReadOnly)
		bool bIsThirsty;


	UPROPERTY(EditAnywhere, Category = "ceshi")
		uint32 bIsOK : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D *texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterial *mt;

	UFUNCTION(BlueprintCallable, meta=(DisplayName="ReadTextureData"))
	void ReadTextureData(UTexture2D* MyTexture2D);

	UFUNCTION(BlueprintCallable)
	void WriteTextureData(FString PackageName, FString TextureName);

	UFUNCTION(BlueprintCallable)
	UTexture2D* LoadTexByPath(FString filepath);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
