#include "RHGamestationUpgrade.h"
#include "Settings.h"
#include "Log.h"
#include <Locale.h>
#include <boost/algorithm/string/replace.hpp>


RHGamestationUpgrade *RHGamestationUpgrade::instance = NULL;

RHGamestationUpgrade *RHGamestationUpgrade::getInstance() {
    if (RHGamestationUpgrade::instance == NULL) {
        RHGamestationUpgrade::instance = new RHGamestationUpgrade();
    }
    return RHGamestationUpgrade::instance;
}

std::string RHGamestationUpgrade::getVersion() {
    std::string version = Settings::getInstance()->getString("VersionFile");
    return system->readFile(version);
}

std::string RHGamestationUpgrade::getUpdateVersion() {
    std::pair<std::string, int> res = system->execute(Settings::getInstance()->getString("RHGamestationUpgradeScript") + " canupgrade");
    if(res.second != 0){
        return _("NO UPGRADE AVAILABLE");
    }else {
        res.first.erase(std::remove(res.first.begin(), res.first.end(), '\n'), res.first.end());
        return res.first;
    }
}

bool RHGamestationUpgrade::updateLastChangelogFile() {
    std::ostringstream oss;
    oss << "cp  " << Settings::getInstance()->getString("Changelog").c_str() << " " <<
        Settings::getInstance()->getString("LastChangelog").c_str();
    if (std::system(oss.str().c_str())) {
        LOG(LogWarning) << "Error executing " << oss.str().c_str();
        return false;
    } else {
        LOG(LogError) << "Last version file updated";
        return true;
    }
}


std::string RHGamestationUpgrade::getUpdateChangelog() {
    std::pair<std::string, int> res = system->execute(Settings::getInstance()->getString("RHGamestationUpgradeScript") + " diffremote");
    if(res.second == 0){
        return res.first;
    }else {
        return "";
    }
}

std::string RHGamestationUpgrade::getChangelog() {
    std::pair<std::string, int> res = system->execute(Settings::getInstance()->getString("RHGamestationUpgradeScript") + " difflocal");
    if(res.second == 0){
        return res.first;
    }else {
        return "";
    }
}

std::pair<std::string, int> RHGamestationUpgrade::updateSystem(BusyComponent* ui) {
    std::string updatecommand = Settings::getInstance()->getString("RHGamestationUpgradeScript") + " upgrade";
    FILE *pipe = popen(updatecommand.c_str(), "r");
    char line[1024] = "";
    if (pipe == NULL) {
        return std::pair<std::string, int>(std::string("Cannot call upgrade command"), -1);
    }
    while (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        std::string output = line;
        boost::replace_all(output, "\e[1A\e[K", "");
		if (output.find(':') != std::string::npos) {

			int p1 = output.find(":");
			std::string i18n = output.substr(0, p1);
			std::string percent = output.substr(p1 + 1, std::string::npos);
			ui->setText(_(i18n.c_str()) + ": " + percent);
		} else
			ui->setText(_(output.c_str()));
    }

    int exitCode = pclose(pipe);
    return std::pair<std::string, int>(std::string(line), exitCode);
}

bool RHGamestationUpgrade::canUpdate() {

    std::pair<std::string, int> res = system->execute(Settings::getInstance()->getString("RHGamestationUpgradeScript") + " canupgrade");
    if(res.second == 0){
        LOG(LogInfo) << "Can upgrade";
        return true;
    }else {
        LOG(LogInfo) << "Cannot upgrade";
        return false;
    }
}
