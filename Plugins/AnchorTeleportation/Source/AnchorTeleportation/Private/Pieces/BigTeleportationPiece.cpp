// Fill out your copyright notice in the Description page of Project Settings.


#include "Pieces/BigTeleportationPiece.h"
#include "TeleportationSubsystem.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ABigTeleportationPiece::ABigTeleportationPiece()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	Collider = CreateDefaultSubobject<UBoxComponent>(TEXT("Collider"));
	Collider->SetupAttachment(RootComponent);
	Collider->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collider->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Collider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	Collider->OnComponentHit.AddDynamic(this, &ABigTeleportationPiece::OnHit);
}

void ABigTeleportationPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABigTeleportationPiece::BeginPlay()
{
	Super::BeginPlay();
}

void ABigTeleportationPiece::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || !HasAuthority()) return;

	// 🔹 Check if the hitting actor is in the AllowedColliders list
	bool bIsAllowed = false;
	for (TSubclassOf<AActor> AllowedType : AllowedColliders)
	{
		if (OtherActor->IsA(AllowedType))
		{
			bIsAllowed = true;
			break;
		}
	}

	if (!bIsAllowed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collision ignored, actor %s is not allowed"),
			   *OtherActor->GetName());
		return;
	}
	
	APlayerController* InstigatorPlayer = nullptr;
	FVector SpawnReferenceLocation = Hit.ImpactPoint;
	
	if (ACharacter* HittingCharacter = Cast<ACharacter>(OtherActor))
	{
		InstigatorPlayer = Cast<APlayerController>(HittingCharacter->GetController());
		if (InstigatorPlayer)
		{
			SpawnReferenceLocation = HittingCharacter->GetActorLocation();
		}
	}
	
	if (!InstigatorPlayer && OtherActor->GetInstigatorController())
	{
		InstigatorPlayer = Cast<APlayerController>(OtherActor->GetInstigatorController());
	}

	if (!InstigatorPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid instigator found for collision"));
		return;
	}
	
	ServerBreakSource(InstigatorPlayer, SpawnReferenceLocation);
}

void ABigTeleportationPiece::BreakSource(APlayerController* InstigatorPlayer, FVector SpawnReferenceLocation)
{
	if (HasAuthority())
	{
		if (!PieceClass) return;

		UWorld* World = GetWorld();
		if (!World) return;

	
		int32 NumPieces = FMath::RandRange(1, MaxPieces);
		
		for (int32 i = 0; i < NumPieces; i++)
		{
			FVector SafeSpawnLocation;
			if (FindSafeSpawnLocation(SpawnReferenceLocation, SafeSpawnLocation))
			{
				ASmallTeleportationPieces* SmallPiece = World->SpawnActor<ASmallTeleportationPieces>(
					PieceClass, SafeSpawnLocation, FRotator::ZeroRotator);

				if (SmallPiece)
				{
					UE_LOG(LogTemp, Log, TEXT("Small Teleportation Piece Spawned at %s"), *SafeSpawnLocation.ToString());
					
					SmallPiece->SetReplicates(true);
					SmallPiece->SetReplicateMovement(true);
				}
			}
		}
		DestroyAndRespawnSource();
	}
	else
	{
		ServerBreakSource(InstigatorPlayer, SpawnReferenceLocation);
	}
}

void ABigTeleportationPiece::ServerBreakSource_Implementation(APlayerController* InstigatorPlayer, FVector SpawnReferenceLocation)
{
	if (!PieceClass) return;

	UWorld* World = GetWorld();
	if (!World) return;
	
	int32 NumPieces = FMath::RandRange(1, MaxPieces);
	
	for (int32 i = 0; i < NumPieces; i++)
	{
		FVector SafeSpawnLocation;
		if (FindSafeSpawnLocation(SpawnReferenceLocation, SafeSpawnLocation))
		{
			ASmallTeleportationPieces* SmallPiece = World->SpawnActor<ASmallTeleportationPieces>(
				PieceClass, SafeSpawnLocation, FRotator::ZeroRotator);

			if (SmallPiece)
			{
				UE_LOG(LogTemp, Log, TEXT("Small Teleportation Piece Spawned at %s"), *SafeSpawnLocation.ToString());
				
				SmallPiece->SetReplicates(true);
				SmallPiece->SetReplicateMovement(true);
			}
		}
	}
	DestroyAndRespawnSource();
}

bool ABigTeleportationPiece::FindSafeSpawnLocation(FVector PlayerLocation, FVector& OutLocation)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	const float MinDistance = 150.0f;
	const float MaxDistance = 300.0f;
	const float MaxSpawnHeightOffset = 30.0f;
	const float TraceDistance = 400.0f;

	for (int32 Attempt = 0; Attempt < 10; Attempt++)
	{
		FVector RandomDirection = FMath::VRand();
		RandomDirection.Z = 0;

		FVector SpawnLocation = PlayerLocation + RandomDirection * FMath::RandRange(MinDistance, MaxDistance);
		FVector TraceStart = SpawnLocation + FVector(0, 0, MaxSpawnHeightOffset);
		FVector TraceEnd = SpawnLocation - FVector(0, 0, TraceDistance);
		
		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

		if (bHit && HitResult.bBlockingHit)
		{
			OutLocation = HitResult.ImpactPoint + FVector(0, 0, 10);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Failed to find a safe spawn location"));
	return false;
}

void ABigTeleportationPiece::DestroyAndRespawnSource()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector RespawnLocation = GetActorLocation();
	FRotator RespawnRotation = GetActorRotation();
	
	FTimerHandle RespawnTimer;
	World->GetTimerManager().SetTimer(RespawnTimer, [World, RespawnLocation, RespawnRotation, this]()
	{
		ABigTeleportationPiece* NewPiece = World->SpawnActor<ABigTeleportationPiece>(GetClass(), RespawnLocation, RespawnRotation);
		if (NewPiece)
		{
			UE_LOG(LogTemp, Log, TEXT("Big Teleportation Piece Respawned"));
		}
	}, RespawnTime, false);
	
	Destroy();
}
