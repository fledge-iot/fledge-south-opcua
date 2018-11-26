/*
 * FogLAMP south service plugin
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <opcua.h>
#include <reading.h>
#include <logger.h>

using namespace std;

/**
 * Constructor for the opcua plugin
 */
OPCUA::OPCUA(const string& url) : m_url(url)
{
}

/**
 * Destructor for the opcua interface
 */
OPCUA::~OPCUA()
{
	delete m_subClient;
}

/**
 * Set the asset name for the asset we write
 *
 * @param asset Set the name of the asset with insert into readings
 */
void
OPCUA::setAssetName(const std::string& asset)
{
	m_asset = asset;
}

/**
 * Clear down the subscriptions ahead of reconfiguration
 */
void
OPCUA::clearSubscription()
{
	m_subscriptions.clear();
}

/**
 * Add a subscription parent node to the list
 */
void
OPCUA::addSubscription(const string& parent)
{
	m_subscriptions.push_back(parent);
}

/**
 * Restart the OPCUA connection
 */
void
OPCUA::restart()
{
	stop();
	start();
}

/**
 * Starts the plugin
 *
 * We register with the OPC UA server, retrieve all the objects under the parent
 * to which we are subscribing and start the process to enable OPC UA to send us
 * change notifications for those items.
 */
void
OPCUA::start()
{
	m_client = new OpcUa::UaClient(Logger::getLogger());
	m_client->Connect(m_url);

	OpcUa::Node root = m_client->GetRootNode();

	m_subClient = new OpcUaClient(this);
	m_sub = m_client->CreateSubscription(100, *m_subClient);

	/* For every parent object we subscribe to fidn it's children and
	 * add an OPC UA subscription for data changed events.
	 */
	for (string parent : m_subscriptions)
	{
		vector<string> varpath({"Objects", parent});
		OpcUa::Node parentNode = root.GetChild(varpath);
		for (OpcUa::Node child : parentNode.GetChildren())
		{
			string childName = child.GetId().GetStringIdentifier();
			vector<string> childpath({"Objects", parent, childName});
			OpcUa::Node subvar = root.GetChild(childpath);
			m_sub->SubscribeDataChange(subvar);
		}
	}
}

/**
 * Stop all subscriptions and disconnect from the OPCUA server
 */
void
OPCUA::stop()
{
	m_client->Disconnect();
}

/**
 * Called when a data changed event is received. This calls back to the south service
 * and adds the points to the readings queue to send.
 *
 * @param points	The points in the reading we must create
 */
void OPCUA::ingest(vector<Datapoint *>	points)
{
string asset = m_asset + points[0]->getName();

	(*m_ingest)(m_data, Reading(asset, points));
}
