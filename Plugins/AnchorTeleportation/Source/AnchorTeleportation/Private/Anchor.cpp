#include "Anchor.h"
#include "TeleportationSubsystem.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AAnchor::AAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAnchor::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAnchor::RegisterWithSubsystem);
}

void AAnchor::RegisterWithSubsystem()
{
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GetWorld() returned nullptr"));
		return;
	}
	
	AActor* SubsystemOwner = UGameplayStatics::GetActorOfClass(World, ACharacter::StaticClass());
	if (!SubsystemOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid subsystem owner found"));
		World->GetTimerManager().SetTimerForNextTick(this, &AAnchor::RegisterWithSubsystem);
		return;
	}

	UTeleportationSubsystem* TeleportSubsystem = SubsystemOwner->FindComponentByClass<UTeleportationSubsystem>();
	if (!TeleportSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportationSubsystem is nullptr"));
		World->GetTimerManager().SetTimerForNextTick(this, &AAnchor::RegisterWithSubsystem);
		return;
	}
	
	FReplicatedAnchorList* AnchorEntry = TeleportSubsystem->AnchorPairs.FindByPredicate(
		[this](const FReplicatedAnchorList& Entry)
		{
			return Entry.AnchorID == AnchorID;
		});

	if (AnchorEntry)
	{
		if (!AnchorEntry->Anchors.Contains(this))
		{
			AnchorEntry->Anchors.Add(this);
		}
	}
	else
	{
		FReplicatedAnchorList NewEntry;
		NewEntry.AnchorID = AnchorID;
		NewEntry.Anchors.Add(this);
		TeleportSubsystem->AnchorPairs.Add(NewEntry);
	}

	UE_LOG(LogTemp, Log, TEXT("Anchor Registered: %s at Location: %s"),
	       *AnchorID.ToString(), *GetActorLocation().ToString());
}
