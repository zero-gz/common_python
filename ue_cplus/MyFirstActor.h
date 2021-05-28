// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyFirstActor.generated.h"

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
class MY_UE_TEST_RENDER_API AMyFirstActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyFirstActor();

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
