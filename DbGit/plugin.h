#pragma once

#include "pluginmain.h"

//plugin data
#define PLUGIN_NAME "DbGit"
#define PLUGIN_VERSION 1

//functions
bool pluginInit(PLUG_INITSTRUCT* initStruct);
void pluginStop();
void pluginSetup();
