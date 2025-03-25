// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmallTeleportationPieces.generated.h"

UCLASS()
class ANCHORTELEPORTATION_API ASmallTeleportationPieces : public AActor
{
	GENERATED_BODY()
	
public:	
	ASmallTeleportationPieces();

protected:
	virtual void BeginPlay() override;
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleAnywhere, Category = "Teleportation")
	UStaticMeshComponent* MeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Teleportation")
	class USphereComponent* Collider;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teleportation")
	bool bIsCollected = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teleportation")
	bool bCanBePickedUp = false;

	int OverlapCheckAttempts = 0;
	
	UFUNCTION()
	void EnablePickup(); // Allows pickup after a delay
	
	UFUNCTION()
	void DestroyPiece();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	float DespawnTime = 15.0f;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
};
