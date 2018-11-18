#include "guis/GuiMetaDataEd.h"
#include "Renderer.h"
#include "Log.h"
#include "components/AsyncReqComponent.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "guis/GuiGameScraper.h"
#include "guis/GuiMsgBox.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <RHGamestationConf.h>
#include <components/SwitchComponent.h>
#include <LibretroRatio.h>

#include "components/TextEditComponent.h"
#include "components/DateTimeComponent.h"
#include "components/RatingComponent.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "MenuThemeData.h"
#include "components/OptionListComponent.h"
#include "rhgamestation/RHGamestationSystem.h"


using namespace Eigen;

GuiMetaDataEd::GuiMetaDataEd(Window *window, MetaDataList *md, const std::vector<MetaDataDecl> &mdd,
                             ScraperSearchParams scraperParams, const std::string &header, 
                             std::function<void()> saveCallback, std::function<void()> deleteFunc, 
                             SystemData *system, bool main) :   GuiComponent(window),
                                                                mScraperParams(scraperParams),
                                                                mBackground(window, ":/frame.png"),
                                                                mGrid(window, Vector2i(1, 3)),
                                                                mMetaDataDecl(mdd),
                                                                mMetaData(md),
                                                                mSavedCallback(saveCallback),
                                                                mDeleteFunc(deleteFunc) {
    addChild(&mBackground);
    addChild(&mGrid);

    mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 5));
	
	auto menuTheme = MenuThemeData::getInstance()->getCurrentTheme();
	
	mBackground.setImagePath(menuTheme->menuBackground.path);
	mBackground.setCenterColor(menuTheme->menuBackground.color);
	mBackground.setEdgeColor(menuTheme->menuBackground.color);

    mTitle = std::make_shared<TextComponent>(mWindow, _("EDIT METADATA"), menuTheme->menuTitle.font, menuTheme->menuTitle.color,
                                             ALIGN_CENTER);
    mSubtitle = std::make_shared<TextComponent>(mWindow,
                                                strToUpper(scraperParams.game->getPath().filename().generic_string()),
                                                menuTheme->menuFooter.font, menuTheme->menuFooter.color, ALIGN_CENTER);
    float y = 0;
    y += mTitle->getFont()->getHeight() + mSubtitle->getFont()->getHeight();
    mHeaderGrid->setEntry(mTitle, Vector2i(0, 1), false, true);
    mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 3), false, true);

    mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

    mList = std::make_shared<ComponentList>(mWindow);
    mGrid.setEntry(mList, Vector2i(0, 1), true, true);

    EmulatorDefaults emulatorDefaults = RHGamestationSystem::getInstance()->getEmulatorDefaults(system->getName());

    auto emu_choice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("Emulator"), false, FONT_SIZE_MEDIUM);
    auto core_choice = std::make_shared<OptionListComponent<std::string> >(mWindow, _("core"), false, FONT_SIZE_MEDIUM);

    // populate list
    for (auto iter = mMetaDataDecl.begin(); iter != mMetaDataDecl.end(); iter++) {
        std::shared_ptr<GuiComponent> ed;

        // don't add statistics
        if (iter->isStatistic)
            continue;

        // filter on main or secondary entries depending on the requested type
        if (iter->isMain != main)
            continue;

        // create ed and add it (and any related components) to mMenu
        // ed's value will be set below
        ComponentListRow row;
        auto lbl = std::make_shared<TextComponent>(mWindow, strToUpper(iter->displayName), menuTheme->menuText.font, menuTheme->menuText.color);

        row.addElement(lbl, true); // label
        y += lbl->getFont()->getHeight();

        switch (iter->type) {
            case MD_RATING: {
                ed = std::make_shared<RatingComponent>(window, menuTheme->menuText.color);

                ed->setSize(0, lbl->getSize().y() * 0.8f);
                row.addElement(ed, false, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
                row.addElement(spacer, false);

                // pass input to the actual RatingComponent instead of the spacer
                row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1, std::placeholders::_2);

                break;
            }
            case MD_DATE: {
                ed = std::make_shared<DateTimeComponent>(mWindow);
                row.addElement(ed, false);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.0025f, 0);
                row.addElement(spacer, false);

                // pass input to the actual DateTimeComponent instead of the spacer
                row.input_handler = std::bind(&GuiComponent::input, ed.get(), std::placeholders::_1, std::placeholders::_2);

                break;
            }
            case MD_BOOL: {
                auto switchComp = std::make_shared<SwitchComponent>(mWindow);
                switchComp->setState(mMetaData->get(iter->key) == "true");
                ed = switchComp;
                row.addElement(ed, false);
                break;
            }
            case MD_LIST:
                if (iter->key == "emulator") {
                    row.addElement(emu_choice, false);
                    std::string currentEmulator = mMetaData->get(iter->key);
                    std::string mainConfigEmulator = RHGamestationConf::getInstance()->get(system->getName() + ".emulator");

                    if (mainConfigEmulator == "" || mainConfigEmulator == "default") {
                        mainConfigEmulator = emulatorDefaults.emulator;
                    }

                    emu_choice->add(str(boost::format(_("DEFAULT (%1%)")) % mainConfigEmulator), "default", true);

                    for (auto it = system->getEmulators()->begin(); it != system->getEmulators()->end(); it++) {
                        emu_choice->add(it->first,  it->first, it->first == currentEmulator);
                    }

                    // when emulator changes, load new core list
                    emu_choice->setSelectedChangedCallback([this, system, emulatorDefaults, core_choice](std::string emulatorName) {
                        core_choice->clear();

                        if (emulatorName == "default") {
                            std::string mainConfigCore = RHGamestationConf::getInstance()->get(system->getName() + ".core");
                            if (mainConfigCore == "" || mainConfigCore == "default") {
                                mainConfigCore = emulatorDefaults.core;
                            }
                            core_choice->add(str(boost::format(_("DEFAULT (%1%)")) % mainConfigCore), "default", true);
                            return ;
                        }
                        
                        std::vector<std::string> cores = system->getCores(emulatorName);
                        std::string currentCore = mMetaData->get("core");

                        if (currentCore == "") {
                            currentCore = "default";
                        }

                        // update current core if it is not available in the emulator selected core list
                        if (currentCore != "default" && std::find(cores.begin(), cores.end(), currentCore) == cores.end()) {
                            if (emulatorName == emulatorDefaults.emulator) {
                                currentCore = emulatorDefaults.core;
                            } else {
                                // use first one
                                currentCore = *(cores.begin());
                            }
                        }

                        for (auto it = cores.begin(); it != cores.end(); it++) {
                            std::string core = *it;
                            // select at least first one in case of bad entry in config file
                            bool selected = currentCore == core || it == cores.begin();
                            if (currentCore == "default" && emulatorName == emulatorDefaults.emulator) {
                                selected = selected || core == emulatorDefaults.core;
                            }
                            core_choice->add(core, core, selected);
                        }
                    });

                    ed = emu_choice;
                }

                if (iter->key == "core") {
                    row.addElement(core_choice, false);
                    ed = core_choice;

                    // force change event to load core list
                    emu_choice->invalidate(); 
                }

                if (iter->key == "ratio") {
                    auto ratio_choice = std::make_shared<OptionListComponent<std::string> >(mWindow, "ratio", false, FONT_SIZE_MEDIUM);
                    row.addElement(ratio_choice, false);
                    std::map<std::string, std::string> *ratioMap = LibretroRatio::getInstance()->getRatio();
                    if (mMetaData->get("ratio") == "") {
                        mMetaData->set("ratio", "auto");
                    }
                    for (auto ratio = ratioMap->begin(); ratio != ratioMap->end(); ratio++) {
                        ratio_choice->add(ratio->first, ratio->second, mMetaData->get("ratio") == ratio->second);
                    }
                    ed = ratio_choice;
                }
                break;
            case MD_MULTILINE_STRING:
            default: {
                // MD_STRING
                ed = std::make_shared<TextComponent>(window, "", menuTheme->menuText.font, menuTheme->menuText.color, ALIGN_RIGHT);
                row.addElement(ed, true);

                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
                row.addElement(spacer, false);

                auto bracket = std::make_shared<ImageComponent>(mWindow);
	
				bracket->setImage(menuTheme->iconSet.arrow);
				bracket->setColorShift(menuTheme->menuText.color);
                bracket->setResize(Eigen::Vector2f(0, lbl->getFont()->getLetterHeight()));
                row.addElement(bracket, false);

                bool multiLine = iter->type == MD_MULTILINE_STRING;
                const std::string title = iter->displayPrompt;
                auto updateVal = [ed](const std::string &newVal) {
                    ed->setValue(newVal);
                }; // ok callback (apply new value to ed)
                row.makeAcceptInputHandler([this, title, ed, updateVal, multiLine] {
                    if (Settings::getInstance()->getBool("UseOSK") && (!multiLine))
                        mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, title, ed->getValue(), updateVal, multiLine));
                    else
                        mWindow->pushGui(new GuiTextEditPopup(mWindow, title, ed->getValue(), updateVal, multiLine));
                });
                break;
            }
        }

        assert(ed);
        mList->addRow(row);
        ed->setValue(mMetaData->get(iter->key));
        mEditors.push_back(ed);
        mMetaDataEditable.push_back(*iter);
    }
    
    std::vector<std::shared_ptr<ButtonComponent> > buttons;

    if (main)
    {
        // append a shortcut to the secondary menu
        ComponentListRow row;
        auto lbl = std::make_shared<TextComponent>(mWindow, _("MORE DETAILS"), menuTheme->menuText.font, menuTheme->menuText.color);
        row.addElement(lbl, true);
        y += lbl->getFont()->getHeight();

        auto bracket = std::make_shared<ImageComponent>(mWindow);
        bracket->setImage(menuTheme->iconSet.arrow);
        bracket->setColorShift(menuTheme->menuText.color);

        bracket->setResize(Eigen::Vector2f(0, lbl->getFont()->getLetterHeight()));
        row.addElement(bracket, false);
        
        row.makeAcceptInputHandler([this, header, system] {
            // call the same Gui with "main" set to "false"
            mWindow->pushGui(new GuiMetaDataEd(mWindow, mMetaData, mMetaDataDecl, mScraperParams, header, nullptr, nullptr, system, false));
         });

        mList->addRow(row);
    }


    if (main && !scraperParams.system->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SCRAPE"), _("SCRAPE"), std::bind(&GuiMetaDataEd::fetch, this)));

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SAVE"), _("SAVE"), [&] { save(); delete this; }));

    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("CANCEL"), [&] { delete this; }));

    if (main && mDeleteFunc) {
        auto deleteFileAndSelf = [&] {
            mDeleteFunc();
            delete this;
        };
        auto deleteBtnFunc = [this, deleteFileAndSelf] {
            mWindow->pushGui(new GuiMsgBox(mWindow, _("THIS WILL DELETE A FILE!\nARE YOU SURE?"), _("YES"), deleteFileAndSelf, _("NO"), nullptr));
        };
        buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("DELETE"), _("DELETE"), deleteBtnFunc));
    }


    mButtons = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtons, Vector2i(0, 2), true, false);

    mGrid.setUnhandledInputCallback([this] (InputConfig* config, Input input) -> bool {
        if (config->isMappedTo("down", input)) {
            mGrid.setCursorTo(mList);
            mList->setCursorIndex(0);
            return true;
        }
        if(config->isMappedTo("up", input)) {
            mList->setCursorIndex(mList->size() - 1);
            mGrid.moveCursor(Vector2i(0, 1));
            return true;
        }
        return false;
    });

    // resize + center
    y /= Renderer::getScreenHeight();
    setSize(Renderer::getScreenWidth() * 0.7f, Renderer::getScreenHeight() * (y + 0.15f));
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiMetaDataEd::onSizeChanged() {
    mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

    mGrid.setSize(mSize);

    const float titleHeight = mTitle->getFont()->getLetterHeight();
    const float subtitleHeight = mSubtitle->getFont()->getLetterHeight();
    const float titleSubtitleSpacing = mSize.y() * 0.03f;

    mGrid.setRowHeightPerc(0, (titleHeight + titleSubtitleSpacing + subtitleHeight + TITLE_VERT_PADDING) / mSize.y());
    mGrid.setRowHeightPerc(2, mButtons->getSize().y() / mSize.y());

    mHeaderGrid->setRowHeightPerc(1, titleHeight / mHeaderGrid->getSize().y());
    mHeaderGrid->setRowHeightPerc(2, titleSubtitleSpacing / mHeaderGrid->getSize().y());
    mHeaderGrid->setRowHeightPerc(3, subtitleHeight / mHeaderGrid->getSize().y());
}

void GuiMetaDataEd::save() {
    for (unsigned int i = 0; i < mEditors.size(); i++) {

        if (mMetaDataEditable.at(i).type != MD_LIST)
            mMetaData->set(mMetaDataEditable.at(i).key, mEditors.at(i)->getValue());
        else {
            std::shared_ptr<GuiComponent> ed = mEditors.at(i);
            std::shared_ptr<OptionListComponent<std::string>> list = std::static_pointer_cast<OptionListComponent<std::string>>(ed);
            mMetaData->set(mMetaDataEditable.at(i).key, list->getSelected());
        }
    }

    if (mSavedCallback)
        mSavedCallback();
}

void GuiMetaDataEd::fetch() {
    GuiGameScraper *scr = new GuiGameScraper(mWindow, mScraperParams,
                                             std::bind(&GuiMetaDataEd::fetchDone, this, std::placeholders::_1));
    mWindow->pushGui(scr);
}

void GuiMetaDataEd::fetchDone(const ScraperSearchResult &result) {
    mMetaData->merge(result.mdl);
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string &key = mMetaDataEditable.at(i).key;
        mEditors.at(i)->setValue(mMetaData->get(key));
    }
}

void GuiMetaDataEd::close(bool closeAllWindows) {
    // find out if the user made any changes
    bool dirty = false;
    for (unsigned int i = 0; i < mEditors.size(); i++) {
        const std::string &key = mMetaDataEditable.at(i).key;
        if (mMetaData->get(key) != mEditors.at(i)->getValue()) {
            dirty = true;
            break;
        }
    }

    std::function<void()> closeFunc;
    if (!closeAllWindows) {
        closeFunc = [this] { delete this; };
    } else {
        Window *window = mWindow;
        closeFunc = [window, this] {
            while (window->peekGui() != ViewController::get())
                delete window->peekGui();
        };
    }


    if (dirty) {
        // changes were made, ask if the user wants to save them
        mWindow->pushGui(new GuiMsgBox(mWindow,
                                       _("SAVE CHANGES?"),
                                       _("YES"), [this, closeFunc] {
                    save();
                    closeFunc();
                },
                                       _("NO"), closeFunc
        ));
    } else {
        closeFunc();
    }
}

bool GuiMetaDataEd::input(InputConfig *config, Input input) {
    if (GuiComponent::input(config, input))
        return true;

    const bool isStart = config->isMappedTo("start", input);
    if (input.value != 0 && (config->isMappedTo("a", input) || isStart)) {
        close(isStart);
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiMetaDataEd::getHelpPrompts() {
    std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
    prompts.push_back(HelpPrompt("a", _("BACK")));
    prompts.push_back(HelpPrompt("start", _("CLOSE")));
    return prompts;
}
