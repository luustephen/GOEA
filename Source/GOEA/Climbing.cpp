// Fill out your copyright notice in the Description page of Project Settings.


#include "Climbing.h"

// Sets default values
AClimbing::AClimbing()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AClimbing::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AClimbing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AClimbing::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

