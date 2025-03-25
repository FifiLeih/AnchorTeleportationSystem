// Fill out your copyright notice in the Description page of Project Settings.


#include "Pieces/SmallTeleportationPieces.h"

#include "TeleportationSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

ASmallTeleportationPieces::ASmallTeleportationPieces()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	
	Collider = CreateDefaultSubobject<USphereComponent>(TEXT("Collider"));
	Collider->SetupAttachment(RootComponent);
	Collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collider->SetCollisionObjectType(ECC_WorldDynamic);
	
	Collider->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collider->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	
}

void ASmallTeleportationPieces::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASmallTeleportationPieces, bIsCollected);
}

void ASmallTeleportationPieces::BeginPlay()
{
	Super::BeginPlay();

	Collider->OnComponentBeginOverlap.AddDynamic(this, &ASmallTeleportationPieces::OnSphereOverlap);

	FTimerHandle DespawnTimer;
	GetWorld()->GetTimerManager().SetTimer(DespawnTimer, this, &ASmallTeleportationPieces::DestroyPiece, DespawnTime, false);

	FTimerHandle PickupDelayTimer;
	GetWorld()->GetTimerManager().SetTimer(PickupDelayTimer, this, &ASmallTeleportationPieces::EnablePickup, 1.0f, false);
}

void ASmallTeleportationPieces::EnablePickup()
{
	bCanBePickedUp = true;
}

void ASmallTeleportationPieces::DestroyPiece()
{
	Destroy();
}

void ASmallTeleportationPieces::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character) return;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController) return;

	UTeleportationSubsystem* TeleportSubsystem = Character->FindComponentByClass<UTeleportationSubsystem>();
	if (!TeleportSubsystem) return;
	
	TeleportSubsystem->Pieces = this;
    
	UE_LOG(LogTemp, Log, TEXT("Player is overlapping a small teleportation piece"));
}
