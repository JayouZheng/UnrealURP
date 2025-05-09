// Copyright Jayou, Inc. All Rights Reserved.

#include "MultiPassFlagsCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Internationalization/Text.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FMultiPassFlagsCustomization"

TSharedRef<IPropertyTypeCustomization> FMultiPassFlagsCustomization::MakeInstance()
{
	return MakeShareable( new FMultiPassFlagsCustomization );
}

void FMultiPassFlagsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	MultiPassFlagsHandle = StructPropertyHandle;

	TSharedRef<SHorizontalBox> ButtonOptionsPanel =
		SNew(SHorizontalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
	;
	
	HeaderRow
	.NameContent()
	[
		SNew(STextBlock)
		.Text(this, &FMultiPassFlagsCustomization::GetStructPropertyNameText)
		.ToolTipText(this, &FMultiPassFlagsCustomization::GetStructPropertyTooltipText)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Visibility(EVisibility::SelfHitTestInvisible)
	]
	.ValueContent()
	[
		ButtonOptionsPanel
	];

	uint32 ChildCount;
	MultiPassFlagsHandle->GetNumChildren(ChildCount);

	for (uint32 ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++)
	{
		const FText NumericText = FText::AsNumber(ChildIndex);
		const FText SlotTooltipText = FText::Format(LOCTEXT("MultiPassFlagToggleFormat", "Toggle MultiPass Flag {0}"), NumericText);

		const bool bIsLastChild = ChildIndex == ChildCount - 1;
		
		ButtonOptionsPanel->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(0, 0, bIsLastChild ? 0 : 8, 0)
		[
			SNew(SBox)
			.WidthOverride(20)
			.HAlign(HAlign_Fill)
			.IsEnabled(this, &FMultiPassFlagsCustomization::IsMultiPassFlagButtonEditable, ChildIndex)
			[
				SNew(SCheckBox)
				.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("DetailsView.ChannelToggleButton"))
				.ToolTipText(SlotTooltipText)
				.OnCheckStateChanged(this, &FMultiPassFlagsCustomization::OnButtonCheckedStateChanged, ChildIndex)
				.IsChecked(this, &FMultiPassFlagsCustomization::GetButtonCheckedState, ChildIndex)
				.HAlign(HAlign_Center)
				.Padding(FMargin(0, 2))
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("SmallText"))
					.Visibility(EVisibility::HitTestInvisible)
					.Text(NumericText)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];
	}
}

void FMultiPassFlagsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Display channels as a normal foldout struct
	uint32 ChildCount;
	PropertyHandle->GetNumChildren(ChildCount);

	for (uint32 ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++)
	{
		if (TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex))
		{
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
		}
	}
}

FText FMultiPassFlagsCustomization::GetStructPropertyNameText() const
{
	if (MultiPassFlagsHandle.IsValid())
	{
		return MultiPassFlagsHandle->GetPropertyDisplayName();
	}

	return FText::GetEmpty();
}

FText FMultiPassFlagsCustomization::GetStructPropertyTooltipText() const
{
	if (MultiPassFlagsHandle.IsValid())
	{
		return MultiPassFlagsHandle->GetToolTipText();
	}

	return FText::GetEmpty();
}

bool FMultiPassFlagsCustomization::IsMultiPassFlagButtonEditable(uint32 ChildIndex) const
{
	if (MultiPassFlagsHandle.IsValid())
	{
		if (TSharedPtr<IPropertyHandle> ChildHandle = MultiPassFlagsHandle->GetChildHandle(ChildIndex))
		{
			return ChildHandle->IsEditable() && !ChildHandle->IsEditConst();
		}
	}
	return false;
}

void FMultiPassFlagsCustomization::OnButtonCheckedStateChanged(ECheckBoxState NewState, uint32 ChildIndex) const
{
	if (MultiPassFlagsHandle.IsValid())
	{
		uint32 OutNumChildren = 0;
		MultiPassFlagsHandle->GetNumChildren(OutNumChildren);

		if (ChildIndex < OutNumChildren)
		{
			MultiPassFlagsHandle->GetChildHandle(ChildIndex)->SetValue(NewState == ECheckBoxState::Checked);
			MultiPassFlagsHandle->GetChildHandle(ChildIndex)->NotifyFinishedChangingProperties();
			MultiPassFlagsHandle->NotifyFinishedChangingProperties();
		}
	}
}

ECheckBoxState FMultiPassFlagsCustomization::GetButtonCheckedState(uint32 ChildIndex) const
{
	if (MultiPassFlagsHandle.IsValid())
	{
		uint32 OutNumChildren = 0;
		MultiPassFlagsHandle->GetNumChildren(OutNumChildren);

		if (ChildIndex < OutNumChildren)
		{
			bool Value;
			MultiPassFlagsHandle->GetChildHandle(ChildIndex)->GetValue(Value);
			return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
	}

	return ECheckBoxState::Undetermined;
}

#undef LOCTEXT_NAMESPACE
