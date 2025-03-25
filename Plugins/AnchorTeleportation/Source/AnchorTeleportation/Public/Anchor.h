// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Anchor.generated.h"

UCLASS(blueprintable)
class ANCHORTELEPORTATION_API AAnchor : public AActor
{
	GENERATED_BODY()
	
public:	
	AAnchor();

protected:
	virtual void BeginPlay() override;

public:    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleportation")
	FName AnchorID;
	
	void RegisterWithSubsystem();
};
