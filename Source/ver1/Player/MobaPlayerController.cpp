#include "Player/MobaPlayerController.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NavigationSystem.h"
#include "Player/MobaCameraRig.h"

AMobaPlayerController::AMobaPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	// 시점은 폰이 아니라 로컬 카메라 리그가 담당한다.
	bAutoManageActiveCameraTarget = false;

	CameraRigClass = AMobaCameraRig::StaticClass();
}

void AMobaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && CameraRigClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CameraRig = GetWorld()->SpawnActor<AMobaCameraRig>(CameraRigClass, FTransform::Identity, SpawnParams);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 입력 구성

void AMobaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	CreateDefaultInput();

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnMoveCommandStarted);
		EnhancedInput->BindAction(MoveCommandAction, ETriggerEvent::Triggered, this, &AMobaPlayerController::OnMoveCommandHeld);
		EnhancedInput->BindAction(CameraLockAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnCameraLockToggled);
		EnhancedInput->BindAction(CameraCenterAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnCameraCenterStarted);
		EnhancedInput->BindAction(CameraCenterAction, ETriggerEvent::Completed, this, &AMobaPlayerController::OnCameraCenterCompleted);
		EnhancedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AMobaPlayerController::OnCameraZoom);
		EnhancedInput->BindAction(AbilityQAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAbilityQ);
		EnhancedInput->BindAction(AbilityWAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAbilityW);
		EnhancedInput->BindAction(AbilityEAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAbilityE);
		EnhancedInput->BindAction(AbilityRAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAbilityR);
		EnhancedInput->BindAction(AttackModeAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAttackModeStarted);
		EnhancedInput->BindAction(ConfirmTargetAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnConfirmTarget);
	}
}

void AMobaPlayerController::CreateDefaultInput()
{
	if (DefaultMappingContext)
	{
		return;
	}

	MoveCommandAction = NewObject<UInputAction>(this, TEXT("IA_MoveCommand"));
	CameraLockAction = NewObject<UInputAction>(this, TEXT("IA_CameraLock"));
	CameraCenterAction = NewObject<UInputAction>(this, TEXT("IA_CameraCenter"));
	CameraZoomAction = NewObject<UInputAction>(this, TEXT("IA_CameraZoom"));
	CameraZoomAction->ValueType = EInputActionValueType::Axis1D;
	AbilityQAction = NewObject<UInputAction>(this, TEXT("IA_AbilityQ"));
	AbilityWAction = NewObject<UInputAction>(this, TEXT("IA_AbilityW"));
	AbilityEAction = NewObject<UInputAction>(this, TEXT("IA_AbilityE"));
	AbilityRAction = NewObject<UInputAction>(this, TEXT("IA_AbilityR"));
	AttackModeAction = NewObject<UInputAction>(this, TEXT("IA_AttackMode"));
	ConfirmTargetAction = NewObject<UInputAction>(this, TEXT("IA_ConfirmTarget"));

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));
	DefaultMappingContext->MapKey(MoveCommandAction, EKeys::RightMouseButton);
	DefaultMappingContext->MapKey(CameraLockAction, EKeys::Y);
	DefaultMappingContext->MapKey(CameraCenterAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(CameraZoomAction, EKeys::MouseWheelAxis);

	// Ctrl과 함께 누르면 같은 키가 스킬 단계 올리기로 쓰인다(HandleAbilityKey 참고).
	DefaultMappingContext->MapKey(AbilityQAction, EKeys::Q);
	DefaultMappingContext->MapKey(AbilityWAction, EKeys::W);
	DefaultMappingContext->MapKey(AbilityEAction, EKeys::E);
	DefaultMappingContext->MapKey(AbilityRAction, EKeys::R);

	DefaultMappingContext->MapKey(AttackModeAction, EKeys::A);
	DefaultMappingContext->MapKey(ConfirmTargetAction, EKeys::LeftMouseButton);
}

// ---------------------------------------------------------------------------------------------------------------------
// 틱

void AMobaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateCamera(DeltaTime);

#if WITH_EDITORONLY_DATA
	if (!IsLocalController())
	{
		return;
	}

	if (bDebugIssueMove)
	{
		bDebugIssueMove = false;
		IssueMoveTo(DebugMoveGoal);
	}

	if (bDebugIssueAttackMove)
	{
		bDebugIssueAttackMove = false;
		IssueAttackMoveTo(DebugMoveGoal);
	}

	if (bDebugIssueAttack)
	{
		bDebugIssueAttack = false;
		IssueAttack(DebugAttackTarget);
	}

	if (bDebugLevelUp)
	{
		bDebugLevelUp = false;
		if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
		{
			MobaCharacter->LevelUpAbility(DebugCastSlot);
		}
	}

	if (bDebugIssueCast)
	{
		bDebugIssueCast = false;
		if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
		{
			MobaCharacter->CastAbility(DebugCastSlot, DebugCastLocation);
		}
	}
#endif
}

void AMobaPlayerController::UpdateCamera(float DeltaTime)
{
	if (!CameraRig)
	{
		return;
	}

	if (GetViewTarget() != CameraRig)
	{
		SetViewTarget(CameraRig);
	}

	const APawn* ControlledPawn = GetPawn();
	if ((bCameraLocked || bCameraCenterHeld) && ControlledPawn)
	{
		CameraRig->SetFocusLocation(ControlledPawn->GetActorLocation());
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0 || !GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	FVector2D ScrollDirection = FVector2D::ZeroVector;
	if (MouseX <= EdgeScrollMargin)
	{
		ScrollDirection.X = -1.0;
	}
	else if (MouseX >= ViewportX - EdgeScrollMargin)
	{
		ScrollDirection.X = 1.0;
	}

	// 화면 좌표는 아래로 갈수록 Y가 커진다.
	if (MouseY <= EdgeScrollMargin)
	{
		ScrollDirection.Y = 1.0;
	}
	else if (MouseY >= ViewportY - EdgeScrollMargin)
	{
		ScrollDirection.Y = -1.0;
	}

	if (!ScrollDirection.IsZero())
	{
		CameraRig->Pan(ScrollDirection.GetSafeNormal(), DeltaTime);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 커서

bool AMobaPlayerController::GetCursorGroundLocation(FVector& OutLocation) const
{
	FHitResult GroundHit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, GroundHit))
	{
		OutLocation = GroundHit.Location;
		return true;
	}
	return false;
}

AActor* AMobaPlayerController::FindEnemyAtCursor(FVector& OutCursorLocation) const
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter)
	{
		return nullptr;
	}

	// 챔피언·미니언(폰), 건물(동적 오브젝트), 지형(정적 오브젝트) 중 커서에 가장 먼저 닿는 것을 본다.
	static const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic),
		UEngineTypes::ConvertToObjectType(ECC_WorldStatic)
	};

	FHitResult Hit;
	if (!GetHitResultUnderCursorForObjects(ObjectTypes, false, Hit))
	{
		return nullptr;
	}

	OutCursorLocation = Hit.Location;

	if (MobaCombat::IsValidEnemyTarget(MobaCharacter, Hit.GetActor()))
	{
		return Hit.GetActor();
	}

	// 정확히 겹치지 않았더라도 커서 근처의 적 중 가장 가까운 것을 잡아 준다.
	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(MobaCharacter, Hit.Location, ClickSelectMargin + 400.0f, Enemies, true);

	AActor* Nearest = nullptr;
	float NearestDistance = ClickSelectMargin;
	for (AActor* Enemy : Enemies)
	{
		const float Distance = FMath::Max(FVector::Dist2D(Hit.Location, Enemy->GetActorLocation()) - Enemy->GetSimpleCollisionRadius(), 0.0f);
		if (Distance <= NearestDistance)
		{
			Nearest = Enemy;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

bool AMobaPlayerController::ProjectToNavigation(const FVector& WorldLocation, FVector& OutLocation) const
{
	// 경로 탐색은 조종하는 클라이언트에서 수행한다. 프로젝트 설정의 클라이언트 측 내비게이션이 켜져 있어야 한다.
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("이동 명령 실패: 내비게이션 시스템이 없습니다 (bAllowClientSideNavigation 확인)."));
		return false;
	}

	FNavLocation NavLocation;
	if (NavSystem->ProjectPointToNavigation(WorldLocation, NavLocation, NavProjectExtent))
	{
		OutLocation = NavLocation.Location;
		return true;
	}
	return false;
}

bool AMobaPlayerController::IsControlKeyDown() const
{
	return IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
}

// ---------------------------------------------------------------------------------------------------------------------
// 이동·공격 명령

void AMobaPlayerController::OnMoveCommandStarted()
{
	// 조준 중 우클릭은 조준을 취소하고 평소대로 이동·공격 명령을 내린다.
	bTargeting = false;
	bAttackTargeting = false;

	LastHeldMoveTime = GetWorld()->GetTimeSeconds();
	IssueCommandAtCursor();
}

void AMobaPlayerController::OnMoveCommandHeld()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastHeldMoveTime >= HeldMoveInterval)
	{
		LastHeldMoveTime = Now;
		IssueCommandAtCursor();
	}
}

void AMobaPlayerController::IssueCommandAtCursor()
{
	FVector CursorLocation;
	if (AActor* Enemy = FindEnemyAtCursor(CursorLocation))
	{
		IssueAttack(Enemy);
	}
	else if (GetCursorGroundLocation(CursorLocation))
	{
		IssueMoveTo(CursorLocation);
	}
}

void AMobaPlayerController::IssueAttackCommandAtCursor()
{
	FVector CursorLocation;
	if (AActor* Enemy = FindEnemyAtCursor(CursorLocation))
	{
		IssueAttack(Enemy);
	}
	else if (GetCursorGroundLocation(CursorLocation))
	{
		IssueAttackMoveTo(CursorLocation);
	}
}

void AMobaPlayerController::IssueAttack(AActor* Target)
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (MobaCharacter && MobaCharacter->CanAct())
	{
		MobaCharacter->GetCombatComponent()->SetAttackTarget(Target);
	}
}

void AMobaPlayerController::IssueMoveTo(const FVector& WorldLocation)
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter || !MobaCharacter->CanAct())
	{
		return;
	}

	MobaCharacter->GetCombatComponent()->SetAttackTarget(nullptr);

	FVector NavLocation;
	if (ProjectToNavigation(WorldLocation, NavLocation))
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, NavLocation);
	}
}

void AMobaPlayerController::IssueAttackMoveTo(const FVector& WorldLocation)
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter || !MobaCharacter->CanAct())
	{
		return;
	}

	FVector NavLocation;
	if (ProjectToNavigation(WorldLocation, NavLocation))
	{
		MobaCharacter->GetCombatComponent()->StartAttackMove(NavLocation);
	}
}

void AMobaPlayerController::OnAttackModeStarted()
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (MobaCharacter && MobaCharacter->CanAct())
	{
		bTargeting = false;
		bAttackTargeting = true;
	}
}

void AMobaPlayerController::OnConfirmTarget()
{
	if (bTargeting)
	{
		bTargeting = false;
		CastAbilityAtCursor(TargetingSlot);
	}
	else if (bAttackTargeting)
	{
		bAttackTargeting = false;
		IssueAttackCommandAtCursor();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 스킬

bool AMobaPlayerController::GetTargetingSlot(EMobaAbilitySlot& OutSlot) const
{
	OutSlot = TargetingSlot;
	return bTargeting;
}

void AMobaPlayerController::OnAbilityQ()
{
	HandleAbilityKey(EMobaAbilitySlot::Q);
}

void AMobaPlayerController::OnAbilityW()
{
	HandleAbilityKey(EMobaAbilitySlot::W);
}

void AMobaPlayerController::OnAbilityE()
{
	HandleAbilityKey(EMobaAbilitySlot::E);
}

void AMobaPlayerController::OnAbilityR()
{
	HandleAbilityKey(EMobaAbilitySlot::R);
}

void AMobaPlayerController::HandleAbilityKey(EMobaAbilitySlot Slot)
{
	AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter)
	{
		return;
	}

	// Ctrl + 스킬 키는 스킬 포인트로 그 스킬의 단계를 올린다. 죽어 있거나 기절 중이어도 올릴 수 있다.
	if (IsControlKeyDown())
	{
		MobaCharacter->LevelUpAbility(Slot);
		return;
	}

	const UMobaGameplayAbility* Ability = MobaCharacter->GetAbilityInSlot(Slot);
	if (!Ability || !MobaCharacter->CanAct())
	{
		return;
	}

	bAttackTargeting = false;

	if (Ability->GetTargetingMode() == EMobaTargetingMode::None)
	{
		bTargeting = false;
		MobaCharacter->CastAbility(Slot, MobaCharacter->GetActorLocation());
	}
	else if (bSmartCast)
	{
		CastAbilityAtCursor(Slot);
	}
	else
	{
		bTargeting = true;
		TargetingSlot = Slot;
	}
}

void AMobaPlayerController::CastAbilityAtCursor(EMobaAbilitySlot Slot)
{
	AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	FVector GroundLocation;
	if (MobaCharacter && GetCursorGroundLocation(GroundLocation))
	{
		MobaCharacter->CastAbility(Slot, GroundLocation);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 카메라

void AMobaPlayerController::OnCameraLockToggled()
{
	bCameraLocked = !bCameraLocked;
}

void AMobaPlayerController::OnCameraCenterStarted()
{
	bCameraCenterHeld = true;
}

void AMobaPlayerController::OnCameraCenterCompleted()
{
	bCameraCenterHeld = false;
}

void AMobaPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	if (CameraRig)
	{
		CameraRig->Zoom(Value.Get<float>());
	}
}
