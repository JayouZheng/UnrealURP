// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

/**
 * Customizes MultiPassFlags as a horizontal row of buttons.
 */
class UNREALURPED_API FMultiPassFlagsCustomization : public IPropertyTypeCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	FMultiPassFlagsCustomization(){}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:

	FText GetStructPropertyNameText() const;
	FText GetStructPropertyTooltipText() const;

	bool IsMultiPassFlagButtonEditable(uint32 ChildIndex) const;
	void OnButtonCheckedStateChanged(ECheckBoxState NewState, uint32 ChildIndex) const;
	ECheckBoxState GetButtonCheckedState(uint32 ChildIndex) const;

	TSharedPtr<IPropertyHandle> MultiPassFlagsHandle;
	FCheckBoxStyle Style;
};
