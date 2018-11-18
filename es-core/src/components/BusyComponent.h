#pragma once
#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"

class AnimatedImageComponent;
class TextComponent;

class BusyComponent : public GuiComponent
{
public:
	BusyComponent(Window* window);
	~BusyComponent();

	void onSizeChanged() override;
	void setText(std::string txt);

	void reset(); // reset to frame 0
	virtual void render(const Eigen::Affine3f& parentTrans);
private:
	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<AnimatedImageComponent> mAnimation;
	std::shared_ptr<TextComponent> mText;

	SDL_mutex *mutex;
	bool threadMessagechanged;
	std::string threadMessage;
};