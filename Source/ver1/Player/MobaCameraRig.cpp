#include "Player/MobaCameraRig.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AMobaCameraRig::AMobaCameraRig()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeRotation(FRotator(-56.0, 0.0, 0.0));
	CameraBoom->TargetArmLength = 2000.0f;
	CameraBoom->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(45.0f);
}

void AMobaCameraRig::SetFocusLocation(const FVector& NewLocation)
{
	FVector Clamped = NewLocation;
	Clamped.X = FMath::Clamp(Clamped.X, FocusBounds.Min.X, FocusBounds.Max.X);
	Clamped.Y = FMath::Clamp(Clamped.Y, FocusBounds.Min.Y, FocusBounds.Max.Y);
	SetActorLocation(Clamped);
}

void AMobaCameraRig::Pan(const FVector2D& ScreenDirection, float DeltaSeconds)
{
	// 카메라 요가 0이므로 화면 위쪽이 월드 +X, 오른쪽이 월드 +Y다.
	const FVector Offset = FVector(ScreenDirection.Y, ScreenDirection.X, 0.0) * PanSpeed * DeltaSeconds;
	SetFocusLocation(GetActorLocation() + Offset);
}

void AMobaCameraRig::Zoom(float Amount)
{
	CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Amount * ZoomStep, MinArmLength, MaxArmLength);
}
