// Copyright Epic Games, Inc. All Rights Reserved.

#include "GOEAGameMode.h"
#include "GOEACharacter.h"
#include "UObject/ConstructorHelpers.h"

AGOEAGameMode::AGOEAGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
