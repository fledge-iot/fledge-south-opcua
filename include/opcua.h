#ifndef _OPCUA_H
#define _OPCUA_H
/*
 * FogLAMP south service plugin
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch, Massimiliano Pinto
 */
#include <string>
#include <opc/ua/client/client.h>
#include <opc/ua/node.h>
#include <opc/ua/subscription.h>
#include <reading.h>
#include <logger.h>
#include <mutex>

class OpcUaClient;

class OPCUA
{
	public:
		OPCUA(const std::string& url);
		~OPCUA();
		void		clearSubscription();
		void		addSubscription(const std::string& parent);
		void		setAssetName(const std::string& name);
		void		restart();
		void		newURL(const std::string& url) { m_url = url; };
		void		subscribeById(bool byId) { m_subscribeById = byId; };
		void		start();
		void		stop();
		void		ingest(std::vector<Datapoint *>  points);
		void		registerIngest(void *data, void (*cb)(void *, Reading))
				{
					m_ingest = cb;
					m_data = data;
				}

	private:
		int				addSubscribe(const OpcUa::Node& node, bool active);
		std::vector<std::string>	m_subscriptions;
		std::string			m_url;
		std::string			m_asset;
		OpcUa::UaClient			*m_client;
		void				(*m_ingest)(void *, Reading);
		void				*m_data;
		OpcUaClient			*m_subClient;
		OpcUa::Subscription::SharedPtr	m_sub;
		std::mutex			m_configMutex;
		bool				m_subscribeById;
};

class OpcUaClient : public OpcUa::SubscriptionHandler
{ 
	public:
	  	OpcUaClient(OPCUA *opcua) : m_opcua(opcua) {};
		void DataChange(uint32_t handle,
				const OpcUa::Node & node,
				const OpcUa::Variant & val,
				OpcUa::AttributeId attr) override
		{
			std::string bValue;
			std::string sValue = val.ToString();
			bool replaceBytes = val.Type() == OpcUa::VariantType::BYTE ||
					    val.Type() == OpcUa::VariantType::SBYTE;

			if (replaceBytes)
			{
				const char* replaceByte = "\\u%04d";
				for (size_t i = 0; i < sValue.length(); i++)
				{
					// Replace not printable char
					if (!isprint(sValue[i]))
					{
						char replace[strlen(replaceByte) + 1];
						snprintf(replace,
							 strlen(replaceByte) + 1,
							 replaceByte,
							 sValue[i]);
						bValue += replace;
					}
					else
					{
						bValue += sValue[i];
					}
				}
			}

			DatapointValue value(replaceBytes ? bValue : sValue);
			std::vector<Datapoint *> points;
			points.push_back(new Datapoint(node.GetId().GetStringIdentifier(), value));
			m_opcua->ingest(points);
		};
	private:
		OPCUA		*m_opcua;
};
#endif
