#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MobaTypes.generated.h"

UENUM(BlueprintType)
enum class EMobaTeam : uint8
{
	None,
	Blue,
	Red
};

UENUM(BlueprintType)
enum class EMobaLane : uint8
{
	Top,
	Mid,
	Bot
};

UENUM(BlueprintType)
enum class EMobaAbilitySlot : uint8
{
	Q,
	W,
	E,
	R
};

/** 스킬을 쓸 때 무엇을 지정해야 하는지. */
UENUM(BlueprintType)
enum class EMobaTargetingMode : uint8
{
	/** 지정 없이 즉시 발동. */
	None,
	/** 커서 방향으로 발사. */
	Direction,
	/** 커서가 가리키는 지면 위치. */
	Location
};

namespace MobaTeam
{
	/** PlayerStart 등 레벨 액터에서 팀을 구분할 때 쓰는 태그 이름 ("Blue" / "Red"). */
	VER1_API FName GetTeamTag(EMobaTeam Team);

	VER1_API FLinearColor GetTeamColor(EMobaTeam Team);
}

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UMobaTeamInterface : public UInterface
{
	GENERATED_BODY()
};

/** 팀에 속하는 모든 것(챔피언, 미니언, 건물)이 구현한다. */
class VER1_API IMobaTeamInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Team")
	virtual EMobaTeam GetTeam() const = 0;
};
