//
//  BlackrockLEDDriverPlugin.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class Plugin : public mw::Plugin {
    void registerComponents(boost::shared_ptr<ComponentRegistry> registry) override {
        registry->registerFactory<StandardComponentFactory, Device>();
    }
};


extern "C" mw::Plugin* getPlugin() {
    return new Plugin();
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER
