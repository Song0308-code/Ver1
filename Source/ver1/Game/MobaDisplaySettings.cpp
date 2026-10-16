#include "Game/MobaDisplaySettings.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
	const FIntPoint Resolutions[] = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440) };

	void ApplyAndSave(UGameUserSettings* Settings)
	{
		// 에디터에서 실행 중일 때는 저장만 하고 창은 건드리지 않는다.
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void MobaDisplaySettings::ToggleFullscreen()
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const bool bWindowed = Settings->GetFullscreenMode() == EWindowMode::Windowed;
		Settings->SetFullscreenMode(bWindowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		ApplyAndSave(Settings);
	}
}

void MobaDisplaySettings::CycleResolution()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return;
	}

	const FIntPoint Current = Settings->GetScreenResolution();
	int32 Next = 0;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Resolutions); ++Index)
	{
		if (Resolutions[Index] == Current)
		{
			Next = (Index + 1) % UE_ARRAY_COUNT(Resolutions);
			break;
		}
	}

	Settings->SetScreenResolution(Resolutions[Next]);
	ApplyAndSave(Settings);
}

FString MobaDisplaySettings::DescribeWindowMode()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return FString();
	}
	return Settings->GetFullscreenMode() == EWindowMode::Windowed ? TEXT("창 모드") : TEXT("전체 화면");
}

FString MobaDisplaySettings::DescribeResolution()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return FString();
	}
	const FIntPoint Resolution = Settings->GetScreenResolution();
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}
