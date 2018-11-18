/* 
 * File:   NetworkThread.cpp
 * Author: matthieu
 * 
 * Created on 6 février 2015, 11:40
 */

#include "NetworkThread.h"
#include "rhgamestation/RHGamestationSystem.h"
#include "rhgamestation/RHGamestationUpgrade.h"
#include "RHGamestationConf.h"
#include "Locale.h"

NetworkThread::NetworkThread(Window* window) : mWindow(window){
    mFirstRun = true;
    mRunning = true;
    mThreadHandle = new boost::thread(boost::bind(&NetworkThread::run, this));
}

NetworkThread::~NetworkThread() {
    mThreadHandle->join();
}

void NetworkThread::run(){
    while(mRunning){
        if(mFirstRun){
            boost::this_thread::sleep(boost::posix_time::seconds(15));
            mFirstRun = false;
        } else {
            boost::this_thread::sleep(boost::posix_time::hours(1));
        }

        if (RHGamestationUpgrade::getInstance()->canUpdate()) {
            if(RHGamestationConf::getInstance()->get("updates.enabled") == "1") {
                std::string changelog = RHGamestationUpgrade::getInstance()->getUpdateChangelog();

                while (mWindow->isShowingPopup()) {
                    boost::this_thread::sleep(boost::posix_time::seconds(5));
                }

                if (changelog != "") {
                    std::string message = changelog;
                    std::string updateVersion = RHGamestationUpgrade::getInstance()->getUpdateVersion();
                    mWindow->displayScrollMessage(_("AN UPDATE IS AVAILABLE FOR YOUR RHGAMESTATION"),
                                                _("UPDATE VERSION:") + " " + updateVersion + "\n" +
                                                _("UPDATE CHANGELOG:") + "\n" + message);
                } else {
                    mWindow->displayMessage(_("AN UPDATE IS AVAILABLE FOR YOUR RHGAMESTATION"));
                }
            }
            mRunning = false;
        }
    }
}

