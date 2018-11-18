/*
 * File:   RHGamestationSystem.cpp
 * Author: digitallumberjack
 * 
 * Created on 29 novembre 2014, 03:1
 */

#include "RHGamestationConf.h"
#include "RHGamestationSystem.h"
#include <sys/statvfs.h>
#include "Settings.h"
#include "Log.h"
#include "HttpReq.h"

#include "AudioManager.h"
#include "VolumeControl.h"

#include <ifaddrs.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <Locale.h>


RHGamestationSystem::RHGamestationSystem() {
}

RHGamestationSystem *RHGamestationSystem::instance = NULL;


RHGamestationSystem *RHGamestationSystem::getInstance() {
    if (RHGamestationSystem::instance == NULL) {
        RHGamestationSystem::instance = new RHGamestationSystem();
    }
    return RHGamestationSystem::instance;
}

unsigned long RHGamestationSystem::getFreeSpaceGB(std::string mountpoint) {
    struct statvfs fiData;
    const char *fnPath = mountpoint.c_str();
    int free = 0;
    if ((statvfs(fnPath, &fiData)) >= 0) {
        free = (fiData.f_bfree * fiData.f_bsize) / (1024 * 1024 * 1024);
    }
    return free;
}

std::string RHGamestationSystem::getFreeSpaceInfo() {
    struct statvfs fiData;
    std::string sharePart = Settings::getInstance()->getString("SharePartition");
    if (sharePart.size() > 0) {
        const char *fnPath = sharePart.c_str();
        if ((statvfs(fnPath, &fiData)) < 0) {
            return "";
        } else {
            unsigned long total = (fiData.f_blocks * (fiData.f_bsize / 1024)) / (1024L * 1024L);
            unsigned long free = (fiData.f_bfree * (fiData.f_bsize / 1024)) / (1024L * 1024L);
            unsigned long used = total - free;
            unsigned long percent = 0;
            std::ostringstream oss;
            if (total != 0) {  //for small SD card ;) with share < 1GB
                percent = used * 100 / total;
                oss << used << "GB/" << total << "GB (" << percent << "%)";
            } else
                oss << "N/A";
            return oss.str();
        }
    } else {
        return "ERROR";
    }
}

bool RHGamestationSystem::isFreeSpaceLimit() {
    std::string sharePart = Settings::getInstance()->getString("SharePartition");
    if (sharePart.size() > 0) {
        return getFreeSpaceGB(sharePart) < 2;
    } else {
        return "ERROR";
    }

}

std::string RHGamestationSystem::readFile(std::string file) {
    if (file.size() > 0) {
        std::ifstream ifs(file);

        if (ifs.good()) {
            std::string contents;
            std::getline(ifs, contents);
            return contents;
        }
    }
    return "";
}

std::vector<std::string> RHGamestationSystem::getAvailableWiFiSSID(bool activatedWifi) {
	if (!activatedWifi){
		enableWifi("","");
	}
	std::vector<std::string> res;
	std::ostringstream oss;
	oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "wifi list";
	FILE *pipe = popen(oss.str().c_str(), "r");
	char line[1024];

	if (pipe == NULL) {
		return res;
	}

	while (fgets(line, 1024, pipe)) {
		strtok(line, "\n");
		res.push_back(std::string(line));
	}
	pclose(pipe);
	return res;
}

std::vector<std::string> RHGamestationSystem::getAvailableAudioOutputDevices() {
	std::vector<std::string> res;
	std::ostringstream oss;
	oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "lsaudio";
	FILE *pipe = popen(oss.str().c_str(), "r");
	char line[1024];

	if (pipe == NULL) {
		return res;
	}

	while (fgets(line, 1024, pipe)) {
		strtok(line, "\n");
		res.push_back(std::string(line));
	}
	pclose(pipe);

	return res;
}

std::string RHGamestationSystem::getCurrentAudioOutputDevice() {

	std::ostringstream oss;
	oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "getaudio";
	FILE *pipe = popen(oss.str().c_str(), "r");
	char line[1024];

	if (pipe == NULL) {
		return "";
	}

	if (fgets(line, 1024, pipe)) {
		strtok(line, "\n");
		pclose(pipe);
		return std::string(line);
	}
	return "auto";
}

bool RHGamestationSystem::setAudioOutputDevice(std::string selected) {
	std::ostringstream oss;

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

	oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "audio" << " '" << selected << "'";
	int exitcode = system(oss.str().c_str());
	if (selected.find('[') != std::string::npos)
	{
		int p1 = selected.find(":");
		int p2 = selected.find("]");
		std::string acard = selected.substr(1, p1 - 1);
		std::string adevice = selected.substr(p1 + 1, p2 - p1 - 1);
		std::string alsaAudioDev = "hw:" + acard + "," + adevice;
		setenv("AUDIODEV",alsaAudioDev.c_str(),1);
	}
	else
		setenv("AUDIODEV","hw:0,0",1);
	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->resumeMusic();
	//AudioManager::getInstance()->playCheckSound();

	return exitcode == 0;
}

bool RHGamestationSystem::setOverscan(bool enable) {

    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "overscan";
    if (enable) {
        oss << " " << "enable";
    } else {
        oss << " " << "disable";
    }
    std::string command = oss.str();
    LOG(LogInfo) << "Launching " << command;
    if (system(command.c_str())) {
        LOG(LogWarning) << "Error executing " << command;
        return false;
    } else {
        LOG(LogInfo) << "Overscan set to : " << enable;
        return true;
    }

}

bool RHGamestationSystem::setOverclock(std::string mode) {
    if (mode != "") {
        std::ostringstream oss;
        oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " "
            << "overclock" << " " << mode;
        std::string command = oss.str();
        LOG(LogInfo) << "Launching " << command;
        if (system(command.c_str())) {
            LOG(LogWarning) << "Error executing " << command;
            return false;
        } else {
            LOG(LogInfo) << "Overclocking set to " << mode;
            return true;
        }
    }

    return false;
}

bool RHGamestationSystem::launchKodi(Window *window) {

    LOG(LogInfo) << "Attempting to launch kodi...";


    AudioManager::getInstance()->deinit();
    VolumeControl::getInstance()->deinit();

    std::string commandline = InputManager::getInstance()->configureEmulators();
    std::string command = "configgen -system kodi -rom '' " + commandline;

    window->deinit();

    int exitCode = system(command.c_str());
    if (WIFEXITED(exitCode)) {
        exitCode = WEXITSTATUS(exitCode);
    }

    window->init();
    VolumeControl::getInstance()->init();
    AudioManager::getInstance()->resumeMusic();
    window->normalizeNextUpdate();

    // handle end of kodi
    switch (exitCode) {
        case 10: // reboot code
            reboot();
            return true;
            break;
        case 11: // shutdown code
            shutdown();
            return true;
            break;
    }

    return exitCode == 0;

}

bool RHGamestationSystem::enableWifi(std::string ssid, std::string key) {
    std::ostringstream oss;
    boost::replace_all(ssid, "\"", "\\\"");
    boost::replace_all(key, "\"", "\\\"");
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " "
        << "wifi" << " "
        << "enable" << " \""
        << ssid << "\" \"" << key << "\"";
    std::string command = oss.str();
    LOG(LogInfo) << "Launching " << command;
    if (system(command.c_str()) == 0) {
        LOG(LogInfo) << "Wifi enabled ";
        return true;
    } else {
        LOG(LogInfo) << "Cannot enable wifi ";
        return false;
    }
}

bool RHGamestationSystem::disableWifi() {
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " "
        << "wifi" << " "
        << "disable";
    std::string command = oss.str();
    LOG(LogInfo) << "Launching " << command;
    if (system(command.c_str()) == 0) {
        LOG(LogInfo) << "Wifi disabled ";
        return true;
    } else {
        LOG(LogInfo) << "Cannot disable wifi ";
        return false;
    }
}


bool RHGamestationSystem::halt(bool reboot, bool fast) {
    SDL_Event *quit = new SDL_Event();
    if (fast)
        if (reboot)
            quit->type = SDL_FAST_QUIT | SDL_RB_REBOOT;
        else
            quit->type = SDL_FAST_QUIT | SDL_RB_SHUTDOWN;
    else if (reboot)
        quit->type = SDL_QUIT | SDL_RB_REBOOT;
    else
        quit->type = SDL_QUIT | SDL_RB_SHUTDOWN;
    SDL_PushEvent(quit);
    return 0;
}

bool RHGamestationSystem::reboot() {
    return halt(true, false);
}

bool RHGamestationSystem::fastReboot() {
    return halt(true, true);
}

bool RHGamestationSystem::shutdown() {
    return halt(false, false);
}

bool RHGamestationSystem::fastShutdown() {
    return halt(false, true);
}


std::string RHGamestationSystem::getIpAdress() {
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;

    std::string result = "NOT CONNECTED";
    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            if (std::string(ifa->ifa_name).find("eth") != std::string::npos ||
                std::string(ifa->ifa_name).find("wlan") != std::string::npos) {
                result = std::string(addressBuffer);
            }
        }
    }
    // Seeking for ipv6 if no IPV4
    if (result.compare("NOT CONNECTED") == 0) {
        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) {
                continue;
            }
            if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
                // is a valid IP6 Address
                tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
                char addressBuffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
                if (std::string(ifa->ifa_name).find("eth") != std::string::npos ||
                    std::string(ifa->ifa_name).find("wlan") != std::string::npos) {
                    return std::string(addressBuffer);
                }
            }
        }
    }
    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
    return result;
}

std::vector<std::string> *RHGamestationSystem::scanBluetooth() {
    std::vector<std::string> *res = new std::vector<std::string>();
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "hcitoolscan";
    FILE *pipe = popen(oss.str().c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return NULL;
    }

    while (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        res->push_back(std::string(line));
    }
    pclose(pipe);

    return res;
}

bool RHGamestationSystem::pairBluetooth(std::string &controller) {
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "hiddpair" << " " << controller;
    int exitcode = system(oss.str().c_str());
    return exitcode == 0;
}

std::vector<std::string> RHGamestationSystem::getAvailableStorageDevices() {

    std::vector<std::string> res;
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "storage list";
    FILE *pipe = popen(oss.str().c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return res;
    }

    while (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        res.push_back(std::string(line));
    }
    pclose(pipe);

    return res;
}

std::string RHGamestationSystem::getCurrentStorage() {

    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "storage current";
    FILE *pipe = popen(oss.str().c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return "";
    }

    if (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        pclose(pipe);
        return std::string(line);
    }
    return "INTERNAL";
}

bool RHGamestationSystem::setStorage(std::string selected) {
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "storage" << " " << selected;
    int exitcode = system(oss.str().c_str());
    return exitcode == 0;
}

bool RHGamestationSystem::forgetBluetoothControllers() {
    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "forgetBT";
    int exitcode = system(oss.str().c_str());
    return exitcode == 0;
}

std::string RHGamestationSystem::getRootPassword() {

    std::ostringstream oss;
    oss << Settings::getInstance()->getString("RHGamestationSettingScript") << " " << "getRootPassword";
    FILE *pipe = popen(oss.str().c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return "";
    }

    if (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        pclose(pipe);
        return std::string(line);
    }
    return oss.str().c_str();
}

std::pair<std::string, int> RHGamestationSystem::execute(std::string command) {
    std::ostringstream oss;
    oss << command;

    FILE *pipe = popen(oss.str().c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return std::make_pair("", -1);
    }
    std::ostringstream res;
    while (fgets(line, 1024, pipe)) {
        res << line;
    }
    int exitCode = pclose(pipe);
    return std::make_pair(res.str(), WEXITSTATUS(exitCode));
}

bool RHGamestationSystem::ping() {
    std::string updateserver = Settings::getInstance()->getString("UpdateServer");
    std::string s("ping -c 1 " + updateserver);
    int exitcode = system(s.c_str());
    return exitcode == 0;
}

std::pair<std::string, int> RHGamestationSystem::getSDLBatteryInfo(){
    std::pair<std::string, int> result;
    int percent;
    auto powerInfo = SDL_GetPowerInfo(NULL,&percent);
    switch (powerInfo){
        case SDL_POWERSTATE_UNKNOWN:
        {
            percent = -1;
            break;
        }
        case SDL_POWERSTATE_NO_BATTERY:
        {
            percent = -1;
            break;
        }
        case SDL_POWERSTATE_ON_BATTERY:
        {
            if (percent>66)
                result.first = "\uF1ba";
            else if (percent>33)
                result.first = "\uF1b8";
            else if (percent>15)
                result.first = "\uF1b1";
            else
                result.first = "\uF1b5";
            break;
        }
        case SDL_POWERSTATE_CHARGING:
        {
            result.first = "\uf1b4";
            break;
        }
        case SDL_POWERSTATE_CHARGED:
        {
            result.first = "\uf1b4";
            break;
        }
    }
    result.second = percent;

    return result;
}

std::pair<std::string, int> RHGamestationSystem::getSysBatteryInfo(){
	std::pair<std::string, int> result;


	if (!boost::filesystem::exists("/sys/class/power_supply/BAT0/capacity")) {
		return std::make_pair("", -1);
	}

	std::ifstream ifs("/sys/class/power_supply/BAT0/capacity");
	int percent;
	ifs >> percent;
	ifs.close();

	std::ifstream ifst("/sys/class/power_supply/BAT0/status");
	std::string status;
	ifst >> status;
	ifst.close();
	
	if (status == "Discharging") {
		if (percent>66)
			result.first = "\uF1ba";
		else if (percent>33)
			result.first = "\uF1b8";
		else if (percent>15)
			result.first = "\uF1b1";
		else
			result.first = "\uF1b5";
	}
	else {
		result.first = "\uf1b4";
	}
	result.second = percent;

	return result;
}

std::string RHGamestationSystem::getJSONStringValue(std::string json, std::string key) {
    int startPos = json.find("\"" + key + "\": ");

    if (startPos > 0) {
        std::string token = json.substr(startPos + key.length() + 4, json.length());
        if (token.substr(0, 4) != "null") {
            int endPos = token.find("\"", 1);

            if (endPos > 0) {
                return token.substr(1, endPos - 1);
            }
        }

    }
    return "";
}

EmulatorDefaults RHGamestationSystem::getEmulatorDefaults(std::string emulatorName) {
    EmulatorDefaults defaults;
    auto initConfig = new RHGamestationConf(false);
    std::string json = runCmd("getEmulatorDefaults " + emulatorName);
    
    defaults.emulator = initConfig->get(emulatorName + ".emulator");
    if (defaults.emulator.empty()) {
        defaults.emulator = getJSONStringValue(json, "emulator");
    }

    defaults.core = initConfig->get(emulatorName + ".core");
    if (defaults.core.empty()) {
        defaults.core = getJSONStringValue(json, "core");
    }

    return  defaults;
}

std::string RHGamestationSystem::runCmd(std::string options) {
    std::string cmd = Settings::getInstance()->getString("RHGamestationSettingScript") + " " + options;
    FILE *pipe = popen(cmd.c_str(), "r");
    char line[1024];

    if (pipe == NULL) {
        return "";
    }

    if (fgets(line, 1024, pipe)) {
        strtok(line, "\n");
        pclose(pipe);
        return std::string(line);
    }
    return cmd;
}