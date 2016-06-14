//
//  BlackrockLEDDriverPlugin.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"
#include "BlackrockLEDDriverSetIntensityAction.h"
#include "BlackrockLEDDriverPrepareAction.hpp"
#include "BlackrockLEDDriverRunAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class Plugin : public mw::Plugin {
    void registerComponents(boost::shared_ptr<ComponentRegistry> registry) override {
        registry->registerFactory<StandardComponentFactory, Device>();
        registry->registerFactory<StandardComponentFactory, SetIntensityAction>();
        registry->registerFactory<StandardComponentFactory, PrepareAction>();
        registry->registerFactory<StandardComponentFactory, RunAction>();
    }
};


extern "C" mw::Plugin* getPlugin() {
    return new Plugin();
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER
