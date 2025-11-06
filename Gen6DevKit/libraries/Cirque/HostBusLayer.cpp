// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HostBusLayer.h"

extern "C"
{

}


#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HOSTBUSLAYER)
HostBusLayer * HostBusLayer::host_bus = 0;
#endif

HostBusLayer::HostBusLayer()
{
    // first HostBusLayer that gets made is global in HostBus
    // to be helpful for Arduino-like stuff
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HOSTBUSLAYER)
    if (host_bus == 0)  
    {
        host_bus = this; // Init HostBus
    }
#endif
    m_bus_power_on = false;
}

HostBusLayer::~HostBusLayer()
{
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HOSTBUSLAYER)
    if (this == host_bus)
    {
        host_bus = 0;
    }
#endif
}

bool HostBusLayer::busPowerOn(void)
{
    return m_bus_power_on;
}

