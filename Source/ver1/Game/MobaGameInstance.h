#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Game/MobaTypes.h"
#include "MobaGameInstance.generated.h"

class UNetDriver;

/** 접속 실패나 끊김처럼 메뉴로 돌아간 이유를 맵 이동 사이에 들고 있는다. */
UCLASS()
class VER1_API UMobaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** 메뉴에 한 번 보여 줄 안내 문구를 꺼내고 비운다. */
	FText ConsumePendingMessage();

	/** 메뉴에서 고른 챔피언의 에셋 이름. 고르지 않았으면 비어 있다. 바꾸면 저장한다. */
	const FString& GetSelectedChampion() const { return SelectedChampion; }
	void SetSelectedChampion(const FString& InName);

	/** 고른 소환사 주문. Index 0이 D, 1이 F. */
	EMobaSummonerSpell GetSelectedSpell(int32 Index) const { return Index == 0 ? SelectedSpellD : SelectedSpellF; }

	/** 바꾸면 저장한다. 다른 칸에 같은 주문이 있으면 두 칸을 맞바꾼다. */
	void SetSelectedSpell(int32 Index, EMobaSummonerSpell Spell);

	/** 방을 만들 때의 인원(2면 1대1, 4면 2대2). 바꾸면 저장한다. */
	int32 GetHostPlayers() const { return HostPlayers; }
	void SetHostPlayers(int32 Players);

	/** 조작 설정. 바꾸면 저장한다. */
	bool IsSmartCastEnabled() const { return bSmartCast; }
	void SetSmartCastEnabled(bool bEnabled);
	bool IsCameraLockDefault() const { return bCameraLockDefault; }
	void SetCameraLockDefault(bool bLocked);
	int32 GetCameraSpeedIndex() const { return CameraSpeedIndex; }
	void SetCameraSpeedIndex(int32 Index);

	/** 카메라 가장자리 이동 속도 단계 수와 단계별 속도·이름. */
	static constexpr int32 NumCameraSpeeds = 3;
	static float GetCameraPanSpeed(int32 Index);
	static FString GetCameraSpeedName(int32 Index);

	/** 접속 주소 뒤에 붙일 "?Champion=..?Spells=.." 옵션. */
	FString MakeLoadoutOptions() const;

protected:
	void RecordNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void RecordTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	void LoadProfile();
	void SaveProfile() const;

	FText PendingMessage;
	FString SelectedChampion;
	EMobaSummonerSpell SelectedSpellD = EMobaSummonerSpell::Flash;
	EMobaSummonerSpell SelectedSpellF = EMobaSummonerSpell::Ignite;
	int32 HostPlayers = 2;
	bool bSmartCast = true;
	bool bCameraLockDefault = true;
	int32 CameraSpeedIndex = 1;

	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
};
