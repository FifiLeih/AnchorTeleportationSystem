// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Anchor.h"
#include "Components/ActorComponent.h"
#include "Pieces/SmallTeleportationPieces.h"
#include "TeleportationSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FReplicatedAnchorList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	FName AnchorID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	TArray<AAnchor*> Anchors;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ANCHORTELEPORTATION_API UTeleportationSubsystem : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	virtual void BeginPlay() override;

public:
	UTeleportationSubsystem();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerTeleportPlayer(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable)
	void ClientRequestTeleport(APlayerController* PlayerController);
	
	AAnchor* FindPairedAnchor(AAnchor* CurrentAnchor) const;
	
	bool CanTeleport(APlayerController* PlayerController) const;
	
	TMap<APlayerController*, float> LastTeleportTimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	bool bPickUpTeleportation = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	float TeleportCooldown = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	USoundCue* TeleportSoundCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	UMaterialInterface* GhostMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	float FadeDuration = 6.f;
	
	UPROPERTY(Replicated)
	uint32 PickedUpPieces = 0;
	
	UFUNCTION(Server, Reliable)
	void ServerCollectTeleportationPiece(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void CollectTeleportationPiece(APlayerController* PlayerController);

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	// UNiagaraSystem* TeleportNiagaraEffect;
	
	UPROPERTY(Replicated)
	TArray<FReplicatedAnchorList> AnchorPairs;

	UPROPERTY()
	ASmallTeleportationPieces* Pieces;

	UFUNCTION(NetMulticast, Reliable)
	void SpawnAfterImage(FVector Location, ACharacter* OriginalCharacter);
};
