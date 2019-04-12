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
	lock_guard<mutex> guard(m_configMutex);
	m_subscriptions.clear();
}

/**
 * Add a subscription parent node to the list
 */
void
OPCUA::addSubscription(const string& parent)
{
	lock_guard<mutex> guard(m_configMutex);
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
 * Recurse the object tree and add subscriptions for all variables that are found.
 * The member variable m_subscriptions holds filters that wil be applied to the
 * subscription process. If this is non-empty then it contains a set of strings which
 * are matched against the name of the items in the object tree. Only variables that are in
 * a node that is a desendant of one of these named nodes is added to the subscription list.
 *
 * @param	The node to recurse from
 * @active	Should subscriptions be added, i.e. have we satisfied any filtering requirements.
 * @return	The number of subscriptions added
 */
int OPCUA::addSubscribe(const OpcUa::Node& node, bool active)
{
	int n_subscriptions = 0;

	OpcUa::QualifiedName nName = node.GetBrowseName();
	Logger::getLogger()->debug("addSubscribe(%d:%s) %s", nName.NamespaceIndex, nName.Name.c_str(), active ? "true" : "false");
	if (active)
	{
		vector<OpcUa::Node> variables = node.GetVariables();
		Logger::getLogger()->debug("Node has %d variables", variables.size());
		for (auto var : variables)
		{
			OpcUa::QualifiedName qName = var.GetBrowseName();
			Logger::getLogger()->debug("Subscribe to variable %d:%s", qName.NamespaceIndex, qName.Name.c_str());
			try {
				m_sub->SubscribeDataChange(var);
				n_subscriptions++;
			} catch (exception& e) {
				Logger::getLogger()->warn("Subscription to variable %d:%s failed, %s", qName.NamespaceIndex, qName.Name.c_str(), e.what());
			}
		}
	}
	vector<OpcUa::Node> children = node.GetChildren();
	Logger::getLogger()->debug("Node has %d children", children.size());
	for (auto child : children)
	{
		bool child_active = active;
		if (! child_active)
		{
			OpcUa::QualifiedName qName = child.GetBrowseName();
			for (string parent : m_subscriptions)
			{
				size_t pos;
				if ((pos = parent.find(":")) != string::npos)
				{
					unsigned long pns = stoul(parent.substr(0, pos), NULL, 10);
					if (qName.Name.compare(parent.substr(pos + 1)) == 0 
							&& pns == qName.NamespaceIndex)
					{
						child_active = true;
					}
				}
				else if (parent.compare(qName.Name) == 0)
				{
					child_active = true;
				}
			}
		}
		try {
			n_subscriptions += addSubscribe(child, child_active);
		} catch (exception& e) {
			OpcUa::QualifiedName cName = child.GetBrowseName();
			Logger::getLogger()->warn("Failed to add subscriptions for child %d:%s, %s",
					cName.NamespaceIndex, cName.Name.c_str(), e.what());
		}
	}

	return n_subscriptions;
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
int n_subscriptions = 0;

	m_client = new OpcUa::UaClient(Logger::getLogger());
	m_client->Connect(m_url);

	OpcUa::Node root = m_client->GetRootNode();


	m_subClient = new OpcUaClient(this);
	m_sub = m_client->CreateSubscription(100, *m_subClient);

	/*
	 * First look under the Objects root for any variables to subscribe to that
	 * match out filter criteria for subscriptions.
	 */
	Logger::getLogger()->info("Look for variable to subscribe to under ObjectsNode");
	lock_guard<mutex> guard(m_configMutex);
	try {
		n_subscriptions = addSubscribe(m_client->GetObjectsNode(), m_subscriptions.size() == 0 ? true : false);
	} catch (exception& e) {
		Logger::getLogger()->error("Failed to create subscriptions from Objects node: %s", e.what());
	}

	/*
	 * If we failed to find subscriptions under the Objects node then
	 * we will try again from the root.
	 */
	if (n_subscriptions == 0)
	{
		Logger::getLogger()->warn("Look for variable to subscribe to under the root node");
		try {
			n_subscriptions != addSubscribe(root, m_subscriptions.size() == 0 ? true : false);
		} catch (exception& e) {
			Logger::getLogger()->error("Failed to create subscriptions from root node: %s", e.what());
		}
	}
	Logger::getLogger()->info("Added %d variable subscriptions.", n_subscriptions);
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
