// Copyright Epic Games, Inc. All Rights Reserved.

#include "AddonGameMode.h"
#include "AddonCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAddonGameMode::AAddonGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
