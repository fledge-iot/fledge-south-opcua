/*
 * Fledge south service plugin
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
OPCUA::OPCUA(const string& url) : m_url(url), m_subscribeById(false),
	m_connected(false), m_client(NULL), m_subClient(NULL), m_reportingInterval(100)
{
}

/**
 * Destructor for the opcua interface
 */
OPCUA::~OPCUA()
{
	if (m_subClient)
	{
		delete m_subClient;
	}
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
 * Set the minimum interval between data change events for subscriptions
 *
 * @param value	Interval in milliseconds
 */
void
OPCUA::setReportingInterval(long value)
{
	m_reportingInterval = value;
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

	try {
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

		// Special case of being called with a variable
		if (m_subscribeById &&
		    node.GetNodeClass() == OpcUa::NodeClass::Variable)
		{
			// Get ParentName
			OpcUa::Node parent = node.GetParent();
			string key;
			try {
				OpcUa::QualifiedName pName = parent.GetBrowseName();
				// Create key
				key = to_string(nName.NamespaceIndex) + ":" + pName.Name + ":" + nName.Name;
			} catch (...) {
				Logger::getLogger()->warn("Failed to get parent browse name for a variable (%d:%s)", nName.NamespaceIndex, nName.Name.c_str());
				key = to_string(nName.NamespaceIndex) + ":_:" +  nName.Name;
			}

			// Add variable if not existant
			if (subscriptionVariables.find(key) == subscriptionVariables.end())
			{
				subscriptionVariables[key] = true;
				Logger::getLogger()->debug("Adding subscription variable by Id (%s) to "
							   "the map, key (%s)",
				nName.Name.c_str(),
				key.c_str());

				try {
					m_sub->SubscribeDataChange(node);
					n_subscriptions++;
				} catch (exception& e) {
					Logger::getLogger()->warn("Subscription to variable (%s) failed, %s", nName.Name.c_str(), e.what());
				}
			}
			return n_subscriptions;
		}

		// Get variables
		for (auto var : variables)
		{
			bool varMatched = false;
			OpcUa::QualifiedName qName = var.GetBrowseName();
			// Create key for variables std::map
			// key is : NameSpaceIndex : NodeName : VarName 
			string key = to_string(qName.NamespaceIndex) + ":" + nName.Name + ":" + qName.Name;

			// Get configuration items: ObjectNames or Variables
			for (auto it = m_subscriptions.begin();
				  it != m_subscriptions.end(); ++it)
			{
				size_t pos;
				string subName = *it;
		
				if (m_subscribeById)
				{
					// Check whether to add variable to the map
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
				else if ((pos = subName.find(":")) != string::npos)
				{
					unsigned long pns = 0;
					try {
						pns = stoul(subName.substr(0, pos), NULL, 10);
					}
					catch (exception& e)
					{
						Logger::getLogger()->error("Exception while parsing "
									   "configuration element '%s' in node '%d:%s', "
									   "error '%s'. Configuration element removed.",
									   subName.c_str(),
									   qName.NamespaceIndex,
									   qName.Name.c_str(),
									   e.what());

						// Remove configuration item
						m_subscriptions.erase(it);
						continue;
					}
					Logger::getLogger()->debug("Variable %s has namespace in it, pns %d",
								   subName.c_str(),
								   pns);
					if (qName.Name.compare(subName.substr(pos + 1)) == 0 
							&& pns == qName.NamespaceIndex)
					{
						// Check whether to add variable to the map
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
						Logger::getLogger()->debug("Subscribing to variable (%s), belonging to (%d:%s)",
									   qName.Name.c_str(),
									   qName.NamespaceIndex,
									   nName.Name.c_str()); 
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
				for (auto it = m_subscriptions.begin();
					  it != m_subscriptions.end(); ++it)
				{
					string parent = *it;
					{
						size_t pos;
						if ((pos = parent.find(":")) != string::npos)
						{
							unsigned long pns = 0;
							try {
								pns = stoul(parent.substr(0, pos), NULL, 10);
							}
							catch (exception& e)
							{
								Logger::getLogger()->error("Exception while parsing "
											   "configuration element '%s' in "
											   "child node '%d:%s', error '%s'. "
											   "Configuration element removed.",
											   parent.c_str(),
											   qName.NamespaceIndex,
											   qName.Name.c_str(),
											   e.what());

								// Remove configuration item
								m_subscriptions.erase(it);
								continue;
							}
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
			}
			try {
				OpcUa::QualifiedName cName = child.GetBrowseName();
				n_subscriptions += addSubscribe(child, child_active);
			} catch (exception& e) {
				OpcUa::QualifiedName cName = child.GetBrowseName();
				Logger::getLogger()->warn("Failed to add subscriptions for child %d:%s, %s",
						cName.NamespaceIndex, cName.Name.c_str(), e.what());
			}
		}

		return n_subscriptions;
	} catch(const std::runtime_error& re) {
		Logger::getLogger()->error("addSubscribe: Runtime error: %s", re.what());
	} catch(const exception& e) {
		Logger::getLogger()->error("addSubscribe: Exception: %s", e.what());
	} catch(...) {
		Logger::getLogger()->error("addSubscribe: Unknown error occured");
	}
	return 0;
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
	try {
		m_client->Connect(m_url);
	} catch (exception &e) {
		Logger::getLogger()->error("Failed to connect to OPCUA server %s: %s", m_url.c_str(), e.what());
		throw e;
	}
	m_connected = true;



	try {
		m_subClient = new OpcUaClient(this);
		m_sub = m_client->CreateSubscription(m_reportingInterval, *m_subClient);
	} catch (exception &e) {
		Logger::getLogger()->error("Failed to setup subscription infrastructure for OPCUA server %s: %s", m_url.c_str(), e.what());
		throw e;
	}

	if (m_subscribeById)
	{
		for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
		{
			string subscription = (*it);
			// Parse out ns=..;s=...
			size_t sStart = subscription.find("s=");
			size_t iStart = subscription.find("i=");
			size_t nsStart = subscription.find("ns=");
			size_t delim = subscription.find(";");

			if (sStart != string::npos && nsStart != string::npos && sStart == nsStart + 1)
			{
				sStart = subscription.find("s=", sStart + 1);
			}
			if ((sStart == string::npos && iStart == string::npos) || nsStart == string::npos || delim == string::npos)
			{
				Logger::getLogger()->error(
					"Malformed subscription string '%s', must be of the form ns=...;s=... or ns=...;i=...",
						subscription.c_str());
				continue;
			}
			string str;
			int ns;
			if (sStart != string::npos)
			{
				if (sStart < delim)
					str = subscription.substr(sStart + 2, delim - (sStart + 2));
				else if (sStart > delim)
					str = subscription.substr(sStart + 2);
				if (nsStart < delim)
					ns = atoi(subscription.substr(nsStart + 3, delim - (nsStart + 3)).c_str());
				else if (nsStart > delim)
					ns = atoi(subscription.substr(nsStart + 3).c_str());
				try {
					OpcUa::NodeId nodeid(str, ns);
					Logger::getLogger()->info("Add string subscription %s", str.c_str());
					OpcUa::Node node = m_client->GetNode(nodeid);
					n_subscriptions += addSubscribe(node, true);
				} catch (...) {
					Logger::getLogger()->error("Failed to find node ns=%d;s=%s", ns, str.c_str());
				}
			}
			else if (iStart != string::npos)
			{
				if (iStart < delim)
					str = subscription.substr(iStart + 2, delim - (iStart + 2));
				else if (iStart > delim)
					str = subscription.substr(iStart + 2);
				if (nsStart < delim)
					ns = atoi(subscription.substr(nsStart + 3, delim - (nsStart + 3)).c_str());
				else if (nsStart > delim)
					ns = atoi(subscription.substr(nsStart + 3).c_str());
				uint32_t nodeNum = atoi(str.c_str());
				try {
					Logger::getLogger()->info("Add integer subscription %d:%d", ns, nodeNum);
					OpcUa::NodeId nodeid(nodeNum, ns);
					OpcUa::Node node = m_client->GetNode(nodeid);
					n_subscriptions += addSubscribe(node, true);
				} catch (exception& e) {
					Logger::getLogger()->error("Failed to find node ns=%d;i=%d: %s", ns, nodeNum, e.what());
				} catch (...) {
					Logger::getLogger()->error("Failed to find node ns=%d;i=%d", ns, nodeNum);
				}
			}

		}
	}
	else
	{


		OpcUa::Node root;
		try {
			root = m_client->GetRootNode();
		} catch (exception &e) {
			Logger::getLogger()->error("Failed to fetch root node from OPCUA server %s: %s", m_url.c_str(), e.what());
			throw e;
		}

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
	}
	if (n_subscriptions == 0)
	{
		Logger::getLogger()->warn("No eligible variables in OPC UA server to which to subscribe");
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
	if (m_connected)
	{
		subscriptionVariables.clear();
		m_client->Disconnect();
	}
	if (m_client)
	{
		delete m_client;
	}
}

/**
 * Called when a data changed event is received. This calls back to the south service
 * and adds the points to the readings queue to send.
 *
 * @param points	        The points in the reading we must create
 * @param sourceTimestamp	Timestamp from the OPC UA server Source
 */
void OPCUA::ingest(vector<Datapoint *> & points, OpcUa::DateTime sourceTimestamp)
{
	string asset = m_asset + points[0]->getName();

	double TimeAsSecondsFloat = ((double) sourceTimestamp) / 1.0E7;		// divide by 100 nanoseconds
	double integerPart = 0.0;
	struct timeval tm;
	tm.tv_sec = OpcUa::DateTime::ToTimeT(sourceTimestamp);
	tm.tv_usec = (suseconds_t) (1E6 * modf(TimeAsSecondsFloat, &integerPart));	// convert fraction to number of microseconds

	Reading reading(asset, points);
	reading.setUserTimestamp(tm);

	(*m_ingest)(m_data, reading);
}
