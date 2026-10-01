#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Game/MobaTypes.h"
#include "MobaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/** 플레이어가 조종하는 챔피언의 기본 클래스. */
UCLASS()
class VER1_API AMobaCharacter : public ACharacter, public IMobaTeamInterface
{
	GENERATED_BODY()

public:
	AMobaCharacter();

	virtual EMobaTeam GetTeam() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

protected:
	/** PlayerState의 팀 변경을 구독하고 현재 팀을 외형에 반영한다. */
	void BindToPlayerStateTeam();

	void HandleTeamChanged(EMobaTeam NewTeam);

	/** 실제 메시가 들어오기 전까지 쓰는 임시 외형. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> TopDownCamera;
};
