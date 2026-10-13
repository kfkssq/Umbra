#pragma once

#include "Layout/ArrangedChildren.h"
#include "Widgets/SCompoundWidget.h"

/** Fit the painted tooltip without feeding the fit scale back into text measurement. */
class SUmbraTooltipViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUmbraTooltipViewport) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) { ChildSlot[Args._Content.Widget]; }

	void SetFitScale(float InScale)
	{
		if (FitScale != InScale)
		{
			FitScale = InScale;
			Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	virtual void OnArrangeChildren(const FGeometry& Geometry, FArrangedChildren& Children) const override
	{
		const TSharedRef<SWidget> Content = ChildSlot.GetWidget();
		// Allocate its natural layout on every frame. Only painting is scaled, so wrapping
		// and font measurement cannot alternate as the viewport fit changes their scale.
		const FGeometry PaintedGeometry = Geometry.MakeChild(FSlateRenderTransform(FitScale), FVector2D::ZeroVector);
		// The widget-aware overload also retains the authored root's render transform.
		Children.AddWidget(PaintedGeometry.MakeChild(Content, Content->GetDesiredSize(), FSlateLayoutTransform()));
	}

protected:
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(ChildSlot.GetWidget()->GetDesiredSize()) * FitScale;
	}

private:
	float FitScale = 1.f;
};
