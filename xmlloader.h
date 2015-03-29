#pragma once
#include <stdint.h>
#include <string>

class TiXmlHandle;
class XmlLoader
{
public:
	XmlLoader();
	~XmlLoader();

	//load current dir's server.xml
	bool load_xml_file(const char* fileName, const std::string& srvElemName);
	//load /opt/meituan/conf/common.xml
	bool load_common_xml_file(const char* fileName="/opt/meituan/conf/common.xml");
protected:
	virtual void parse_my_xml_node() = 0;
	//only called after load_common_xml_file was called
	virtual void parse_common_xml_node() {}

protected:
	void extract_integer(const char* elemName, uint16_t& out_value);
	void extract_integer(const char* elemName, uint32_t& out_value);
	void extract_integer(const char* elemName, uint64_t& out_value);
	void extract_string(const char* elemName, std::string& out_value);

private:
	TiXmlHandle* m_pCurNodeHandler;
};
