#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

namespace MobaVisuals
{
	/**
	 * 임시 도형에 Color 파라미터가 있는 엔진 기본 도형 머티리얼을 입힌다.
	 * 이 머티리얼이 있어야 SetVectorParameterValueOnMaterials("Color")로 팀 색을 바꿀 수 있다.
	 * 내부에서 ConstructorHelpers를 쓰므로 생성자에서만 호출한다.
	 */
	VER1_API void UseColorableMaterial(UStaticMeshComponent* Mesh);
}
