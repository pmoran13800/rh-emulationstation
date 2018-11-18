#include "BusyComponent.h"

#include "components/AnimatedImageComponent.h"
#include "components/TextComponent.h"
#include "Renderer.h"
#include "Locale.h"
#include "MenuThemeData.h"

// animation definition
AnimationFrame BUSY_ANIMATION_FRAMES[] = {
	{":/busy_0.png", 300},
	{":/busy_1.png", 300},
	{":/busy_2.png", 300},
	{":/busy_3.png", 300},
};
const AnimationDef BUSY_ANIMATION_DEF = { BUSY_ANIMATION_FRAMES, 4, true };

using namespace Eigen;

BusyComponent::BusyComponent(Window* window) : GuiComponent(window),
	mBackground(window, ":/frame.png"), mGrid(window, Vector2i(5, 3))
{
	mutex = SDL_CreateMutex();


    auto menuTheme = MenuThemeData::getInstance()->getCurrentTheme();

    mBackground.setImagePath(menuTheme->menuBackground.path);
    mBackground.setCenterColor(menuTheme->menuBackground.color);
    mBackground.setEdgeColor(menuTheme->menuBackground.color);

	mAnimation = std::make_shared<AnimatedImageComponent>(mWindow);
	mAnimation->load(&BUSY_ANIMATION_DEF);
	mText = std::make_shared<TextComponent>(mWindow, _("WORKING..."), menuTheme->menuText.font, menuTheme->menuText.color);

	// col 0 = animation, col 1 = spacer, col 2 = text
	mGrid.setEntry(mAnimation, Vector2i(1, 1), false, true);
	mGrid.setEntry(mText, Vector2i(3, 1), false, true);

	addChild(&mBackground);
	addChild(&mGrid);
}

BusyComponent::~BusyComponent() {
	SDL_DestroyMutex(mutex);
}

void BusyComponent::setText(std::string txt) {
	if (SDL_LockMutex(mutex) == 0) {
		threadMessage = txt;
		threadMessagechanged = true;
		SDL_UnlockMutex(mutex);
	}
}

void BusyComponent::render(const Eigen::Affine3f& parentTrans) {
	if (SDL_LockMutex(mutex) == 0) {
		if(threadMessagechanged) {
			threadMessagechanged = false;
			mText->setText(threadMessage);
			onSizeChanged();
		}
		SDL_UnlockMutex(mutex);
	}
	GuiComponent::render(parentTrans);
}

void BusyComponent::onSizeChanged()
{
	mGrid.setSize(mSize);

	if(mSize.x() == 0 || mSize.y() == 0)
		return;

	const float middleSpacerWidth = 0.01f * Renderer::getScreenWidth();
	const float textHeight = mText->getFont()->getLetterHeight();
	mText->setSize(0, textHeight);
	const float textWidth = mText->getSize().x() + 4;

	mGrid.setColWidthPerc(1, textHeight / mSize.x()); // animation is square
	mGrid.setColWidthPerc(2, middleSpacerWidth / mSize.x());
	mGrid.setColWidthPerc(3, textWidth / mSize.x());

	mGrid.setRowHeightPerc(1, textHeight / mSize.y());
	
	mBackground.fitTo(Vector2f(mGrid.getColWidth(1) + mGrid.getColWidth(2) + mGrid.getColWidth(3), textHeight + 2),
		mAnimation->getPosition(), Vector2f(0, 0));
}

void BusyComponent::reset()
{
	//mAnimation->reset();
}
