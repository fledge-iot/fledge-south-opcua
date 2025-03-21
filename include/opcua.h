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
#include <opc/spdlog/spdlog.h>
#include <opc/spdlog/sinks/sink.h>
#include <opc/spdlog/details/log_msg.h>
#include <reading.h>
#include <logger.h>
#include <mutex>
#include <stdlib.h>
#include <logger.h>

enum class AssetNameType
{
	NodeIdAsName,
	BrowseAsName,
	SubscriptionWithNodeId,
	SubscriptionWithBrowseName,
	FullPathWithNodeId,
	FullPathWithBrowseName
};

class OpcUaClient;

class OPCUA
{
	public:
		OPCUA(const std::string& url);
		~OPCUA();
		void		clearSubscription();
		void		addSubscription(const std::string& parent);
		void		setAssetName(const std::string& name);
		void		setPathDelimiter(const std::string& delmiter);
		void		setAssetNameSource(const std::string& assetNameSource);
		std::string	getAssetPath(const OpcUa::NodeId& nodeId);
		std::string getNodeName(const OpcUa::Node& node);
		void		restart();
		void		newURL(const std::string& url) { m_url = url; };
		void		subscribeById(bool byId) { m_subscribeById = byId; };
		void		start();
		void		stop();
		void		ingest(std::vector<Datapoint *> & points, const std::string & assetPath, OpcUa::DateTime sourceTimestamp);
		void		setReportingInterval(long value);
		void		registerIngest(void *data, void (*cb)(void *, Reading))
				{
					m_ingest = cb;
					m_data = data;
				}

	private:
		int					addSubscribe(const OpcUa::Node& node, std::string& subscriptionParentPath, bool active);
		std::vector<std::string>	m_subscriptions;
		std::string			m_url;
		std::string			m_asset;
		std::string			m_pathDelimiter;
		OpcUa::UaClient			*m_client;
		void				(*m_ingest)(void *, Reading);
		void				*m_data;
		OpcUaClient			*m_subClient;
		OpcUa::Subscription::SharedPtr	m_sub;
		std::mutex			m_configMutex;
		bool				m_subscribeById;
		bool				m_connected;
		bool				m_useBrowseName;
		long				m_reportingInterval;
		AssetNameType		m_assetNameType;
		std::map<OpcUa::NodeId, std::string> m_assetPathNames;
		std::string			createAssetName(const OpcUa::Node& node, const std::string subscriptionPath);
		std::string			NodeIdString(const OpcUa::Node& node);
		void				getNodeFullPath(const OpcUa::Node& node, std::string& fullPath);
		std::shared_ptr<spdlog::logger> createCustomLogger(const std::string &logger_name, spdlog::level::level_enum log_level);
};

class OpcUaClient : public OpcUa::SubscriptionHandler
{ 
	public:
	  	OpcUaClient(OPCUA *opcua) : m_opcua(opcua) {};
		void DataValueChange(uint32_t handle,
				const OpcUa::Node & node,
				const OpcUa::DataValue & dval,
				OpcUa::AttributeId attr) override
		{
			OpcUa::Variant val(dval.Value);
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
					{
						long lval = static_cast<uint8_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::SBYTE:
					{
						long lval = static_cast<int8_t>(val);
						value = DatapointValue(lval);
						break;
					}
					case OpcUa::VariantType::DATE_TIME:
					{
						OpcUa::DateTime timestamp = static_cast<OpcUa::DateTime>(val);
						int64_t raw = static_cast<int64_t>(timestamp);
						struct timeval tm;
						uint64_t micro = raw % 10000000;
						raw -= micro;
						raw = raw / 10000000LL;
						const int64_t daysBetween1601And1970 = 134774;
						const int64_t secsFrom1601To1970 = daysBetween1601And1970 * 24 * 3600LL;
						tm.tv_sec = raw - secsFrom1601To1970;
						tm.tv_usec = micro / 10;

						char date_time[80], usec[10];

						// Populate tm structure with UTC time
						struct tm timeinfo;
						gmtime_r(&tm.tv_sec, &timeinfo);

						// Build date_time with format YYYY-MM-DD HH24:MM:SS.MS+00:00
						// Create datetime with seconds
						std::strftime(date_time, sizeof(date_time),
								"%Y-%m-%d %H:%M:%S", &timeinfo);
						// Add microseconds
						snprintf(usec, sizeof(usec), ".%06lu", tm.tv_usec);
						strcat(date_time, usec);
						strcat(date_time, "+00:00");
						value = DatapointValue(std::string(date_time));
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
			std::string dpname = m_opcua->getNodeName(node);

			if (dpname.length() == 0) {
				Logger::getLogger()->error("No name for data change event: %s", m_opcua->getAssetPath(node.GetId()));
			}

			// Strip " from Datapoint name
			size_t pos;
			while ((pos = dpname.find_first_of("\"")) != std::string::npos)
			{
				dpname.erase(pos, 1);
			}
			points.push_back(new Datapoint(dpname, value));
			m_opcua->ingest(points, m_opcua->getAssetPath(node.GetId()), dval.SourceTimestamp);
		};
	private:
		OPCUA		*m_opcua;
};

/**
 * @class LogSink
 * @brief A custom sink for spdlog that routes log messages to a Logger instance.
 *
 * This class implements a custom sink for spdlog that directly passes raw log
 * messages to a custom Logger instance without applying any formatting. Each log
 * message is routed to the appropriate logging method (e.g., debug, info, warn).
 */
class LogSink : public spdlog::sinks::sink
{
public:
	/**
	 * @brief Constructor for LogSink.
	 *
	 * Initializes the LogSink. No additional initialization is required.
	 */
	explicit LogSink() {}

	/**
	 * @brief Destructor for LogSink.
	 *
	 * Cleans up any resources used by the LogSink. In this case, no resources
	 * need to be explicitly released.
	 */
	~LogSink() {}

	/**
	 * @brief Processes a log message and routes it to the Logger.
	 *
	 * This method is called by spdlog when a log message is emitted. The log
	 * message is passed to the appropriate method on the custom Logger instance
	 * (e.g., debug, info, warn).
	 *
	 * @param msg The log message to process.
	 */
	void log(const spdlog::details::log_msg &msg) override
	{
		spdlog::level::level_enum level = msg.level;

		// Extract the raw log message
		std::string logMsg(msg.raw.data(), msg.raw.size());

		// Route the raw message to the appropriate Logger method
		switch (level)
		{
		case spdlog::level::trace:
		case spdlog::level::debug:
			Logger::getLogger()->debug(logMsg.c_str());
			break;
		case spdlog::level::info:
			Logger::getLogger()->info(logMsg.c_str());
			break;
		case spdlog::level::warn:
			Logger::getLogger()->warn(logMsg.c_str());
			break;
		case spdlog::level::err:
			Logger::getLogger()->error(logMsg.c_str());
			break;
		case spdlog::level::critical:
			Logger::getLogger()->fatal(logMsg.c_str());
			break;
		default:
			Logger::getLogger()->info(logMsg.c_str());
			break;
		}
	}

	/**
	 * @brief Flushes the sink.
	 *
	 * This method is a no-op for the LogSink class because the Logger
	 * implementation does not require explicit flushing.
	 */
	void flush() override
	{
		// No explicit flush needed for Logger
	}
};
#endif
