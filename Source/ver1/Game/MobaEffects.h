#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MobaEffects.generated.h"

class UNiagaraSystem;
class USoundBase;

/** 화면 효과의 종류. 게임 코드는 종류만 정하고, 실제로 무엇을 재생할지는 설정에서 고른다. */
UENUM(BlueprintType)
enum class EMobaEffect : uint8
{
	/** 기본 공격·스킬샷 명중. */
	Hit,
	/** 챔피언·미니언·몬스터 사망. */
	UnitDeath,
	/** 건물 파괴. */
	StructureDeath,
	/** 스킬 시전. */
	Cast,
	/** 범위 스킬 폭발. */
	AreaBlast,
	/** 레벨업. */
	LevelUp
};

/** 효과 하나에 쓰는 이펙트와 소리. 둘 다 비어 있어도 된다. */
USTRUCT(BlueprintType)
struct FMobaEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> System;

	/** 이펙트 크기 배율. */
	UPROPERTY(Config, EditAnywhere, Category = "Effect")
	float Scale = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Effect")
	TSoftObjectPtr<USoundBase> Sound;
};

/**
 * 효과마다 재생할 나이아가라 이펙트와 소리. 프로젝트 설정의 "Moba Effects"에서 바꾸거나 DefaultGame.ini에 적는다.
 * 기본값은 엔진 나이아가라 템플릿으로 만든 임시 이펙트이고, 소리는 비어 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Moba Effects"))
class VER1_API UMobaEffectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMobaEffectSettings();

	const FMobaEffectEntry* GetEntry(EMobaEffect Effect) const;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry Hit;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry UnitDeath;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry StructureDeath;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry Cast;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry AreaBlast;

	UPROPERTY(Config, EditAnywhere, Category = "Effects")
	FMobaEffectEntry LevelUp;
};

namespace MobaEffects
{
	/**
	 * 이 PC에서만 이펙트와 소리를 재생한다(화면이 없는 서버에서는 아무것도 하지 않는다).
	 * RelatedActor가 시야 밖이라 숨겨져 있으면 재생하지 않는다.
	 */
	VER1_API void Play(const UObject* WorldContextObject, EMobaEffect Effect, const FVector& Location, float ScaleMultiplier = 1.0f, const AActor* RelatedActor = nullptr);
}
