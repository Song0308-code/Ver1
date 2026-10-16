#pragma once

#include "CoreMinimal.h"

/** 메인 메뉴와 게임 메뉴가 같이 쓰는 화면 설정. 엔진의 게임 사용자 설정에 저장되어 다음 실행에도 유지된다. */
namespace MobaDisplaySettings
{
	/** 창 모드와 전체 화면(창 테두리 없는 전체 화면)을 오간다. */
	VER1_API void ToggleFullscreen();

	/** 미리 정한 해상도 목록에서 다음 것으로 바꾼다. */
	VER1_API void CycleResolution();

	/** 지금 화면 모드를 사람이 읽을 수 있게. */
	VER1_API FString DescribeWindowMode();

	VER1_API FString DescribeResolution();
}
