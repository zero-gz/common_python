// Fill out your copyright notice in the Description page of Project Settings.


#include "MyFirstActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyFirstActor, Log, All);

// Sets default values
AMyFirstActor::AMyFirstActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	UE_LOG(LogMyFirstActor, Log, TEXT("first actor in init function!"));
}

// Called when the game starts or when spawned
void AMyFirstActor::BeginPlay()
{
	Super::BeginPlay();	
}

// Called every frame
void AMyFirstActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

