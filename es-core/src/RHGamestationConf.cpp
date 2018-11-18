#include "RHGamestationConf.h"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include "Log.h"
#include <boost/algorithm/string/predicate.hpp>

RHGamestationConf *RHGamestationConf::sInstance = NULL;
boost::regex validLine("^(?<key>[^;|#].*?)=(?<val>.*?)$");
boost::regex commentLine("^;(?<key>.*?)=(?<val>.*?)$");

std::string rhgamestationConfFile = "/rhgamestation/share/system/rhgamestation.conf";
std::string rhgamestationConfFileInit = "/rhgamestation/share_init/system/rhgamestation.conf";
std::string rhgamestationConfFileTmp = "/rhgamestation/share/system/rhgamestation.conf.tmp";

RHGamestationConf::RHGamestationConf(bool mainFile) {
    loadRHGamestationConf(mainFile);
}

RHGamestationConf::~RHGamestationConf() {
	if (sInstance && sInstance == this)
		delete sInstance;
}

RHGamestationConf *RHGamestationConf::getInstance() {
    if (sInstance == NULL)
        sInstance = new RHGamestationConf();

    return sInstance;
}

bool RHGamestationConf::loadRHGamestationConf(bool mainFile) {
    std::string line;
    std::string filePath = mainFile ? rhgamestationConfFile : rhgamestationConfFileInit;
    std::ifstream rhgamestationConf(filePath);
    if (rhgamestationConf && rhgamestationConf.is_open()) {
        while (std::getline(rhgamestationConf, line)) {
            boost::smatch lineInfo;
            if (boost::regex_match(line, lineInfo, validLine)) {
                confMap[std::string(lineInfo["key"])] = std::string(lineInfo["val"]);
            }
        }
        rhgamestationConf.close();
    } else {
        LOG(LogError) << "Unable to open " << filePath;
        return false;
    }
    return true;
}


bool RHGamestationConf::saveRHGamestationConf() {
    std::ifstream filein(rhgamestationConfFile); //File to read from
    if (!filein) {
        LOG(LogError) << "Unable to open for saving :  " << rhgamestationConfFile << "\n";
        return false;
    }
    /* Read all lines in a vector */
    std::vector<std::string> fileLines;
    std::string line;
    while (std::getline(filein, line)) {
        fileLines.push_back(line);
    }
    filein.close();


    /* Save new value if exists */
    for (std::map<std::string, std::string>::iterator it = confMap.begin(); it != confMap.end(); ++it) {
        std::string key = it->first;
        std::string val = it->second;
        bool lineFound = false;
        for (int i = 0; i < fileLines.size(); i++) {
            std::string currentLine = fileLines[i];

            if (boost::starts_with(currentLine, key+"=") || boost::starts_with(currentLine, ";"+key+"=")){
                fileLines[i] = key + "=" + val;
                lineFound = true;
            }
        }
        if(!lineFound){
            fileLines.push_back(key + "=" + val);
        }
    }
    std::ofstream fileout(rhgamestationConfFileTmp); //Temporary file
    if (!fileout) {
        LOG(LogError) << "Unable to open for saving :  " << rhgamestationConfFileTmp << "\n";
        return false;
    }
    for (int i = 0; i < fileLines.size(); i++) {
        fileout << fileLines[i] << "\n";
    }

    fileout.close();


    /* Copy back the tmp to rhgamestation.conf */
    std::ifstream  src(rhgamestationConfFileTmp, std::ios::binary);
    std::ofstream  dst(rhgamestationConfFile,   std::ios::binary);
    dst << src.rdbuf();

    remove(rhgamestationConfFileTmp.c_str());

    return true;
}

std::string RHGamestationConf::get(const std::string &name) {
    if (confMap.count(name)) {
        return confMap[name];
    }
    return "";
}
std::string RHGamestationConf::get(const std::string &name, const std::string &defaultValue) {
    if (confMap.count(name)) {
        return confMap[name];
    }
    return defaultValue;
}

bool RHGamestationConf::getBool(const std::string &name, bool defaultValue) {
    if (confMap.count(name)) {
        return confMap[name] == "1";
    }
    return defaultValue;
}

unsigned int RHGamestationConf::getUInt(const std::string &name, unsigned int defaultValue) {
    try {
        if (confMap.count(name)) {
            int value = std::stoi(confMap[name]);
            return value > 0 ? (unsigned int) value : 0;
        }
    } catch(std::invalid_argument&) {}

    return defaultValue;
}

void RHGamestationConf::set(const std::string &name, const std::string &value) {
    confMap[name] = value;
}

void RHGamestationConf::setBool(const std::string &name, bool value) {
    confMap[name] = value ? "1" : "0";
}

void RHGamestationConf::setUInt(const std::string &name, unsigned int value) {
    confMap[name] = std::to_string(value).c_str();
}

bool RHGamestationConf::isInList(const std::string &name, const std::string &value) {
    bool result = false;
    if (confMap.count(name)) {
        std::string s = confMap[name];
        std::string delimiter = ",";

        size_t pos = 0;
        std::string token;
        while (((pos = s.find(delimiter)) != std::string::npos) ) {
            token = s.substr(0, pos);
            if (token == value)
	            result = true;
            s.erase(0, pos + delimiter.length());
        }
	    if (s == value)
		    result = true;
    }
    return result;
}