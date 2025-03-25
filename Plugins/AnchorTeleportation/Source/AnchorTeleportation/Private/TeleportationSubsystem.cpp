#include "TeleportationSubsystem.h"
#include "Anchor.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Pieces/SmallTeleportationPieces.h"
#include "Sound/SoundCue.h"

UTeleportationSubsystem::UTeleportationSubsystem()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTeleportationSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTeleportationSubsystem, AnchorPairs);
	DOREPLIFETIME(UTeleportationSubsystem, PickedUpPieces);
}

void UTeleportationSubsystem::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		for (TActorIterator<AAnchor> It(GetWorld()); It; ++It)
		{
			AAnchor* Anchor = *It;
			if (Anchor)
			{
				FReplicatedAnchorList* ExistingPair = AnchorPairs.FindByPredicate(
					[Anchor](const FReplicatedAnchorList& Pair) { return Pair.AnchorID == Anchor->AnchorID; });

				if (!ExistingPair)
				{
					FReplicatedAnchorList NewPair;
					NewPair.AnchorID = Anchor->AnchorID;
					NewPair.Anchors.Add(Anchor);
					AnchorPairs.Add(NewPair);
				}
				else
				{
					ExistingPair->Anchors.Add(Anchor);
				}
			}
		}
	}
}

void UTeleportationSubsystem::ClientRequestTeleport(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is NULL"));
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player's character is NULL"));
		return;
	}
	
	if (!this)
	{
		UE_LOG(LogTemp, Error, TEXT("TeleportationSubsystem is NULL"));
		return;
	}
	
	if (AnchorPairs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No anchor pairs available"));
		return;
	}
	
	if (PlayerController->IsLocalController() && !PlayerController->HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("Requesting teleport from server"));
		ServerTeleportPlayer(PlayerController);
		return;
	}
	
	if (PlayerController->HasAuthority())
	{
		ServerTeleportPlayer(PlayerController);
		return;
	}
}

AAnchor* UTeleportationSubsystem::FindPairedAnchor(AAnchor* CurrentAnchor) const
{
	if (!CurrentAnchor)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentAnchor is nullptr"));
		return nullptr;
	}

	const FReplicatedAnchorList* AnchorEntry = AnchorPairs.FindByPredicate(
		[CurrentAnchor](const FReplicatedAnchorList& Entry)
		{
			return Entry.AnchorID == CurrentAnchor->AnchorID;
		});

	if (!AnchorEntry || AnchorEntry->Anchors.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid anchor pair found for %s"),
		       *CurrentAnchor->AnchorID.ToString());
		return nullptr;
	}

	for (AAnchor* Anchor : AnchorEntry->Anchors)
	{
		if (Anchor && Anchor != CurrentAnchor)
		{
			UE_LOG(LogTemp, Log, TEXT("Found Paired Anchor: %s -> %s"),
			       *CurrentAnchor->AnchorID.ToString(), *Anchor->AnchorID.ToString());
			return Anchor;
		}
	}

	return nullptr;
}

bool UTeleportationSubsystem::CanTeleport(APlayerController* PlayerController) const
{
	if (!PlayerController) return false;
	
	if (bPickUpTeleportation)
	{
		if (PickedUpPieces > 0)
		{
			return true;
		}
		return false;
	}
	
	const float* LastTeleportTime = LastTeleportTimes.Find(PlayerController);
	if (LastTeleportTime)
	{
		return (GetWorld()->GetTimeSeconds() - *LastTeleportTime) >= TeleportCooldown;
	}
	return true;
}

void UTeleportationSubsystem::CollectTeleportationPiece(APlayerController* PlayerController)
{

	UE_LOG(LogTemp, Log, TEXT("CollectTeleportationPiece called"));

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is NULL"));
		return;
	}

	if (!Pieces)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pieces is NULL, ensure overlap "));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character || !Pieces->Collider->IsOverlappingActor(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is not overlapping the piece"));
		return;
	}

	if (!Pieces->bCanBePickedUp)
	{
		UE_LOG(LogTemp, Warning, TEXT("The piece is not yet pickable"));
		return;
	}
	
	if (!PlayerController || !Pieces) return;

	//ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character || !Pieces->Collider->IsOverlappingActor(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is not overlapping the piece"));
		return;
	}

	if (PlayerController->HasAuthority())
	{
		if (Pieces->bIsCollected) return;
		
		Pieces->bIsCollected = true;
		PickedUpPieces++;

		UE_LOG(LogTemp, Log, TEXT("✅ Picked Up Pieces: %d"), PickedUpPieces);
		
		Pieces->Destroy();
		
		Pieces = nullptr;
	}
	else
	{
		ServerCollectTeleportationPiece(PlayerController);
	}
}

void UTeleportationSubsystem::ServerCollectTeleportationPiece_Implementation(APlayerController* PlayerController)
{
	UE_LOG(LogTemp, Log, TEXT("CollectTeleportationPiece called"));

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is NULL"));
		return;
	}

	if (!Pieces)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pieces is NULL, ensure overlap"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character || !Pieces->Collider->IsOverlappingActor(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is not overlapping the piece"));
		return;
	}

	if (!Pieces->bCanBePickedUp)
	{
		UE_LOG(LogTemp, Warning, TEXT("he piece is not yet pickable"));
		return;
	}

	if (!PlayerController || !Pieces) return;

	//ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character || !Pieces->Collider->IsOverlappingActor(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is not overlapping the piece"));
		return;
	}
	
	if (Pieces->bIsCollected) return;
	
	Pieces->bIsCollected = true;
	PickedUpPieces++;

	UE_LOG(LogTemp, Log, TEXT("Picked Up Pieces: %d"), PickedUpPieces);
	
	Pieces->Destroy();
	
	Pieces = nullptr;
}

void UTeleportationSubsystem::ServerTeleportPlayer_Implementation(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerController is NULL"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character is NULL"));
		return;
	}
	
	if (bPickUpTeleportation)
	{
		if (PickedUpPieces > 0)
		{
			PickedUpPieces--;
			UE_LOG(LogTemp, Log, TEXT("Used a teleportation charge. Remaining: %d"),
			       PickedUpPieces);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No teleportation charges left"));
			return;
		}
	}
	else
	{
		LastTeleportTimes.Add(PlayerController, GetWorld()->GetTimeSeconds());
	}

	FVector PlayerLocation = Character->GetActorLocation();
	AAnchor* ClosestAnchor = nullptr;
	float MinDistance = FLT_MAX;

	for (const FReplicatedAnchorList& AnchorList : AnchorPairs)
	{
		for (AAnchor* Anchor : AnchorList.Anchors)
		{
			if (Anchor)
			{
				float Dist = FVector::Dist(PlayerLocation, Anchor->GetActorLocation());
				if (Dist < MinDistance)
				{
					MinDistance = Dist;
					ClosestAnchor = Anchor;
				}
			}
		}
	}

	if (!ClosestAnchor)
	{
		UE_LOG(LogTemp, Warning, TEXT("No closest anchor found"));
		return;
	}

	AAnchor* TargetAnchor = FindPairedAnchor(ClosestAnchor);
	if (!TargetAnchor)
	{
		UE_LOG(LogTemp, Warning, TEXT("No paired anchor found for %s"),
		       *ClosestAnchor->AnchorID.ToString());
		return;
	}

	SpawnAfterImage(PlayerLocation, Character);

	Character->SetActorLocation(TargetAnchor->GetActorLocation());

	if (TeleportSoundCue)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TeleportSoundCue, TargetAnchor->GetActorLocation());
	}

	UE_LOG(LogTemp, Log, TEXT("✅ ServerTeleportPlayer: %s teleported from %s to %s"),
	       *Character->GetName(), *ClosestAnchor->GetActorLocation().ToString(),
	       *TargetAnchor->GetActorLocation().ToString());
}

void UTeleportationSubsystem::SpawnAfterImage_Implementation(FVector Location, ACharacter* OriginalCharacter)
{
    if (!OriginalCharacter) return;

    UWorld* World = GetWorld();
    if (!World) return;
	
    ACharacter* GhostCharacter = World->SpawnActor<ACharacter>(OriginalCharacter->GetClass(), Location,
                                                               OriginalCharacter->GetActorRotation());
    if (!GhostCharacter) return;

    GhostCharacter->SetActorEnableCollision(false);
    GhostCharacter->SetReplicates(false);
    GhostCharacter->GetMesh()->SetRenderCustomDepth(true);
	
    UMaterialInstanceDynamic* DynamicMaterial = GhostCharacter->GetMesh()->CreateDynamicMaterialInstance(
        0, GhostMaterial);
    if (!DynamicMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create dynamic material instance"));
        GhostCharacter->Destroy();
        return;
    }
	
    GhostCharacter->GetMesh()->SetMaterial(0, DynamicMaterial);
	
    DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
	
    float LocalFadeDuration = FadeDuration;
    float FadeStepTime = 0.05f;
	
    TWeakObjectPtr<ACharacter> WeakGhostCharacter(GhostCharacter);
	
    TSharedPtr<FTimerHandle> FadeTimerHandle = MakeShared<FTimerHandle>();
	
    float* ElapsedTime = new float(0.0f);

    World->GetTimerManager().SetTimer(*FadeTimerHandle, [WeakGhostCharacter, DynamicMaterial, World, LocalFadeDuration, FadeStepTime, FadeTimerHandle, ElapsedTime]()
    {
        if (!WeakGhostCharacter.IsValid() || !DynamicMaterial) 
        {
            World->GetTimerManager().ClearTimer(*FadeTimerHandle);
            delete ElapsedTime;
            return;
        }

        ACharacter* GhostChar = WeakGhostCharacter.Get();
        if (!GhostChar) return;
    	
        *ElapsedTime += FadeStepTime;
    	
        float NewOpacity = FMath::Clamp(1.0f - (*ElapsedTime / LocalFadeDuration), 0.0f, 1.0f);
        DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), NewOpacity);
    	
        if (*ElapsedTime >= LocalFadeDuration)
        {
            GhostChar->Destroy();
            World->GetTimerManager().ClearTimer(*FadeTimerHandle);
            delete ElapsedTime;
        }
    }, FadeStepTime, true);
}