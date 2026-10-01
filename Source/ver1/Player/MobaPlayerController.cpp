#include "Player/MobaPlayerController.h"
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

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));
	DefaultMappingContext->MapKey(MoveCommandAction, EKeys::RightMouseButton);
	DefaultMappingContext->MapKey(CameraLockAction, EKeys::Y);
	DefaultMappingContext->MapKey(CameraCenterAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(CameraZoomAction, EKeys::MouseWheelAxis);
}

void AMobaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateCamera(DeltaTime);

#if WITH_EDITORONLY_DATA
	if (bDebugIssueMove && IsLocalController())
	{
		bDebugIssueMove = false;
		IssueMoveTo(DebugMoveGoal);
	}

	if (bDebugIssueAttack && IsLocalController())
	{
		bDebugIssueAttack = false;
		IssueAttack(DebugAttackTarget);
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

void AMobaPlayerController::OnMoveCommandStarted()
{
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
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter || MobaCharacter->IsDead())
	{
		return;
	}

	static const TArray<TEnumAsByte<EObjectTypeQuery>> PawnObjectTypes = { UEngineTypes::ConvertToObjectType(ECC_Pawn) };

	FHitResult PawnHit;
	if (GetHitResultUnderCursorForObjects(PawnObjectTypes, false, PawnHit) && MobaCombat::IsValidEnemyTarget(MobaCharacter, PawnHit.GetActor()))
	{
		IssueAttack(PawnHit.GetActor());
		return;
	}

	FHitResult GroundHit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, GroundHit))
	{
		IssueMoveTo(GroundHit.Location);
	}
}

void AMobaPlayerController::IssueAttack(AActor* Target)
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (MobaCharacter && !MobaCharacter->IsDead())
	{
		MobaCharacter->GetCombatComponent()->SetAttackTarget(Target);
	}
}

void AMobaPlayerController::IssueMoveTo(const FVector& WorldLocation)
{
	const AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	if (!MobaCharacter || MobaCharacter->IsDead())
	{
		return;
	}

	MobaCharacter->GetCombatComponent()->SetAttackTarget(nullptr);

	// 경로 탐색은 조종하는 클라이언트에서 수행한다. 프로젝트 설정의 클라이언트 측 내비게이션이 켜져 있어야 한다.
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("이동 명령 실패: 내비게이션 시스템이 없습니다 (bAllowClientSideNavigation 확인)."));
		return;
	}

	FNavLocation NavLocation;
	if (NavSystem->ProjectPointToNavigation(WorldLocation, NavLocation, NavProjectExtent))
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, NavLocation.Location);
	}
}

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
