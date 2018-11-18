#include <RHGamestationConf.h>
#include "GuiGamelistOptions.h"
#include "GuiMetaDataEd.h"
#include "Settings.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "components/SwitchComponent.h"
#include "guis/GuiSettings.h"
#include "Locale.h"
#include "MenuMessages.h"
#include "guis/GuiMsgBox.h"

GuiGamelistOptions::GuiGamelistOptions(Window* window, SystemData* system) :
		GuiComponent(window), mSystem(system), mReloading(false), mMenu(window, _("OPTIONS").c_str()) {
	ComponentListRow row;

	auto menuTheme = MenuThemeData::getInstance()->getCurrentTheme();
	addChild(&mMenu);

	// jump to letter
	auto curChar = (char) toupper(getGamelist()->getCursor()->getName()[0]);

	mJumpToLetterList = std::make_shared<LetterList>(mWindow, _("JUMP TO LETTER"), false);

	std::vector<std::string> letters = getAvailableLetters();
	if (!letters.empty()) { // should not arrive

		// if curChar not found in available letter, take first one
		if (std::find(letters.begin(), letters.end(), std::string(1, curChar)) == letters.end()) {
			curChar = letters.at(0)[0];
		}

		for (auto letter : letters) {
			mJumpToLetterList->add(letter, letter[0], letter[0] == curChar);
		}

		row.addElement(std::make_shared<TextComponent>(mWindow, _("JUMP TO LETTER"), menuTheme->menuText.font,
													   menuTheme->menuText.color), true);
		row.addElement(mJumpToLetterList, false);
		row.input_handler = [&](InputConfig *config, Input input) {
			if (config->isMappedTo("b", input) && input.value) {
				jumpToLetter();
				return true;
			} else if (mJumpToLetterList->input(config, input)) {
				return true;
			}
			return false;
		};
		mMenu.addRowWithHelp(row, _("JUMP TO LETTER"), _(MenuMessages::GAMELISTOPTION_JUMP_LETTER_MSG));
	}

	// sort list by
	unsigned int currentSortId = mSystem->getSortId();
	if (currentSortId > FileSorts::SortTypes.size()) {
		currentSortId = 0;
	}
	mListSort = std::make_shared<SortList>(mWindow, _("SORT GAMES BY"), false);
	for (unsigned int i = 0; i < FileSorts::SortTypes.size(); i++) {
		const FileData::SortType& sortType = FileSorts::SortTypes.at(i);
		mListSort->add(sortType.description, i, i == currentSortId);
	}

	mMenu.addWithLabel(mListSort, _("SORT GAMES BY"), _(MenuMessages::GAMELISTOPTION_SORT_GAMES_MSG));
	addSaveFunc([this, system] { RHGamestationConf::getInstance()->setUInt(system->getName() + ".sort", (unsigned int) mListSort->getSelected()); });

	// favorite only
	auto favorite_only = std::make_shared<SwitchComponent>(mWindow);
	favorite_only->setState(Settings::getInstance()->getBool("FavoritesOnly"));
	mMenu.addWithLabel(favorite_only, _("FAVORITES ONLY"), _(MenuMessages::GAMELISTOPTION_FAVORITES_ONLY_MSG));
	addSaveFunc([favorite_only] { Settings::getInstance()->setBool("FavoritesOnly", favorite_only->getState()); });

	// show hidden
	auto show_hidden = std::make_shared<SwitchComponent>(mWindow);
	show_hidden->setState(Settings::getInstance()->getBool("ShowHidden"));
	mMenu.addWithLabel(show_hidden, _("SHOW HIDDEN"), _(MenuMessages::GAMELISTOPTION_SHOW_HIDDEN_MSG));
	addSaveFunc([show_hidden] { Settings::getInstance()->setBool("ShowHidden", show_hidden->getState()); });

	// flat folders
	auto flat_folders = std::make_shared<SwitchComponent>(mWindow);
	flat_folders->setState(RHGamestationConf::getInstance()->getBool(system->getName() + ".flatfolder"));
	mMenu.addWithLabel(flat_folders, _("SHOW FOLDERS CONTENT"), _(MenuMessages::GAMELISTOPTION_SHOW_FOLDER_CONTENT_MSG));
	addSaveFunc([flat_folders, system] { RHGamestationConf::getInstance()->setBool(system->getName() + ".flatfolder", flat_folders->getState()); });

	// edit game metadata
	row.elements.clear();

	if (RHGamestationConf::getInstance()->get("emulationstation.menu") != "none" && RHGamestationConf::getInstance()->get("emulationstation.menu") != "bartop"){
		row.addElement(std::make_shared<TextComponent>(mWindow, _("EDIT THIS GAME'S METADATA"), menuTheme->menuText.font, menuTheme->menuText.color), true);
		row.addElement(makeArrow(mWindow), false);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::openMetaDataEd, this));
		mMenu.addRowWithHelp(row, _("EDIT THIS GAME'S METADATA"), _(MenuMessages::GAMELISTOPTION_EDIT_METADATA_MSG));
	}

	// update game list
	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, _("UPDATE GAMES LISTS"), menuTheme->menuText.font, menuTheme->menuText.color), true);
	row.addElement(makeArrow(mWindow), false);
	row.makeAcceptInputHandler([this, system, window] {
		mReloading = true;
		window->pushGui(new GuiMsgBox(window, _("REALLY UPDATE GAMES LISTS ?"),
									  _("YES"), [system, window] {
					std::string systemName = system->getName();
					ViewController::get()->goToStart();
					window->renderShutdownScreen();
					delete ViewController::get();
					SystemData::deleteSystems();
					SystemData::loadConfig();
					window->deleteAllGui();
					ViewController::init(window);
					ViewController::get()->reloadAll();
					window->pushGui(ViewController::get());
					if (!ViewController::get()->goToGameList(systemName)) {
						ViewController::get()->goToStart();
					}
				},
									  _("NO"), [this] {
					mReloading = false;
				}
		));
	});
	mMenu.addRowWithHelp(row, _("UPDATE GAMES LISTS"), _(MenuMessages::UI_UPDATE_GAMELIST_HELP_MSG));

	// center the menu
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, (mSize.y() - mMenu.getSize().y()) / 2);

	mFavoriteState = Settings::getInstance()->getBool("FavoritesOnly");
	mHiddenState = Settings::getInstance()->getBool("ShowHidden");
}

GuiGamelistOptions::~GuiGamelistOptions() {
	if (mReloading) {
		return ;
	}
	FileData* root = getGamelist()->getRoot();
	if (root->getDisplayableRecursive(GAME | FOLDER).size()) {


		if (mListSort->getSelected() != mSystem->getSortId()) {
			// apply sort
			mSystem->setSortId(mListSort->getSelected());

			// notify that the root folder has to be sorted
			getGamelist()->onFileChanged(mSystem->getRootFolder(), FILE_SORTED);
		} else {

			// require list refresh
			getGamelist()->onFileChanged(mSystem->getRootFolder(), FILE_DISPLAY_UPDATED);
		}

		// if states has changed, invalidate and reload game list
		if (Settings::getInstance()->getBool("FavoritesOnly") != mFavoriteState || Settings::getInstance()->getBool("ShowHidden") != mHiddenState) {
			ViewController::get()->setAllInvalidGamesList(getGamelist()->getCursor()->getSystem());
			ViewController::get()->reloadGameListView(getGamelist()->getCursor()->getSystem());
		}
	} else {
		Window *window = mWindow;
		if (root->getChildren().size() == 0) {
			ViewController::get()->goToStart();
			window->renderShutdownScreen();
			delete ViewController::get();
			SystemData::deleteSystems();
			SystemData::loadConfig();
			GuiComponent *gui;
			while ((gui = window->peekGui()) != NULL) {
				window->removeGui(gui);
			}
			ViewController::init(window);
			ViewController::get()->reloadAll();
			window->pushGui(ViewController::get());
			ViewController::get()->goToStart();
		}
	}
}

void GuiGamelistOptions::openMetaDataEd() {
	// open metadata editor
	FileData* file = getGamelist()->getCursor();
	ScraperSearchParams p;
	p.game = file;
	p.system = file->getSystem();
	mWindow->pushGui(new GuiMetaDataEd(mWindow, &file->metadata, file->metadata.getMDD(), p, file->getPath().filename().string(),
									   std::bind(&IGameListView::onFileChanged, getGamelist(), file, FILE_METADATA_CHANGED), [this, file] {
				boost::filesystem::remove(file->getPath()); //actually delete the file on the filesystem
				file->getParent()->removeChild(file); //unlink it so list repopulations triggered from onFileChanged won't see it
				getGamelist()->onFileChanged(file, FILE_REMOVED); //tell the view

			},file->getSystem(), true));
}

std::vector<std::string> GuiGamelistOptions::getAvailableLetters() {
	std::vector<std::string> letters;
	std::vector<FileData*>files = getGamelist()->getFileDataList();
	for (auto file : files) {
		std::string letter = std::string(1, toupper(file->getName()[0]));
		if ( ( (letter[0] >= '0' && letter[0] <= '9') || (letter[0] >= 'A' && letter[0] <= 'Z') ) ) {
			if (std::find(letters.begin(), letters.end(), letter) == letters.end()) {
				letters.push_back(letter);
			}
		}
	}
	std::sort(letters.begin(), letters.end());
	return letters;
}

void GuiGamelistOptions::jumpToLetter() {
	char letter = mJumpToLetterList->getSelected();
	auto sortId = (unsigned int) mListSort->getSelected();
	IGameListView* gamelist = getGamelist();

	// if sort is not alpha, need to force an alpha
	if (sortId > 1) {
		sortId = 0; // Alpha ASC
		// select to avoid resort on destroy
		mListSort->select(sortId);
	}

	if (sortId != mSystem->getSortId()) {
		// apply sort
		mSystem->setSortId(sortId);
		// notify that the root folder has to be sorted
		gamelist->onFileChanged(mSystem->getRootFolder(), FILE_SORTED);
	}

	std::vector<FileData*>files = gamelist->getFileDataList();

	long min = 0;
	long max = files.size() - 1;
	long mid = 0;

	if (sortId == 1) {
		// sort Alpha DESC
		std::reverse(files.begin(), files.end());
	}

	while(max >= min) {
		mid = ((max - min) / 2) + min;

		// game somehow has no first character to check
		if (files.at(mid)->getName().empty()) {
			continue;
		}

		char checkLetter = (char) toupper(files.at(mid)->getName()[0]);

		if (checkLetter < letter) {
			min = mid + 1;
		} else if (checkLetter > letter || (mid > 0 && (letter == toupper(files.at(mid - 1)->getName()[0])))) {
			max = mid - 1;
		} else {
			break; //exact match found
		}
	}

	gamelist->setCursor(files.at(mid));
	delete this;
}

bool GuiGamelistOptions::input(InputConfig* config, Input input) {
	if ((config->isMappedTo("a", input) || config->isMappedTo("select", input)) && input.value) {
		save();
		delete this;
		return true;
	}
	return mMenu.input(config, input);
}

std::vector<HelpPrompt> GuiGamelistOptions::getHelpPrompts() {
	auto prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("a", _("CLOSE")));
	return prompts;
}

IGameListView* GuiGamelistOptions::getGamelist() {
	return ViewController::get()->getGameListView(mSystem).get();
}

void GuiGamelistOptions::save() {
	if (mSaveFuncs.empty()) {
		return;
	}
	for (auto it = mSaveFuncs.begin(); it != mSaveFuncs.end(); it++) {
		(*it)();
	}
	Settings::getInstance()->saveFile();
	RHGamestationConf::getInstance()->saveRHGamestationConf();
}
