#ifndef _OPCUA_H
#define _OPCUA_H
/*
 * Fledge south service plugin
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
#include <stdlib.h>

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
		void		setReportingInterval(long value);
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
		bool				m_connected;
		long				m_reportingInterval;
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
			if (val.IsNul())
				return;
			// We don't support non-scalar or Nul values as conversion
			// to string does not work.
			DatapointValue value(0L);
			if (!val.IsScalar())
			{
				std::vector<double> dvec;
				switch (val.Type())
				{
					case OpcUa::VariantType::BYTE:
					{
						std::vector<uint8_t> vec = static_cast<std::vector<uint8_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::SBYTE:
					{
						std::vector<int8_t> vec = static_cast<std::vector<int8_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::INT16:
					{
						std::vector<int16_t> vec = static_cast<std::vector<int16_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::UINT16:
					{
						std::vector<uint16_t> vec = static_cast<std::vector<uint16_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::INT32:
					{
						std::vector<int32_t> vec = static_cast<std::vector<int32_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::UINT32:
					{
						std::vector<uint32_t> vec = static_cast<std::vector<uint32_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::INT64:
					{
						std::vector<int64_t> vec = static_cast<std::vector<int64_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::UINT64:
					{
						std::vector<uint64_t> vec = static_cast<std::vector<uint64_t> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::FLOAT:
					{
						std::vector<float> vec = static_cast<std::vector<float> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					case OpcUa::VariantType::DOUBLE:
					{
						std::vector<double> vec = static_cast<std::vector<double> >(val);
						for (int i = 0; i < vec.size(); i++)
						{
							double d = vec[i];
							dvec.push_back(d);
						}
						break;
					}
					default:
						return;
				}
				value = DatapointValue(dvec);
			}
			else
			{
				switch (val.Type())
				{
					case OpcUa::VariantType::BYTE:
					case OpcUa::VariantType::SBYTE:
					case OpcUa::VariantType::DATE_TIME:
					{
						std::string sValue = val.ToString();
						std::string bValue;
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
						value = DatapointValue(bValue);
						break;
					}
					case OpcUa::VariantType::INT16:
					{
						long lval = static_cast<int16_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::UINT16:
					{
						long lval = static_cast<uint16_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::INT32:
					{
						long lval = static_cast<int32_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::UINT32:
					{
						long lval = static_cast<uint32_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::INT64:
					{
						long lval = static_cast<int64_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::UINT64:
					{
						long lval = static_cast<uint64_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::FLOAT:
					{
						double fval = static_cast<float>(val);
						value = DatapointValue(fval);
						break;
					}
					case OpcUa::VariantType::DOUBLE:
					{
						double fval = static_cast<double>(val);
						value = DatapointValue(fval);
						break;
					}
					default:
					{
						std::string sValue = val.ToString();
						value = DatapointValue(sValue);
						break;
					}
				}
			}

			std::vector<Datapoint *> points;
			std::string dpname = "Unknown";;
			try {
				OpcUa::NodeId id = node.GetId();
				if (id.IsInteger())
				{
					char buf[80];
					snprintf(buf, sizeof(buf), "%d", id.GetIntegerIdentifier());
					dpname = buf;
				}
				else
				{
					dpname = node.GetId().GetStringIdentifier();
				}
			} catch (std::exception& e) {
				Logger::getLogger()->error("No name for data change event: %s", e.what());
			}
			points.push_back(new Datapoint(dpname, value));
			m_opcua->ingest(points);
		};
	private:
		OPCUA		*m_opcua;
};
#endif
