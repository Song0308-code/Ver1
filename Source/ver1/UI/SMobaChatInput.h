#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;

DECLARE_DELEGATE_OneParam(FOnMobaChatSubmit, const FString& /*Text*/);
DECLARE_DELEGATE(FOnMobaChatCancel);

/** 경기 중 Enter로 여는 채팅 입력창. Enter로 보내고 Esc로 닫는다. */
class SMobaChatInput : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMobaChatInput) {}
		SLATE_EVENT(FOnMobaChatSubmit, OnSubmit)
		SLATE_EVENT(FOnMobaChatCancel, OnCancel)
		/** 화면 아래에서 띄울 거리(Slate 단위). 능력치 패널 위에 오도록 컨트롤러가 화면 배율로 계산해 넘긴다. */
		SLATE_ARGUMENT(float, BottomPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 입력칸에 키보드 초점을 준다. */
	TSharedPtr<SWidget> GetFocusTarget() const;

private:
	void HandleCommitted(const FText& Text, ETextCommit::Type CommitType);

	FOnMobaChatSubmit OnSubmit;
	FOnMobaChatCancel OnCancel;
	TSharedPtr<SEditableTextBox> TextBox;
};
