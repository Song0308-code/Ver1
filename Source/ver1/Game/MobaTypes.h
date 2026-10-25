#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MobaTypes.generated.h"

UENUM(BlueprintType)
enum class EMobaTeam : uint8
{
	None,
	Blue,
	Red,

	/** 정글 몬스터. 양 팀 챔피언과만 싸운다. */
	Neutral
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
	R,
	/** 소환사 주문 두 칸. */
	D,
	F
};

/** 핑 종류. Alt+좌클릭 일반, Ctrl+Alt 위험, Shift+Alt 가는 중. */
UENUM()
enum class EMobaPingType : uint8
{
	Normal,
	Danger,
	OnMyWay
};

/** 드래곤의 원소. 나올 때마다 정해지고 처치 보상이 달라진다. */
UENUM()
enum class EMobaDragonElement : uint8
{
	Infernal,
	Mountain,
	Cloud,
	Ocean,
	Count UMETA(Hidden)
};

/** 결과 화면에 세는 팀 오브젝트. */
UENUM()
enum class EMobaObjective : uint8
{
	Tower,
	Dragon,
	Baron,
	Herald,
	Count UMETA(Hidden)
};

/** 한 팀이 잡거나 부순 오브젝트 수. */
USTRUCT()
struct FMobaTeamObjectives
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Towers = 0;

	UPROPERTY()
	int32 Dragons = 0;

	UPROPERTY()
	int32 Barons = 0;

	UPROPERTY()
	int32 Heralds = 0;

	int32& Get(EMobaObjective Objective)
	{
		switch (Objective)
		{
		case EMobaObjective::Dragon: return Dragons;
		case EMobaObjective::Baron: return Barons;
		case EMobaObjective::Herald: return Heralds;
		default: return Towers;
		}
	}

	int32 Get(EMobaObjective Objective) const { return const_cast<FMobaTeamObjectives*>(this)->Get(Objective); }
};

namespace MobaScore
{
	/** 직전 처치에서 Window초 안이면 멀티킬 수를 이어 가고, 아니면 1부터 다시 센다. 최대 5(펜타킬). */
	inline int32 NextMultiKillCount(int32 Previous, double LastKillTime, double Now, double Window)
	{
		return Now - LastKillTime <= Window ? FMath::Min(Previous + 1, 5) : 1;
	}

	/** "더블킬" 같은 이름. 1 이하이면 빈 문자열. */
	inline FString GetMultiKillName(int32 Count)
	{
		switch (Count)
		{
		case 2: return TEXT("더블킬");
		case 3: return TEXT("트리플킬");
		case 4: return TEXT("쿼드라킬");
		case 5: return TEXT("펜타킬");
		default: return FString();
		}
	}
}

namespace MobaDragon
{
	/** "화염" 같은 원소 이름. */
	inline const TCHAR* GetElementName(EMobaDragonElement Element)
	{
		switch (Element)
		{
		case EMobaDragonElement::Mountain: return TEXT("대지");
		case EMobaDragonElement::Cloud: return TEXT("바람");
		case EMobaDragonElement::Ocean: return TEXT("바다");
		default: return TEXT("화염");
		}
	}
}

/** 개발 빌드의 콘솔 명령(Moba*)이 서버에 요청하는 일. */
UENUM()
enum class EMobaDevCommand : uint8
{
	Gold,
	Level,
	SkipTime,
	Refresh,
	SpawnCamps
};

/** 로비에서 고르는 소환사 주문. */
UENUM(BlueprintType)
enum class EMobaSummonerSpell : uint8
{
	Flash,
	Heal,
	Ignite,
	Ghost,
	Smite,
	Barrier,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMobaStructureType : uint8
{
	Tower,
	Inhibitor,
	Nexus
};

UENUM(BlueprintType)
enum class EMobaMinionType : uint8
{
	Melee,
	Caster,

	/** 적 억제기가 파괴된 라인에 나오는 강한 미니언. */
	Super,

	/** 전령의 눈으로 소환한 협곡의 전령. 라인을 따라가며 건물만 공격한다. */
	Herald,

	/** 대포 미니언. 몇 웨이브마다 한 마리씩 근접과 원거리 사이에 나온다. */
	Siege
};

UENUM(BlueprintType)
enum class EMobaCampType : uint8
{
	BlueBuff,
	RedBuff,
	Gromp,
	Wolves,
	Raptors,
	Krugs,
	Scuttle,
	Dragon,
	Baron,

	/** 협곡의 전령. 정해진 시각까지만 나온다. */
	Herald
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

	/** 화면에 보여 줄 팀 이름 ("블루" / "레드"). */
	VER1_API FString GetTeamDisplayName(EMobaTeam Team);
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
