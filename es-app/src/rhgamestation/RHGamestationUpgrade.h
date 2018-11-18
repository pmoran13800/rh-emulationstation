#ifndef EMULATIONSTATION_ALL_RHGamestationUpgrade_H
#define EMULATIONSTATION_ALL_RHGamestationUpgrade_H

#include "RHGamestationSystem.h"

class RHGamestationUpgrade {

public:
    static RHGamestationUpgrade *getInstance();

    std::string getVersion();

    std::string getUpdateVersion();

    bool updateLastChangelogFile();

    std::string getChangelog();

    std::pair<std::string, int> updateSystem(BusyComponent *ui);

    std::string getUpdateChangelog();

    bool canUpdate();

private:
    RHGamestationSystem *system = RHGamestationSystem::getInstance();
    static RHGamestationUpgrade *instance;

};


#endif //EMULATIONSTATION_ALL_RHGamestationUpgrade_H
