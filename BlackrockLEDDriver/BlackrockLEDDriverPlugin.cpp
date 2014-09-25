//
//  BlackrockLEDDriverPlugin.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW


class BlackrockLEDDriverPlugin : public Plugin {
    void registerComponents(boost::shared_ptr<ComponentRegistry> registry) override {
        registry->registerFactory<StandardComponentFactory, BlackrockLEDDriverDevice>();
    }
};


extern "C" Plugin* getPlugin() {
    return new BlackrockLEDDriverPlugin();
}


END_NAMESPACE_MW
