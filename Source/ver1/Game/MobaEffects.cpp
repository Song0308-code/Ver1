#include "Game/MobaEffects.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Vision/MobaVisionSubsystem.h"

UMobaEffectSettings::UMobaEffectSettings()
{
	CategoryName = TEXT("Game");

	const auto System = [](const TCHAR* Name)
	{
		return TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(FString::Printf(TEXT("/Game/Effects/%s.%s"), Name, Name)));
	};

	Hit.System = System(TEXT("NS_Moba_Hit"));
	Hit.Scale = 0.25f;

	UnitDeath.System = System(TEXT("NS_Moba_Death"));
	UnitDeath.Scale = 0.5f;

	StructureDeath.System = System(TEXT("NS_Moba_Death"));
	StructureDeath.Scale = 2.0f;

	Cast.System = System(TEXT("NS_Moba_Cast"));
	Cast.Scale = 0.5f;

	AreaBlast.System = System(TEXT("NS_Moba_Death"));
	AreaBlast.Scale = 1.0f;

	LevelUp.System = System(TEXT("NS_Moba_LevelUp"));
	LevelUp.Scale = 0.6f;
}

const FMobaEffectEntry* UMobaEffectSettings::GetEntry(EMobaEffect Effect) const
{
	switch (Effect)
	{
	case EMobaEffect::Hit: return &Hit;
	case EMobaEffect::UnitDeath: return &UnitDeath;
	case EMobaEffect::StructureDeath: return &StructureDeath;
	case EMobaEffect::Cast: return &Cast;
	case EMobaEffect::AreaBlast: return &AreaBlast;
	case EMobaEffect::LevelUp: return &LevelUp;
	default: return nullptr;
	}
}

void MobaEffects::Play(const UObject* WorldContextObject, EMobaEffect Effect, const FVector& Location, float ScaleMultiplier, const AActor* RelatedActor)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (RelatedActor)
	{
		const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(World);
		if (Vision && !Vision->IsVisibleToLocalPlayer(RelatedActor))
		{
			return;
		}
	}

	const FMobaEffectEntry* Entry = GetDefault<UMobaEffectSettings>()->GetEntry(Effect);
	if (!Entry)
	{
		return;
	}

	if (UNiagaraSystem* System = Entry->System.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location, FRotator::ZeroRotator, FVector(Entry->Scale * ScaleMultiplier));
	}

	if (USoundBase* Sound = Entry->Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(World, Sound, Location);
	}
}
