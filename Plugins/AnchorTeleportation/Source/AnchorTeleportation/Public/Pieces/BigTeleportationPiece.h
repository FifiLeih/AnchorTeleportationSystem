// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SmallTeleportationPieces.h"
#include "GameFramework/Actor.h"
#include "BigTeleportationPiece.generated.h"

UCLASS()
class ANCHORTELEPORTATION_API ABigTeleportationPiece : public AActor
{
	GENERATED_BODY()
	
public:	
	ABigTeleportationPiece();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:

	UPROPERTY(VisibleAnywhere, Category = "Teleportation")
	UStaticMeshComponent* MeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Teleportation")
	class UBoxComponent* Collider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	TArray<TSubclassOf<AActor>> AllowedColliders;

	UFUNCTION()
	void BreakSource(APlayerController* InstigatorPlayer, FVector SpawnReferenceLocation);
	
	UFUNCTION(Server, Reliable)
	void ServerBreakSource(APlayerController* InstigatorPlayer, FVector SpawnReferenceLocation);

	UFUNCTION()
	bool FindSafeSpawnLocation(FVector PlayerLocation, FVector& OutLocation);

	UFUNCTION()
	void DestroyAndRespawnSource();

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	TSubclassOf<ASmallTeleportationPieces> PieceClass; // The collectible piece

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	int32 MaxPieces = 3; // How many pieces spawn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	float RespawnTime = 10.0f; // How long before respawning
	
};
