#include "Vision/MobaVisionSubsystem.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Player/MobaPlayerState.h"
#include "Structures/MobaStructure.h"
#include "Units/MobaMinion.h"
#include "Units/MobaMonster.h"
#include "Vision/MobaBush.h"
#include "Vision/MobaWard.h"

namespace
{
	constexpr float UpdateInterval = 0.1f;

	constexpr float ChampionSightRadius = 1350.0f;
	constexpr float MinionSightRadius = 1100.0f;
	constexpr float StructureSightRadius = 1300.0f;
	constexpr float WardSightRadius = 1000.0f;

	/** 벽에 가려지는지 볼 때 쓰는 높이. 바닥 표시물보다 높고 벽보다 낮아야 한다. */
	constexpr float SightTraceHeight = 70.0f;
}

UMobaVisionSubsystem* UMobaVisionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UMobaVisionSubsystem>() : nullptr;
}

TStatId UMobaVisionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMobaVisionSubsystem, STATGROUP_Tickables);
}

bool UMobaVisionSubsystem::IsVisibleToLocalPlayer(const AActor* Actor) const
{
	return !HiddenActors.Contains(const_cast<AActor*>(Actor));
}

void UMobaVisionSubsystem::RegisterBush(AMobaBush* Bush)
{
	// 비트 하나에 부쉬 하나를 대응시키므로 64개까지만 다룬다.
	if (Bush && Bushes.Num() < 64)
	{
		Bushes.AddUnique(Bush);
	}
}

void UMobaVisionSubsystem::UnregisterBush(AMobaBush* Bush)
{
	Bushes.Remove(Bush);
}

void UMobaVisionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	TimeSinceUpdate += DeltaTime;
	if (TimeSinceUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceUpdate = 0.0f;

	// 화면이 없는 서버이거나 아직 팀이 정해지지 않았으면 아무것도 숨기지 않는다.
	const APlayerController* LocalController = World->GetFirstPlayerController();
	const AMobaPlayerState* LocalState = LocalController && LocalController->IsLocalController() ? LocalController->GetPlayerState<AMobaPlayerState>() : nullptr;
	UpdateVision(LocalState ? LocalState->GetTeam() : EMobaTeam::None);
}

uint64 UMobaVisionSubsystem::GetBushMask(const FVector& Location) const
{
	uint64 Mask = 0;
	for (int32 Index = 0; Index < Bushes.Num(); ++Index)
	{
		const AMobaBush* Bush = Bushes[Index].Get();
		if (Bush && Bush->ContainsLocation(Location))
		{
			Mask |= uint64(1) << Index;
		}
	}
	return Mask;
}

bool UMobaVisionSubsystem::CanAnySourceSee(const TArray<FVisionSource>& Sources, const FVector& TargetLocation) const
{
	const uint64 TargetBushMask = GetBushMask(TargetLocation);
	const FVector TraceEnd(TargetLocation.X, TargetLocation.Y, SightTraceHeight);

	for (const FVisionSource& Source : Sources)
	{
		if (FVector::DistSquared2D(Source.Location, TargetLocation) > FMath::Square(Source.Radius))
		{
			continue;
		}

		// 부쉬 안의 대상은 같은 부쉬 안에서만 보인다.
		if (TargetBushMask != 0 && (TargetBushMask & Source.BushMask) == 0)
		{
			continue;
		}

		// 벽(정적 지형)에 가려지면 보이지 않는다.
		const FVector TraceStart(Source.Location.X, Source.Location.Y, SightTraceHeight);
		if (!GetWorld()->LineTraceTestByObjectType(TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			return true;
		}
	}

	return false;
}

void UMobaVisionSubsystem::SetActorShown(AActor* Actor, bool bShown) const
{
	if (USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr)
	{
		Root->SetVisibility(bShown, true);
	}
}

void UMobaVisionSubsystem::UpdateVision(EMobaTeam LocalTeam)
{
	UWorld* World = GetWorld();
	TSet<TWeakObjectPtr<AActor>> NewHidden;

	if (LocalTeam == EMobaTeam::Blue || LocalTeam == EMobaTeam::Red)
	{
		// 시야를 주는 아군과, 숨길 수 있는 적을 한 번에 모은다.
		TArray<FVisionSource> Sources;
		TArray<AActor*> Targets;

		const auto AddUnit = [&](AActor* Unit, EMobaTeam UnitTeam, float SightRadius)
		{
			if (UnitTeam == LocalTeam)
			{
				const FVector Location = Unit->GetActorLocation();
				Sources.Add({ Location, SightRadius, GetBushMask(Location) });
			}
			else
			{
				Targets.Add(Unit);
			}
		};

		for (TActorIterator<AMobaCharacter> It(World); It; ++It)
		{
			if (!It->IsDead())
			{
				AddUnit(*It, It->GetTeam(), ChampionSightRadius);
			}
		}

		for (TActorIterator<AMobaMinion> It(World); It; ++It)
		{
			if (!MobaCombat::IsDead(*It))
			{
				AddUnit(*It, It->GetTeam(), MinionSightRadius);
			}
		}

		for (TActorIterator<AMobaMonster> It(World); It; ++It)
		{
			if (!MobaCombat::IsDead(*It))
			{
				Targets.Add(*It);
			}
		}

		// 건물은 시야만 주고 숨기지는 않는다.
		for (TActorIterator<AMobaStructure> It(World); It; ++It)
		{
			if (It->GetTeam() == LocalTeam && !It->IsDestroyed())
			{
				Sources.Add({ It->GetActorLocation(), StructureSightRadius, 0 });
			}
		}

		for (TActorIterator<AMobaWard> It(World); It; ++It)
		{
			if (It->GetTeam() == LocalTeam)
			{
				const FVector Location = It->GetActorLocation();
				Sources.Add({ Location, WardSightRadius, GetBushMask(Location) });
			}
			else
			{
				// 적의 와드는 언제나 보이지 않는다.
				NewHidden.Add(*It);
			}
		}

		for (AActor* Target : Targets)
		{
			if (!CanAnySourceSee(Sources, Target->GetActorLocation()))
			{
				NewHidden.Add(Target);
			}
		}
	}

	// 바뀐 것만 반영한다.
	for (const TWeakObjectPtr<AActor>& Actor : HiddenActors)
	{
		if (Actor.IsValid() && !NewHidden.Contains(Actor))
		{
			SetActorShown(Actor.Get(), true);
		}
	}

	for (const TWeakObjectPtr<AActor>& Actor : NewHidden)
	{
		if (!HiddenActors.Contains(Actor))
		{
			SetActorShown(Actor.Get(), false);
		}
	}

	HiddenActors = MoveTemp(NewHidden);
}
