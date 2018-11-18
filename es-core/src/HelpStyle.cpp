#include "HelpStyle.h"
#include "ThemeData.h"
#include "Renderer.h"
#include "resources/Font.h"

HelpStyle::HelpStyle()
{
	position = Eigen::Vector2f(Renderer::getScreenWidth() * 0.012f, Renderer::getScreenHeight() * 0.9515f);
	iconColor = 0x777777FF;
	textColor = 0x777777FF;

	if(FONT_SIZE_SMALL != 0)
		font = Font::get(FONT_SIZE_SMALL);
	else
		font = nullptr;
	
	iconMap.clear();
	
}

void HelpStyle::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
	auto elem = theme->getElement(view, "help", "helpsystem");
	if(!elem)
		return;

	if(elem->has("pos"))
		position = elem->get<Eigen::Vector2f>("pos").cwiseProduct(Eigen::Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight()));

	if(elem->has("textColor"))
		textColor = elem->get<unsigned int>("textColor");

	if(elem->has("iconColor"))
		iconColor = elem->get<unsigned int>("iconColor");

	if(elem->has("fontPath") || elem->has("fontSize"))
		font = Font::getFromTheme(elem, ThemeFlags::ALL, font);
	
	if(elem->has("iconUpDown"))
		iconMap["up/down"] = elem->get<std::string>("iconUpDown");
	
	if(elem->has("iconLeftRight"))
		iconMap["left/right"] = elem->get<std::string>("iconLeftRight");
	
	if(elem->has("iconUpDownLeftRight"))
		iconMap["up/down/left/right"] = elem->get<std::string>("iconUpDownLeftRight");
	
	if(elem->has("iconA"))
		iconMap["a"] = elem->get<std::string>("iconA");
	
	if(elem->has("iconB"))
		iconMap["b"] = elem->get<std::string>("iconB");
	
	if(elem->has("iconX"))
		iconMap["x"] = elem->get<std::string>("iconX");
	
	if(elem->has("iconY"))
		iconMap["y"] = elem->get<std::string>("iconY");
	
	if(elem->has("iconL"))
		iconMap["l"] = elem->get<std::string>("iconL");
	
	if(elem->has("iconR"))
		iconMap["r"] = elem->get<std::string>("iconR");
	
	if(elem->has("iconStart"))
		iconMap["start"] = elem->get<std::string>("iconStart");
	
	if(elem->has("iconSelect"))
		iconMap["select"] = elem->get<std::string>("iconSelect");
}
