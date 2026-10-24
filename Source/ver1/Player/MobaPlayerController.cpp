#include "Player/MobaPlayerController.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Game/MobaGameInstance.h"
#include "Game/MobaGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NavigationSystem.h"
#include "Player/MobaCameraRig.h"
#include "Player/MobaPlayerState.h"
#include "UI/MobaHUD.h"
#include "UI/SMobaChatInput.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Vision/MobaVisionSubsystem.h"

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

	if (IsLocalController())
	{
		// 메뉴가 UI 전용으로 바꿔 둔 입력 모드를 게임용으로 되돌린다.
		RestoreGameInputMode();
	}

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
		EnhancedInput->BindAction(SummonerDAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnSummonerD);
		EnhancedInput->BindAction(SummonerFAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnSummonerF);
		EnhancedInput->BindAction(AttackModeAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnAttackModeStarted);
		EnhancedInput->BindAction(ConfirmTargetAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnConfirmTarget);
		EnhancedInput->BindAction(ReturnToMenuAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnReturnToMenu);
		EnhancedInput->BindAction(PlaceWardAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnPlaceWard);
		EnhancedInput->BindAction(UsePotionAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnUsePotion);
		EnhancedInput->BindAction(PlaceControlWardAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnPlaceControlWard);
		EnhancedInput->BindAction(UseHeraldEyeAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnUseHeraldEye);
		EnhancedInput->BindAction(RecallAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnRecall);
		EnhancedInput->BindAction(ShopAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnToggleShop);
		EnhancedInput->BindAction(GameMenuAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnToggleGameMenu);
		EnhancedInput->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &AMobaPlayerController::OnScoreboardStarted);
		EnhancedInput->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &AMobaPlayerController::OnScoreboardCompleted);
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
	SummonerDAction = NewObject<UInputAction>(this, TEXT("IA_SummonerD"));
	SummonerFAction = NewObject<UInputAction>(this, TEXT("IA_SummonerF"));
	AttackModeAction = NewObject<UInputAction>(this, TEXT("IA_AttackMode"));
	ConfirmTargetAction = NewObject<UInputAction>(this, TEXT("IA_ConfirmTarget"));
	ReturnToMenuAction = NewObject<UInputAction>(this, TEXT("IA_ReturnToMenu"));
	PlaceWardAction = NewObject<UInputAction>(this, TEXT("IA_PlaceWard"));
	UsePotionAction = NewObject<UInputAction>(this, TEXT("IA_UsePotion"));
	PlaceControlWardAction = NewObject<UInputAction>(this, TEXT("IA_PlaceControlWard"));
	UseHeraldEyeAction = NewObject<UInputAction>(this, TEXT("IA_UseHeraldEye"));
	RecallAction = NewObject<UInputAction>(this, TEXT("IA_Recall"));
	ShopAction = NewObject<UInputAction>(this, TEXT("IA_Shop"));
	ScoreboardAction = NewObject<UInputAction>(this, TEXT("IA_Scoreboard"));
	GameMenuAction = NewObject<UInputAction>(this, TEXT("IA_GameMenu"));

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
	DefaultMappingContext->MapKey(SummonerDAction, EKeys::D);
	DefaultMappingContext->MapKey(SummonerFAction, EKeys::F);

	DefaultMappingContext->MapKey(AttackModeAction, EKeys::A);
	DefaultMappingContext->MapKey(ConfirmTargetAction, EKeys::LeftMouseButton);
	DefaultMappingContext->MapKey(ReturnToMenuAction, EKeys::Enter);
	DefaultMappingContext->MapKey(PlaceWardAction, EKeys::Four);
	DefaultMappingContext->MapKey(UsePotionAction, EKeys::One);
	DefaultMappingContext->MapKey(PlaceControlWardAction, EKeys::Three);
	DefaultMappingContext->MapKey(UseHeraldEyeAction, EKeys::Two);
	DefaultMappingContext->MapKey(RecallAction, EKeys::B);
	DefaultMappingContext->MapKey(ShopAction, EKeys::P);
	DefaultMappingContext->MapKey(ScoreboardAction, EKeys::Tab);
	DefaultMappingContext->MapKey(GameMenuAction, EKeys::F10);
}

void AMobaPlayerController::OnReturnToMenu()
{
	// 경기가 끝난 뒤에는 메뉴로 돌아간다. 호스트가 나가면 접속해 있던 쪽도 메뉴로 돌아간다.
	if (MobaCombat::IsMatchOver(this))
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			GameInstance->ReturnToMainMenu();
		}
		return;
	}

	// 경기 중에는 채팅 입력창을 연다.
	OpenChat();
}

void AMobaPlayerController::RestoreGameInputMode()
{
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);
}

// ---------------------------------------------------------------------------------------------------------------------
// 채팅

void AMobaPlayerController::OpenChat()
{
	if (ChatWidget.IsValid() || !IsLocalController() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// HUD(캔버스)는 실제 픽셀, Slate는 화면 배율이 적용된 단위라 능력치 패널(아래 88픽셀) 바로 위에 오도록 환산한다.
	FVector2D ViewportSize(1280.0, 720.0);
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const float DPIScale = FMath::Max(GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y)), 0.1f);

	TWeakObjectPtr<AMobaPlayerController> WeakThis(this);
	ChatWidget = SNew(SMobaChatInput)
		.BottomPadding(98.0f / DPIScale)
		.OnSubmit_Lambda([WeakThis](const FString& Text)
		{
			if (AMobaPlayerController* Controller = WeakThis.Get())
			{
				Controller->SendChat(Text);
				Controller->CloseChat();
			}
		})
		.OnCancel_Lambda([WeakThis]()
		{
			if (AMobaPlayerController* Controller = WeakThis.Get())
			{
				Controller->CloseChat();
			}
		});
	GEngine->GameViewport->AddViewportWidgetContent(ChatWidget.ToSharedRef(), 50);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ChatWidget->GetFocusTarget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMobaPlayerController::CloseChat()
{
	if (!ChatWidget.IsValid())
	{
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ChatWidget.ToSharedRef());
	}
	ChatWidget.Reset();
	RestoreGameInputMode();
}

void AMobaPlayerController::SendChat(const FString& Text)
{
	FString Message = Text.TrimStartAndEnd();
	bool bAll = false;
	if (Message.StartsWith(TEXT("/all"), ESearchCase::IgnoreCase))
	{
		bAll = true;
		Message = Message.RightChop(4).TrimStartAndEnd();
	}

	if (!Message.IsEmpty())
	{
		ServerChat(Message.Left(150), bAll);
	}
}

void AMobaPlayerController::ServerChat_Implementation(const FString& Text, bool bAll)
{
	const AMobaPlayerState* SenderState = GetPlayerState<AMobaPlayerState>();
	const FString Message = Text.TrimStartAndEnd().Left(150);
	if (!SenderState || Message.IsEmpty())
	{
		return;
	}

	const FString Sender = FString::Printf(TEXT("%s %s"), *MobaTeam::GetTeamDisplayName(SenderState->GetTeam()), *SenderState->GetChampionDisplayName());
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMobaPlayerController* Receiver = Cast<AMobaPlayerController>(It->Get());
		const AMobaPlayerState* ReceiverState = Receiver ? Receiver->GetPlayerState<AMobaPlayerState>() : nullptr;
		if (ReceiverState && (bAll || ReceiverState->GetTeam() == SenderState->GetTeam()))
		{
			Receiver->ClientReceiveChat(Sender, Message, bAll, SenderState->GetTeam());
		}
	}
}

void AMobaPlayerController::ClientReceiveChat_Implementation(const FString& Sender, const FString& Text, bool bAll, EMobaTeam SenderTeam)
{
	if (ChatLog.Num() >= 20)
	{
		ChatLog.RemoveAt(0);
	}
	ChatLog.Add({ Sender, Text, bAll, SenderTeam, GetWorld()->GetTimeSeconds() });
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

	// 접속한 쪽의 컨트롤러는 서버에서 틱이 돌지 않으므로 호스트의 컨트롤러가 모두에게 준다.
	if (HasAuthority() && DebugGrantExperience > 0.0f)
	{
		for (TActorIterator<AMobaPlayerState> It(GetWorld()); It; ++It)
		{
			It->AddExperience(DebugGrantExperience);
		}
		DebugGrantExperience = 0.0f;
	}

	if (HasAuthority() && DebugGrantGold > 0.0f)
	{
		for (TActorIterator<AMobaPlayerState> It(GetWorld()); It; ++It)
		{
			It->AddGold(DebugGrantGold);
		}
		DebugGrantGold = 0.0f;
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

	if (!DebugLobbyChampion.IsEmpty())
	{
		RequestChampion(DebugLobbyChampion);
		DebugLobbyChampion.Reset();
	}

	if (!DebugLobbySpells.IsEmpty())
	{
		TArray<FString> Names;
		DebugLobbySpells.ParseIntoArray(Names, TEXT(","));
		const UEnum* SpellEnum = StaticEnum<EMobaSummonerSpell>();
		for (int32 Index = 0; Index < FMath::Min(Names.Num(), 2); ++Index)
		{
			const int64 Value = SpellEnum->GetValueByNameString(Names[Index].TrimStartAndEnd());
			if (Value != INDEX_NONE)
			{
				RequestSummonerSpell(Index, static_cast<EMobaSummonerSpell>(Value));
			}
		}
		DebugLobbySpells.Reset();
	}

	if (bDebugToggleShop)
	{
		bDebugToggleShop = false;
		OnToggleShop();
	}

	if (bDebugToggleGameMenu)
	{
		bDebugToggleGameMenu = false;
		OnToggleGameMenu();
	}

	if (bDebugShowScoreboard)
	{
		bDebugShowScoreboard = false;
		bScoreboardHeld = !bScoreboardHeld;
	}

	if (bDebugLeftClick)
	{
		bDebugLeftClick = false;
		OnConfirmTarget();
	}

	if (bDebugRightClick)
	{
		bDebugRightClick = false;
		OnMoveCommandStarted();
	}

	if (bDebugPing)
	{
		bDebugPing = false;
		SendPingAtCursor();
	}

	if (bDebugSurrender)
	{
		bDebugSurrender = false;
		RequestSurrender();
	}

	if (bDebugToggleReady)
	{
		bDebugToggleReady = false;
		const AMobaPlayerState* MyState = GetPlayerState<AMobaPlayerState>();
		RequestReady(!(MyState && MyState->IsReady()));
	}

	if (bDebugBuyItem || bDebugSellItem || bDebugRecall)
	{
		if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
		{
			if (bDebugBuyItem)
			{
				MobaCharacter->BuyItem(DebugItemIndex);
			}
			if (bDebugSellItem)
			{
				MobaCharacter->SellItem(DebugItemIndex);
			}
			if (bDebugRecall)
			{
				MobaCharacter->StartRecall();
			}
		}

		bDebugBuyItem = false;
		bDebugSellItem = false;
		bDebugRecall = false;
	}

	if (bDebugPlaceControlWard)
	{
		bDebugPlaceControlWard = false;
		if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
		{
			MobaCharacter->PlaceControlWard(DebugCastLocation);
		}
	}

	if (!DebugChatMessage.IsEmpty())
	{
		const FString Message = DebugChatMessage;
		DebugChatMessage.Reset();
		SendChat(Message);
	}

	if (bDebugOpenChat)
	{
		bDebugOpenChat = false;
		OnReturnToMenu();
	}

	if (!DebugConsoleCommand.IsEmpty())
	{
		const FString Command = DebugConsoleCommand;
		DebugConsoleCommand.Reset();
		ConsoleCommand(Command);
	}

	if (bDebugUseHeraldEye)
	{
		bDebugUseHeraldEye = false;
		OnUseHeraldEye();
	}

	if (bDebugUsePotion)
	{
		bDebugUsePotion = false;
		OnUsePotion();
	}

	if (bDebugPlaceWard)
	{
		bDebugPlaceWard = false;
		if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
		{
			MobaCharacter->PlaceWard(DebugCastLocation);
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

	// 챔피언·미니언(폰), 와드(물리 물체), 건물(동적 오브젝트), 지형(정적 오브젝트) 중 커서에 가장 먼저 닿는 것을 본다.
	static const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_PhysicsBody),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic),
		UEngineTypes::ConvertToObjectType(ECC_WorldStatic)
	};

	FHitResult Hit;
	if (!GetHitResultUnderCursorForObjects(ObjectTypes, false, Hit))
	{
		return nullptr;
	}

	OutCursorLocation = Hit.Location;

	// 시야 밖이라 화면에 보이지 않는 적은 클릭할 수 없다.
	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this);
	const auto IsSeen = [Vision](const AActor* Actor)
	{
		return !Vision || Vision->IsVisibleToLocalPlayer(Actor);
	};

	if (MobaCombat::IsValidEnemyTarget(MobaCharacter, Hit.GetActor()) && IsSeen(Hit.GetActor()))
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
		if (Distance <= NearestDistance && IsSeen(Enemy))
		{
			Nearest = Enemy;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

bool AMobaPlayerController::GetPointerPosition(float& OutX, float& OutY) const
{
#if WITH_EDITORONLY_DATA
	if (bDebugPointerOverride)
	{
		OutX = DebugPointerPosition.X;
		OutY = DebugPointerPosition.Y;
		return true;
	}
#endif
	return GetMousePosition(OutX, OutY);
}

bool AMobaPlayerController::GetCursorMinimapLocation(FVector& OutLocation) const
{
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const AMobaHUD* MobaHUD = GetHUD<AMobaHUD>();
	return MobaHUD && GetPointerPosition(MouseX, MouseY) && MobaHUD->GetMinimapWorldLocation(FVector2D(MouseX, MouseY), OutLocation);
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
	// 상점 창 위에서 누른 것은 지면 명령으로 보지 않는다.
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const AMobaHUD* MobaHUD = GetHUD<AMobaHUD>();
	if (MobaHUD && GetPointerPosition(MouseX, MouseY) && MobaHUD->IsOverPanel(FVector2D(MouseX, MouseY)))
	{
		return;
	}

	FVector CursorLocation;
	if (GetCursorMinimapLocation(CursorLocation))
	{
		// 미니맵을 우클릭하면 그 지점으로 걸어간다.
		IssueMoveTo(CursorLocation);
	}
	else if (AActor* Enemy = FindEnemyAtCursor(CursorLocation))
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
	if (GetCursorMinimapLocation(CursorLocation))
	{
		IssueAttackMoveTo(CursorLocation);
	}
	else if (AActor* Enemy = FindEnemyAtCursor(CursorLocation))
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
	if (!MobaCharacter || !MobaCharacter->CanMove())
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
	if (IsAltKeyDown())
	{
		SendPingAtCursor();
		return;
	}

	// 상점 창이나 가방을 눌렀으면 거기서 처리하고 끝낸다.
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	AMobaHUD* MobaHUD = GetHUD<AMobaHUD>();
	if (MobaHUD && GetPointerPosition(MouseX, MouseY) && MobaHUD->HandleClick(FVector2D(MouseX, MouseY)))
	{
		return;
	}

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
	else
	{
		// 미니맵을 좌클릭하면 카메라를 그 지점으로 옮긴다. 카메라 고정은 풀린다.
		FVector MapLocation;
		if (CameraRig && GetCursorMinimapLocation(MapLocation))
		{
			bCameraLocked = false;
			CameraRig->SetFocusLocation(MapLocation);
		}
	}
}

void AMobaPlayerController::OnPlaceControlWard()
{
	AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	FVector GroundLocation;
	if (MobaCharacter && GetCursorGroundLocation(GroundLocation))
	{
		MobaCharacter->PlaceControlWard(GroundLocation);
	}
}

void AMobaPlayerController::OnUseHeraldEye()
{
	if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
	{
		MobaCharacter->UseHeraldEye();
	}
}

void AMobaPlayerController::OnUsePotion()
{
	if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
	{
		MobaCharacter->UsePotion();
	}
}

void AMobaPlayerController::OnPlaceWard()
{
	AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>();
	FVector GroundLocation;
	if (MobaCharacter && GetCursorGroundLocation(GroundLocation))
	{
		MobaCharacter->PlaceWard(GroundLocation);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 귀환, 상점, 점수판, 핑

void AMobaPlayerController::OnRecall()
{
	if (AMobaCharacter* MobaCharacter = GetPawn<AMobaCharacter>())
	{
		bTargeting = false;
		bAttackTargeting = false;
		MobaCharacter->StartRecall();
	}
}

void AMobaPlayerController::OnToggleShop()
{
	bShopOpen = !bShopOpen;
}

void AMobaPlayerController::OnToggleGameMenu()
{
	bGameMenuOpen = !bGameMenuOpen;
}

void AMobaPlayerController::LeaveMatch()
{
	bGameMenuOpen = false;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->ReturnToMainMenu();
	}
}

void AMobaPlayerController::MobaGold(float Amount)
{
	ServerDevCommand(EMobaDevCommand::Gold, Amount);
}

void AMobaPlayerController::MobaLevel(int32 Level)
{
	ServerDevCommand(EMobaDevCommand::Level, static_cast<float>(Level));
}

void AMobaPlayerController::MobaSkipTime(float Seconds)
{
	ServerDevCommand(EMobaDevCommand::SkipTime, Seconds);
}

void AMobaPlayerController::MobaRefresh()
{
	ServerDevCommand(EMobaDevCommand::Refresh, 0.0f);
}

void AMobaPlayerController::MobaSpawnCamps()
{
	ServerDevCommand(EMobaDevCommand::SpawnCamps, 0.0f);
}

void AMobaPlayerController::ServerDevCommand_Implementation(EMobaDevCommand Command, float Value)
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleDevCommand(this, Command, Value);
	}
}

void AMobaPlayerController::ServerRequestSurrender_Implementation()
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->RequestSurrender(this);
	}
}

void AMobaPlayerController::OnScoreboardStarted()
{
	bScoreboardHeld = true;
}

void AMobaPlayerController::OnScoreboardCompleted()
{
	bScoreboardHeld = false;
}

bool AMobaPlayerController::IsAltKeyDown() const
{
	return IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
}

void AMobaPlayerController::SendPingAtCursor()
{
	FVector Location;
	if (GetCursorMinimapLocation(Location) || GetCursorGroundLocation(Location))
	{
		ServerPing(Location);
	}
}

void AMobaPlayerController::ServerPing_Implementation(FVector_NetQuantize Location)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const AMobaPlayerState* MyState = GetPlayerState<AMobaPlayerState>();
	if (!MyState || Now - LastPingTime < 0.5)
	{
		return;
	}
	LastPingTime = Now;

	// 같은 팀에게만 보인다.
	for (TActorIterator<AMobaPlayerController> It(GetWorld()); It; ++It)
	{
		const AMobaPlayerState* OtherState = It->GetPlayerState<AMobaPlayerState>();
		if (OtherState && OtherState->GetTeam() == MyState->GetTeam())
		{
			It->ClientShowPing(Location);
		}
	}
}

void AMobaPlayerController::RequestChampion(const FString& AssetName)
{
	const AMobaPlayerState* MyState = GetPlayerState<AMobaPlayerState>();
	if (MyState && MyState->IsReady())
	{
		return;
	}

	if (UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>())
	{
		MobaGameInstance->SetSelectedChampion(AssetName);
	}
	ServerRequestChampion(AssetName);
}

void AMobaPlayerController::RequestSummonerSpell(int32 Index, EMobaSummonerSpell Spell)
{
	const AMobaPlayerState* MyState = GetPlayerState<AMobaPlayerState>();
	if (MyState && MyState->IsReady())
	{
		return;
	}

	if (UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>())
	{
		MobaGameInstance->SetSelectedSpell(Index, Spell);
	}
	ServerRequestSummonerSpell(Index, Spell);
}

void AMobaPlayerController::ServerRequestChampion_Implementation(const FString& AssetName)
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->RequestChampionChange(this, AssetName);
	}
}

void AMobaPlayerController::ServerRequestSummonerSpell_Implementation(int32 Index, EMobaSummonerSpell Spell)
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->RequestSummonerSpell(this, Index, Spell);
	}
}

void AMobaPlayerController::ServerRequestReady_Implementation(bool bReady)
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->RequestReady(this, bReady);
	}
}

void AMobaPlayerController::ClientShowDeathRecap_Implementation(const FString& Killer, const TArray<FMobaDeathRecapEntry>& Entries)
{
	DeathRecapKiller = Killer;
	DeathRecap = Entries;
}

void AMobaPlayerController::ClientShowPing_Implementation(FVector_NetQuantize Location)
{
	if (Pings.Num() >= 8)
	{
		Pings.RemoveAt(0);
	}

	Pings.Add({ FVector(Location), GetWorld()->GetTimeSeconds() });
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

void AMobaPlayerController::OnSummonerD()
{
	HandleAbilityKey(EMobaAbilitySlot::D);
}

void AMobaPlayerController::OnSummonerF()
{
	HandleAbilityKey(EMobaAbilitySlot::F);
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
