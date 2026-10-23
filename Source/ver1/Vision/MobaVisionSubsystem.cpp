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

bool UMobaVisionSubsystem::IsFogVertexVisible(int32 X, int32 Y) const
{
	const int32 Count = FogCells + 1;
	if (X < 0 || Y < 0 || X >= Count || Y >= Count || FogVisible.Num() != Count * Count)
	{
		return false;
	}
	return FogVisible[Y * Count + X] != 0;
}

void UMobaVisionSubsystem::RegisterBush(AMobaBush* Bush)
{
	// 비트 하나에 부쉬 하나를 대응시키므로 64개까지만 다룬다.
	if (Bush && Bushes.Num() < 64)
	{
		Bushes.AddUnique(Bush);
		bFogBushMasksDirty = true;
	}
}

void UMobaVisionSubsystem::UnregisterBush(AMobaBush* Bush)
{
	Bushes.Remove(Bush);
	bFogBushMasksDirty = true;
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

	// 서버는 팀마다 볼 수 없는 유닛을 계산해 두고, 그 팀의 PC에는 그 유닛을 보내지 않는다.
	if (World->GetNetMode() != NM_Client)
	{
		ComputeHiddenForTeam(EMobaTeam::Blue, ServerHidden[0], nullptr);
		ComputeHiddenForTeam(EMobaTeam::Red, ServerHidden[1], nullptr);
	}

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

void UMobaVisionSubsystem::UpdateFog(const TArray<FVisionSource>& Sources)
{
	const int32 Count = FogCells + 1;
	const float CellSize = FogWorldSize / FogCells;

	if (bFogBushMasksDirty || FogBushMasks.Num() != Count * Count)
	{
		FogBushMasks.SetNumUninitialized(Count * Count);
		for (int32 Y = 0; Y < Count; ++Y)
		{
			for (int32 X = 0; X < Count; ++X)
			{
				FogBushMasks[Y * Count + X] = GetBushMask(FVector(X * CellSize, Y * CellSize, 0.0));
			}
		}
		bFogBushMasksDirty = false;
	}

	FogVisible.SetNumZeroed(Count * Count);
	FMemory::Memzero(FogVisible.GetData(), FogVisible.Num());

	// 시야 원마다 그 원을 덮는 꼭짓점만 살펴본다.
	for (const FVisionSource& Source : Sources)
	{
		const int32 MinX = FMath::Clamp(FMath::FloorToInt((Source.Location.X - Source.Radius) / CellSize), 0, Count - 1);
		const int32 MaxX = FMath::Clamp(FMath::CeilToInt((Source.Location.X + Source.Radius) / CellSize), 0, Count - 1);
		const int32 MinY = FMath::Clamp(FMath::FloorToInt((Source.Location.Y - Source.Radius) / CellSize), 0, Count - 1);
		const int32 MaxY = FMath::Clamp(FMath::CeilToInt((Source.Location.Y + Source.Radius) / CellSize), 0, Count - 1);
		const float RadiusSquared = FMath::Square(Source.Radius);

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const int32 Index = Y * Count + X;
				if (FogVisible[Index])
				{
					continue;
				}

				const float DistanceSquared = FMath::Square(X * CellSize - Source.Location.X) + FMath::Square(Y * CellSize - Source.Location.Y);
				const uint64 BushMask = FogBushMasks[Index];
				if (DistanceSquared <= RadiusSquared && (BushMask == 0 || (BushMask & Source.BushMask) != 0))
				{
					FogVisible[Index] = 1;
				}
			}
		}
	}

	bFogActive = true;
}

bool UMobaVisionSubsystem::IsHiddenFromTeam(const AActor* Actor, EMobaTeam Team) const
{
	const int32 Index = Team == EMobaTeam::Blue ? 0 : (Team == EMobaTeam::Red ? 1 : INDEX_NONE);
	return Index != INDEX_NONE && ServerHidden[Index].Contains(const_cast<AActor*>(Actor));
}

bool UMobaVisionSubsystem::IsRelevantToViewer(const AActor* Actor, const AActor* Viewer)
{
	const APlayerController* ViewerController = Cast<APlayerController>(Viewer);
	const AMobaPlayerState* ViewerState = ViewerController ? ViewerController->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!ViewerState)
	{
		return true;
	}

	const EMobaTeam ViewerTeam = ViewerState->GetTeam();
	if (ViewerTeam == EMobaTeam::None || MobaCombat::GetTeam(Actor) == ViewerTeam)
	{
		return true;
	}

	const UMobaVisionSubsystem* Vision = Get(Actor);
	return !Vision || !Vision->IsHiddenFromTeam(Actor, ViewerTeam);
}

void UMobaVisionSubsystem::UpdateVision(EMobaTeam LocalTeam)
{
	TSet<TWeakObjectPtr<AActor>> NewHidden;

	if (LocalTeam == EMobaTeam::Blue || LocalTeam == EMobaTeam::Red)
	{
		TArray<FVisionSource> Sources;
		ComputeHiddenForTeam(LocalTeam, NewHidden, &Sources);
		UpdateFog(Sources);
	}
	else
	{
		bFogActive = false;
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

void UMobaVisionSubsystem::ComputeHiddenForTeam(EMobaTeam LocalTeam, TSet<TWeakObjectPtr<AActor>>& NewHidden, TArray<FVisionSource>* OutSources) const
{
	UWorld* World = GetWorld();
	NewHidden.Reset();

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

		// 제어 와드 위치를 팀별로 모은다. 상대 제어 와드 근처의 일반 와드는 시야가 꺼지고, 내 제어 와드 근처의 적 일반 와드는 드러난다.
		TArray<FVector> MyControlWards;
		TArray<FVector> EnemyControlWards;
		for (TActorIterator<AMobaWard> It(World); It; ++It)
		{
			if (It->IsControlWard() && !MobaCombat::IsDead(*It))
			{
				(It->GetTeam() == LocalTeam ? MyControlWards : EnemyControlWards).Add(It->GetActorLocation());
			}
		}
		const auto IsNear = [](const TArray<FVector>& Points, const FVector& Location)
		{
			for (const FVector& Point : Points)
			{
				if (FVector::DistSquared2D(Point, Location) <= FMath::Square(AMobaWard::RevealRadius))
				{
					return true;
				}
			}
			return false;
		};

		for (TActorIterator<AMobaWard> It(World); It; ++It)
		{
			const FVector Location = It->GetActorLocation();
			if (It->GetTeam() == LocalTeam)
			{
				if (It->IsControlWard() || !IsNear(EnemyControlWards, Location))
				{
					Sources.Add({ Location, WardSightRadius, GetBushMask(Location) });
				}
			}
			else if (It->IsControlWard() || IsNear(MyControlWards, Location))
			{
				// 제어 와드와, 내 제어 와드에 드러난 일반 와드는 시야 안에 있으면 보인다.
				Targets.Add(*It);
			}
			else
			{
				// 그 밖의 적 와드는 보이지 않는다.
				NewHidden.Add(*It);
			}
		}

		for (AActor* Target : Targets)
		{
			// 은신한 적은 시야 안이어도 보이지 않는다(내 제어 와드 근처 제외).
			if (MobaCombat::IsStealthedFrom(Target, LocalTeam) || !CanAnySourceSee(Sources, Target->GetActorLocation()))
			{
				NewHidden.Add(Target);
			}
		}

		if (OutSources)
		{
			*OutSources = MoveTemp(Sources);
		}
	}
}
