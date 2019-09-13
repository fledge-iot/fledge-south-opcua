/*
 * FogLAMP south plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <opcua.h>
#include <plugin_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <logger.h>
#include <plugin_exception.h>
#include <config_category.h>
#include <rapidjson/document.h>
#include <version.h>

typedef void (*INGEST_CB)(void *, Reading);

using namespace std;

#define PLUGIN_NAME	"opcua"

/**
 * Default configuration
 */
const char *default_config = QUOTE({
	"plugin" : {
       		"description" : "Simple OPC UA data change plugin",
		"type" : "string",
	       	"default" : PLUGIN_NAME,
		"readonly" : "true"
		},
	"asset" : {
       		"description" : "Asset name",
		"type" : "string",
	       	"default" : "opcua",
		"displayName" : "Asset Name",
	       	"order" : "1"
	       	},
	"url" : { "description" : "URL of the OPC UA Server",
		"type" : "string",
	       	"default" : "opc.tcp://mark.local:53530/OPCUA/SimulationServer",
		"displayName" : "OPCUA Server URL",
	       	"order" : "2"},
	"subscription" : {
		"description" : "Variables to observe changes in",
		"type" : "JSON",
	       	"default" : "{ \"subscriptions\" : [  \"ns=5;s=85/0:Simulation\" ] }",
		"displayName" : "OPCUA Object Subscriptions",
	       	"order" : "3"
       		},
	"subscribeById" : {
		"description" : "Subscribe using node id",
		"type" : "boolean",
		"default" : "true",
		"displayName" : "Subcribe By ID",
		"order" : "4"
		}
	});

/**
 * The OPCUA plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
	PLUGIN_NAME,              // Name
	VERSION,                  // Version
	SP_ASYNC, 		  // Flags
	PLUGIN_TYPE_SOUTH,        // Type
	"1.0.0",                  // Interface version
	default_config		  // Default configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	Logger::getLogger()->info("OPC UA Config is %s", info.config);
	return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle
 */
PLUGIN_HANDLE plugin_init(ConfigCategory *config)
{
OPCUA	*opcua;
string	url;


	if (config->itemExists("url"))
	{
		url = config->getValue("url");
		opcua = new OPCUA(url);
	}
	else
	{
		Logger::getLogger()->fatal("UPC UA plugin is missing a URL");
		throw exception();
	}


	if (config->itemExists("asset"))
	{
		opcua->setAssetName(config->getValue("asset"));
	}
	else
	{
		opcua->setAssetName("opcua");
	}

	if (config->itemExists("subscribeById"))
	{
		string byId = config->getValue("subscribeById");
		if (byId.compare("true") == 0)
		{
			opcua->subscribeById(true);
		}
		else
		{
			opcua->subscribeById(false);
		}
	}

	// Now add the subscription data
	string map = config->getValue("subscription");
	rapidjson::Document doc;
	doc.Parse(map.c_str());
	if (!doc.HasParseError())
	{
		if (doc.HasMember("subscriptions") && doc["subscriptions"].IsArray())
		{
			const rapidjson::Value& subs = doc["subscriptions"];
			for (rapidjson::SizeType i = 0; i < subs.Size(); i++)
                        {
                                opcua->addSubscription(subs[i].GetString());
                        }
		}
		else
		{
			Logger::getLogger()->fatal("UPC UA plugin is missing a subscriptions array");
			throw exception();
		}
	}

	return (PLUGIN_HANDLE)opcua;
}

/**
 * Start the Async handling for the plugin
 */
void plugin_start(PLUGIN_HANDLE *handle)
{
OPCUA *opcua = (OPCUA *)handle;


	if (!handle)
		return;
	opcua->start();
}

/**
 * Register ingest callback
 */
void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb, void *data)
{
OPCUA *opcua = (OPCUA *)handle;

	if (!handle)
		throw new exception();
	opcua->registerIngest(data, cb);
}

/**
 * Poll for a plugin reading
 */
Reading plugin_poll(PLUGIN_HANDLE *handle)
{
OPCUA *opcua = (OPCUA *)handle;

	throw runtime_error("OPCUA is an async plugin, poll should not be called");
}

/**
 * Reconfigure the plugin
 *
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, string& newConfig)
{
ConfigCategory	config("new", newConfig);
OPCUA		*opcua = (OPCUA *)*handle;

	if (config.itemExists("url"))
	{
		string url = config.getValue("url");
		opcua->newURL(url);
	}

	if (config.itemExists("asset"))
	{
		opcua->setAssetName(config.getValue("asset"));
	}

	if (config.itemExists("subscribeById"))
	{
		string byId = config.getValue("subscribeById");
		if (byId.compare("true") == 0)
		{
			opcua->subscribeById(true);
		}
		else
		{
			opcua->subscribeById(false);
		}
	}

	if (config.itemExists("subscription"))
	{
		// Now add the subscription data
		string map = config.getValue("subscription");
		rapidjson::Document doc;
		doc.Parse(map.c_str());
		if (!doc.HasParseError())
		{
			opcua->clearSubscription();
			if (doc.HasMember("subscriptions") && doc["subscriptions"].IsArray())
			{
				const rapidjson::Value& subs = doc["subscriptions"];
				for (rapidjson::SizeType i = 0; i < subs.Size(); i++)
				{
					opcua->addSubscription(subs[i].GetString());
				}
			}
			else
			{
				Logger::getLogger()->fatal("UPC UA plugin is missing a subscriptions array");
				throw exception();
			}
		}
	}

	Logger::getLogger()->info("UPC UA plugin restart after reconfigure");
	opcua->restart();
}

/**
 * Shutdown the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
OPCUA *opcua = (OPCUA *)handle;

	opcua->stop();
	delete opcua;
}
};
