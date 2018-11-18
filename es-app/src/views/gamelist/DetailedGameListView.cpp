#include <RHGamestationConf.h>
#include "views/gamelist/DetailedGameListView.h"
#include "views/ViewController.h"
#include "Window.h"
#include "Settings.h"
#include "animations/LambdaAnimation.h"
#include "Locale.h"

DetailedGameListView::DetailedGameListView(Window* window, FileData* root, SystemData* system) :
    BasicGameListView(window, root), 
    mDescContainer(window), mDescription(window), 
    mImage(window), mSystem(system), 

    mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window), 
    mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window), mLblFavorite(window),

    mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
    mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window), mFavorite(window),

    mFolderName(window)
{
    const float padding = 0.01f;

    mList.setPosition(mSize.x() * (0.50f + padding), mList.getPosition().y());
    mList.setSize(mSize.x() * (0.50f - padding), mList.getSize().y());
    mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
    mList.setCursorChangedCallback([&](const CursorState& state) { updateInfoPanel(); });

    // folder components
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            ImageComponent *img = new ImageComponent(window);
            addChild(img); // normalised functions required to be added first
            img->setOrigin(0.5f, 0.5f);
            img->setNormalisedMaxSize(0.4f, 0.4f);
            img->setDefaultZIndex(30);
            img->setZIndex(30);
            mFolderContent.push_back(img);
        }
    }

    addChild(&mFolderName);
    mFolderName.setDefaultZIndex(40);

    // image
    mImage.setOrigin(0.5f, 0.5f);
    mImage.setPosition(mSize.x() * 0.25f, mList.getPosition().y() + mSize.y() * 0.2125f);
    mImage.setMaxSize(mSize.x() * (0.50f - 2*padding), mSize.y() * 0.4f);
    mImage.setDefaultZIndex(30);
    addChild(&mImage);

    // metadata labels + values
    mLblRating.setText(_("Rating") + ": ");
    addChild(&mLblRating);
    addChild(&mRating);
    mLblReleaseDate.setText(_("Released") + ": ");
    addChild(&mLblReleaseDate);
    addChild(&mReleaseDate);
    mLblDeveloper.setText(_("Developer") + ": ");
    addChild(&mLblDeveloper);
    addChild(&mDeveloper);
    mLblPublisher.setText(_("Publisher") + ": ");
    addChild(&mLblPublisher);
    addChild(&mPublisher);
    mLblGenre.setText(_("Genre") + ": ");
    addChild(&mLblGenre);
    addChild(&mGenre);
    mLblPlayers.setText(_("Players") + ": ");
    addChild(&mLblPlayers);
    addChild(&mPlayers);
    mLblLastPlayed.setText(_("Last played") + ": ");
    addChild(&mLblLastPlayed);
    mLastPlayed.setDisplayMode(DateTimeComponent::DISP_RELATIVE_TO_NOW);
    addChild(&mLastPlayed);
    mLblPlayCount.setText(_("Times played") + ": ");
    addChild(&mLblPlayCount);
    addChild(&mPlayCount);
    if (system->getHasFavorites()) {
      mLblFavorite.setText(_("Favorite") + ": ");
        addChild(&mLblFavorite);
        addChild(&mFavorite);
    }

    mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
    mDescContainer.setSize(mSize.x() * (0.50f - 2*padding), mSize.y() - mDescContainer.getPosition().y());
    mDescContainer.setAutoScroll(true);
    mDescContainer.setDefaultZIndex(40);
    addChild(&mDescContainer);

    mDescription.setFont(Font::get(FONT_SIZE_SMALL));
    mDescription.setSize(mDescContainer.getSize().x(), 0);
    mDescContainer.addChild(&mDescription);

    initMDLabels();
    initMDValues();
    updateInfoPanel();
}

void DetailedGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme) {
    BasicGameListView::onThemeChanged(theme);

    using namespace ThemeFlags;
    mImage.applyTheme(theme, getName(), "md_image", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION);

    initMDLabels();
    std::vector<TextComponent*> labels = getMDLabels();
    std::vector<std::string> names({"md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer", "md_lbl_publisher",
                                    "md_lbl_genre", "md_lbl_players", "md_lbl_lastplayed", "md_lbl_playcount"});

    if (mSystem->getHasFavorites()) {
        names.push_back("md_lbl_favorite");
    }

    assert(names.size() == labels.size());
    for (unsigned int i = 0; i < labels.size(); i++) {
        labels[i]->applyTheme(theme, getName(), names[i], ALL);
    }

    initMDValues();
    std::vector<GuiComponent*> values = getMDValues();
    names = {"md_rating", "md_releasedate", "md_developer", "md_publisher", "md_genre", "md_players", "md_lastplayed", "md_playcount"};

    if (mSystem->getHasFavorites()) {
        names.push_back("md_favorite");
    }

    names.push_back("md_folder_name");
    values.push_back(&mFolderName);

    assert(names.size() == values.size());
    for (unsigned int i = 0; i < values.size(); i++) {
        values[i]->applyTheme(theme, getName(), names.at(i), ALL ^ ThemeFlags::TEXT);
    }

    mDescContainer.applyTheme(theme, getName(), "md_description", POSITION | ThemeFlags::SIZE | Z_INDEX);
    mDescription.setSize(mDescContainer.getSize().x(), 0);
    mDescription.applyTheme(theme, getName(), "md_description", ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT));

    if (theme->isFolderHandled()) {
        char strbuf[256];
        mFolderName.applyTheme(theme, getName(), "md_folder_name", ALL);
        for (int i = 0; i < mFolderContent.size(); i++) {
            snprintf(strbuf, 256, "md_folder_image_%d", i);
            mFolderContent.at(i)->applyTheme(theme, getName(), strbuf, POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION);
        }
    } else {
        // backward compatibility
        auto size = mImage.getSize().isZero() ? mImage.getTargetSize() : mImage.getSize();
        float minSize = std::min(size.x() , size.y());
        float left = mImage.getPosition().x() - mImage.getOrigin().x() * minSize;
        float top = mImage.getPosition().y() - mImage.getOrigin().y() * minSize;

        mFolderName.setPosition(left, top);
        mFolderName.setZIndex(40);
        mFolderName.setFont(Font::get(FONT_SIZE_EXTRASMALL));
        mFolderName.setColor(0xFFFFFFFF);

        const unsigned int grid = 3; // 3 x 3
        const float relativeMarge = 0.1f;

        float labelHeight = 2.5f * Font::get(FONT_SIZE_EXTRASMALL)->getLetterHeight();
        top += labelHeight;

        minSize = std::min(size.x() , size.y() - labelHeight);

        const float imgSize = minSize / (grid + 2.0f * relativeMarge);

        // centering with unused space
        left += 0.5f * (minSize - grid * imgSize - 2.0f * relativeMarge);

        for (unsigned int x = 0; x < grid; x++) {
            for (unsigned int y = 0; y < grid; y++) {
                ImageComponent * img = mFolderContent.at(x + y * grid);
                img->setMaxSize(imgSize, imgSize);
                img->setPosition(
                        left + imgSize * img->getOrigin().x() + x * (1 + relativeMarge) * imgSize,
                        top  + imgSize * img->getOrigin().y() + y * (1 + relativeMarge) * imgSize
                );
                img->setZIndex(30);
            }
        }
    }

    sortChildren();
}

void DetailedGameListView::initMDLabels() {
    using namespace Eigen;

    std::vector<TextComponent*> components = getMDLabels();

    const unsigned int colCount = 2;
    const unsigned int rowCount = components.size() / 2;

    Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);
    
    const float colSize = (mSize.x() * 0.48f) / colCount;
    const float rowPadding = 0.01f * mSize.y();

    for (unsigned int i = 0; i < components.size(); i++) {
        const unsigned int row = i % rowCount;
        Vector3f pos(0.0f, 0.0f, 0.0f);
        if (row == 0) {
            pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
        } else {
            // work from the last component
            GuiComponent* lc = components[i-1];
            pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
        }

        components[i]->setFont(Font::get(FONT_SIZE_SMALL));
        components[i]->setPosition(pos);
        components[i]->setDefaultZIndex(40);
    }
}

void DetailedGameListView::initMDValues() {
    using namespace Eigen;

    std::vector<TextComponent*> labels = getMDLabels();
    std::vector<GuiComponent*> values = getMDValues();

    std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
    mRating.setSize(defaultFont->getHeight() * 5.0f, (float)defaultFont->getHeight());
    mReleaseDate.setFont(defaultFont);
    mDeveloper.setFont(defaultFont);
    mPublisher.setFont(defaultFont);
    mGenre.setFont(defaultFont);
    mPlayers.setFont(defaultFont);
    mLastPlayed.setFont(defaultFont);
    mPlayCount.setFont(defaultFont);
    mFavorite.setFont(defaultFont);

    float bottom = 0.0f;

    const float colSize = (mSize.x() * 0.48f) / 2;
    for (unsigned int i = 0; i < labels.size(); i++) {
        const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
        values[i]->setPosition(labels[i]->getPosition() + Vector3f(labels[i]->getSize().x(), heightDiff, 0));
        values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
        values[i]->setDefaultZIndex(40);

        float testBot = values[i]->getPosition().y() + values[i]->getSize().y();
        if (testBot > bottom)
            bottom = testBot;
    }

    mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + mSize.y() * 0.01f);
    mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() - mDescContainer.getPosition().y());
}

void DetailedGameListView::updateInfoPanel() {
    FileData* file = (mList.size() == 0 || mList.isScrolling()) ? NULL : mList.getSelected();

    if (file == NULL) {
        fadeOut(getFolderComponents(), true);
        fadeOut(getGameComponents(), true);
    } else {
        const bool isGame = file->getType() == GAME;
        const bool hasImage = !file->metadata.get("image").empty();

        if (hasImage && !isGame) {
            setScrappedFolderInfo(file);
            switchToFolderScrappedDisplay();
        } else {
            if (isGame) {
                setGameInfo(file);
            } else {
                setFolderInfo(file);
            }
            switchDisplay(isGame);
        }
    }
}

bool DetailedGameListView::switchToFolderScrappedDisplay() {
    fadeOut(getGameComponents(false), true);
    fadeOut(getFolderComponents(), true);
    fadeOut(getScrappedFolderComponents(), false);
}

bool DetailedGameListView::switchDisplay(bool isGame) {
    fadeOut(getGameComponents(), !isGame);
    fadeOut(getFolderComponents(), isGame);
}

std::vector<GuiComponent*> DetailedGameListView::getFolderComponents() {
    std::vector<GuiComponent*> comps;
    for (auto img: mFolderContent) {
        comps.push_back(img);
    }
    comps.push_back(&mFolderName);
    return comps;
}

std::vector<GuiComponent*> DetailedGameListView::getGameComponents(bool includeMainComponents) {
    std::vector<GuiComponent*> comps = getMDValues();
    if (includeMainComponents) {
        comps.push_back(&mImage);
        comps.push_back(&mDescription);
    }
    std::vector<TextComponent*> labels = getMDLabels();
    comps.insert(comps.end(), labels.begin(), labels.end());
    return comps;
}

std::vector<GuiComponent*> DetailedGameListView::getScrappedFolderComponents() {
    std::vector<GuiComponent*> comps;
    comps.push_back(&mImage);
    comps.push_back(&mDescription);
    return comps;
}

void DetailedGameListView::setFolderInfo(FileData* file) {
    std::vector<FileData*> games;
    char strbuf[256];

    getFolderGames(file, games);
    snprintf(strbuf, 256, ngettext("%i GAME AVAILABLE", "%i GAMES AVAILABLE", games.size()).c_str(), games.size());
    mFolderName.setText(file->getName() + " - " + strbuf);

    unsigned char idx = 0;

    for (FileData* game: games) {
        const std::string& thumbnail = game->metadata.get("thumbnail");
        const std::string& image = game->metadata.get("image");
        if (!thumbnail.empty() || !image.empty()) {
            mFolderContent.at(idx)->setImage(thumbnail.empty() ? image : thumbnail);
            idx++;
            if (idx == mFolderContent.size()) {
                break;
            }
        }
    }
    for (int i = idx; i < mFolderContent.size(); i++) {
        mFolderContent.at(i)->setImage("");
    }
}

void DetailedGameListView::setGameInfo(FileData* file) {
    mRating.setValue(file->metadata.get("rating"));
    mReleaseDate.setValue(file->metadata.get("releasedate"));
    mDeveloper.setValue(file->metadata.get("developer"));
    mPublisher.setValue(file->metadata.get("publisher"));
    mGenre.setValue(file->metadata.get("genre"));
    mPlayers.setValue(file->metadata.get("players"));
    mLastPlayed.setValue(file->metadata.get("lastplayed"));
    mPlayCount.setValue(file->metadata.get("playcount"));
    mFavorite.setValue(file->metadata.get("favorite"));

    mImage.setImage(file->metadata.get("image"));
    mDescription.setText(file->metadata.get("desc"));
    mDescContainer.reset();
}

void DetailedGameListView::setScrappedFolderInfo(FileData* file) {
    mImage.setImage(file->metadata.get("image"));
    mDescription.setText(file->metadata.get("desc"));
    mDescContainer.reset();
}

void DetailedGameListView::getFolderGames(FileData* folder, std::vector<FileData*> &output) {
    std::vector<FileData*> items = folder->getChildren();
    for (auto it = items.begin(); it != items.end(); it++) {
        if ((*it)->getType() == GAME) {
            output.push_back(*it);
        } else {
            getFolderGames(*it, output);
        }
    }
}

void DetailedGameListView::fadeOut(std::vector<GuiComponent*> comps, bool fadingOut) {
    for (auto it = comps.begin(); it != comps.end(); it++) {
        GuiComponent* comp = *it;
        // an animation is playing
        //   then animate if reverse != fadingOut
        // an animation is not playing
        //   then animate if opacity != our target opacity
        if ((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
            (!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255))) {
            auto func = [comp](float t) {
                comp->setOpacity((unsigned char)(lerp<float>(0.0f, 1.0f, t)*255));
            };
            comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
        }
    }
}

void DetailedGameListView::launch(FileData* game) {
    Eigen::Vector3f target(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f, 0);
    if (mImage.hasImage())
        target << mImage.getCenter().x(), mImage.getCenter().y(), 0;

    ViewController::get()->launch(game, target);
}

// element order need to follow the one in onThemeChanged
std::vector<TextComponent*> DetailedGameListView::getMDLabels() {
    std::vector<TextComponent*> ret;
    ret.push_back(&mLblRating);
    ret.push_back(&mLblReleaseDate);
    ret.push_back(&mLblDeveloper);
    ret.push_back(&mLblPublisher);
    ret.push_back(&mLblGenre);
    ret.push_back(&mLblPlayers);
    ret.push_back(&mLblLastPlayed);
    ret.push_back(&mLblPlayCount);
    if (mSystem->getHasFavorites()) {
        ret.push_back(&mLblFavorite);
    }
    return ret;
}

// element order need to follow the one in onThemeChanged
std::vector<GuiComponent*> DetailedGameListView::getMDValues() {
    std::vector<GuiComponent*> ret;
    ret.push_back(&mRating);
    ret.push_back(&mReleaseDate);
    ret.push_back(&mDeveloper);
    ret.push_back(&mPublisher);
    ret.push_back(&mGenre);
    ret.push_back(&mPlayers);
    ret.push_back(&mLastPlayed);
    ret.push_back(&mPlayCount);
    if (mSystem->getHasFavorites()) {
        ret.push_back(&mFavorite);
    }
    return ret;
}

std::vector<HelpPrompt> DetailedGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	bool hideSystemView = RHGamestationConf::getInstance()->get("emulationstation.hidesystemview") == "1";

	if (Settings::getInstance()->getBool("QuickSystemSelect") && !hideSystemView)
	{
	  prompts.push_back(HelpPrompt("left/right", _("SYSTEM")));
	}
	prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	prompts.push_back(HelpPrompt("b", _("LAUNCH")));
	if ((RHGamestationConf::getInstance()->get("global.netplay") == "1") && (RHGamestationConf::getInstance()->isInList("global.netplay.systems", getCursor()->getSystem()->getName())))
		prompts.push_back(HelpPrompt("x", _("NETPLAY")));
	if(!hideSystemView)
	  prompts.push_back(HelpPrompt("a", _("BACK")));
	if(getRoot()->getSystem() != SystemData::getFavoriteSystem()) {
	  prompts.push_back(HelpPrompt("y", _("Favorite")));
	  prompts.push_back(HelpPrompt("select", _("OPTIONS")));
	}
	return prompts;
}

