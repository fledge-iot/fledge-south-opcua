/*
 * FogLAMP south service plugin
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch, Massimiliano Pinto
 */
#include <opcua.h>
#include <reading.h>
#include <logger.h>
#include <map>

using namespace std;

// Hold subscription variables
map<string, bool> subscriptionVariables;
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
	Logger::getLogger()->debug("addSubscribe: node (%d:%s) %s",
				   nName.NamespaceIndex,
				   nName.Name.c_str(),
				   active ? "true" : "false");

	vector<OpcUa::Node> variables = node.GetVariables();

	if (variables.size() > 0)
	{
		Logger::getLogger()->debug("Node (%d:%s) has %d variables",
					   nName.NamespaceIndex,
					   nName.Name.c_str(),
					   variables.size());
	}

	// Get variables
	for (auto var : variables)
	{
		bool varMatched = false;
		OpcUa::QualifiedName qName = var.GetBrowseName();
		// Create key for variables std::map
		// key is : NameSpaceIndex : NodeNmae : VarName 
		string key = to_string(qName.NamespaceIndex) + ":" + nName.Name + ":" + qName.Name;

		// Get configuration items: ObjectNames or Variables
		for (string subName : m_subscriptions)
		{
			Logger::getLogger()->debug("Variable %s", key.c_str());
			size_t pos;
			if ((pos = subName.find(":")) != string::npos)
			{
				unsigned long pns = stoul(subName.substr(0, pos), NULL, 10);
				Logger::getLogger()->debug("Variable %s has namespace in it, pns %d",
							   subName.c_str(),
							   pns);
				if (qName.Name.compare(subName.substr(pos + 1)) == 0 
						&& pns == qName.NamespaceIndex)
				{
					// Check wether to add variable to the map
					if (subscriptionVariables.find(key) == subscriptionVariables.end())
					{
						varMatched = true;
						subscriptionVariables[key] = true;
						Logger::getLogger()->debug("Adding subscription variable (%s) to "
									   "the map, key (%s)",
									   subName.c_str(),
									   key.c_str());
					}
				}
			}
			else if (subName.compare(qName.Name) == 0)
			{
				// Check wether to add variable to the map
				if (subscriptionVariables.find(key) == subscriptionVariables.end())
				{
					varMatched = true;
					subscriptionVariables[key] = true;
					Logger::getLogger()->debug("Adding subscription variable (%s) to the map "
								   " key (%s)",
								   subName.c_str(),
								   key.c_str());
				}
			}
		}

		// Now handleSubscribeDataChange call
		if (active || varMatched)
		{
			// Handle variable
			if (varMatched)
			{
				auto it = subscriptionVariables.find(key);
				if (it != subscriptionVariables.end())
				{
					if ((*it).second == true)
					{
						Logger::getLogger()->debug("Subscribing to individual variable (%s)",
									   key.c_str());
						try {
							m_sub->SubscribeDataChange(var);
							n_subscriptions++;
						} catch (exception& e) {
							Logger::getLogger()->warn("Subscription to variable (%s) failed, %s",
										  key.c_str(),
										  e.what());
						}
						// We're done with this variable
						(*it).second = false;
					}
				}
			}

			// Handle ObjectNode
			if (active)
			{
				Logger::getLogger()->debug("Subscribing to variable (%s), belonging to (%d:%s)",
							   qName.Name.c_str(),
							   qName.NamespaceIndex,
							   nName.Name.c_str()); 

				auto it = subscriptionVariables.find(key);
				// Check wether an existing variable has to be subscribed
				bool subscribeVariable = true;
				if (it != subscriptionVariables.end())
				{
					subscribeVariable = (*it).second;
					if (subscribeVariable)
					{
						(*it).second = false;
					}
				}

				if (subscribeVariable)
				{
					try {
						m_sub->SubscribeDataChange(var);
						n_subscriptions++;
					} catch (exception& e) {
						Logger::getLogger()->warn("Subscription to variable (%s) failed, %s",
									  key.c_str(),
									  e.what());
					}
				}
			}
		}
	}

	vector<OpcUa::Node> children = node.GetChildren();
	Logger::getLogger()->debug("Node (%d:%s) has %d children",
				   nName.NamespaceIndex,
				   nName.Name.c_str(),
				   children.size());

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

	subscriptionVariables.clear();

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
		n_subscriptions = addSubscribe(m_client->GetObjectsNode(),
					m_subscriptions.size() == 0 ? true : false);
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
	if (n_subscriptions == 0)
	{
		Logger::getLogger()->warn("No eligible variables in OPC UA server to which to subscribe");
		throw runtime_error("No subscriptions");
	}
	else
	{
		Logger::getLogger()->info("Added %d variable subscriptions.", n_subscriptions);
	}
}

/**
 * Stop all subscriptions and disconnect from the OPCUA server
 */
void
OPCUA::stop()
{
	subscriptionVariables.clear();
	m_client->Disconnect();
	delete m_client;
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
