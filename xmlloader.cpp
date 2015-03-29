#include "xmlloader.h"
#include "tinyxml.h"
#include "logger.h"
#include <unistd.h>
#include <string>

XmlLoader::XmlLoader()
: m_pCurNodeHandler(NULL)
{}

XmlLoader::~XmlLoader()
{}

bool XmlLoader::load_xml_file(const char* fileName, const std::string& srvElemName)
{
	char buf[2046] = { 0 };
	char absFileName[2046] = { 0 };

	int n = readlink("/proc/self/exe" , buf , sizeof(buf));
	if( n > 0 && n < 2046 )
	{
		std::string _buf(buf, sizeof(buf));
		size_t _pos = _buf.find_last_of("/\\") + 1;
		if (  _pos != std::string::npos )
		{
			std::string _path = _buf.substr(0, _pos);
			char path[2046] = { 0 };
			strcpy(path, _path.data());
			strcpy(absFileName, strcat(path, fileName) );
		}
		else
			strcpy(absFileName, fileName);
	}
	else
		strcpy(absFileName, fileName);

	FILE *fp = fopen(absFileName, "r");
	if (fp == NULL)
	{
		log(Error, "[XmlLoader::load_xml_file] file %s not found", absFileName);
		return false;
	}

	TiXmlDocument doc;
	doc.LoadFile(fp);

	TiXmlHandle docH( &doc );
	TiXmlHandle servers = docH.FirstChildElement( "conf" ).FirstChildElement( "servers" );

	if( servers.Element() )
	{	
		TiXmlHandle _tmp = servers.FirstChildElement(srvElemName.data());
		m_pCurNodeHandler = &_tmp;
		if ( m_pCurNodeHandler->Element() )
		{
			log(Info, "[XmlLoader::load_xml_file]---------------------");
			parse_my_xml_node();
		}
		else
		{
			log(Error, "[XmlLoader::load_xml_file] %s config not found", srvElemName.data());
			return false;
		}
		m_pCurNodeHandler = NULL;
	}
	else
	{
		log(Error, "[XmlLoader::load_xml_file] element 'conf/servers' is NULL");
		return false;
	}
	return true;
}

bool XmlLoader::load_common_xml_file(const char* fileName)
{
	FILE *fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		log(Error, "[XmlLoader::load_common_xml_file] file %s not found!", fileName);
		return false;
	}

	TiXmlDocument doc;
	doc.LoadFile(fp);

	TiXmlHandle docH(&doc);
	TiXmlHandle common = docH.FirstChildElement("conf").FirstChildElement("common");

	if ( common.Element() )
	{
		m_pCurNodeHandler = &common;
		if ( m_pCurNodeHandler->Element() )
		{
			log(Info, "[XmlLoader::load_common_xml_file]------------");
			parse_common_xml_node();
		}
		else
		{
			log(Error, "[XmlLoader::load_common_xml_file] common config not found");
			return false;
		}
		m_pCurNodeHandler = NULL;
	}
	else
	{
		log(Error, "[XmlLoader::load_common_xml_file] element conf/common is NULL");
		return false;
	}
	return true;
}

void XmlLoader::extract_integer(const char* elemName, uint16_t& value)
{
	TiXmlElement* pElem = m_pCurNodeHandler->FirstChildElement(elemName).Element();
	if (pElem)
	{
		value = atoi( pElem->GetText() );
		log(Info, "[XmlLoader::loadXMLFile] configured %s = %d", elemName, value);
	}
	else
		log(Warn, "[XmlLoader::loadXMLFile] Not found, use default, %s = %d", elemName, value);
}

void XmlLoader::extract_integer(const char* elemName, uint32_t& value)
{
	TiXmlElement* pElem = m_pCurNodeHandler->FirstChildElement(elemName).Element();
	if (pElem)
	{
		value = atoi( pElem->GetText() );
		log(Info, "[XmlLoader::loadXMLFile] configured %s = %d", elemName, value);
	}
	else
		log(Warn, "[XmlLoader::loadXMLFile] Not found, use default, %s = %d", elemName, value);
}

void XmlLoader::extract_integer(const char* elemName, uint64_t& value)
{
	TiXmlElement* pElem = m_pCurNodeHandler->FirstChildElement(elemName).Element();
	if (pElem)
	{
		value = atol( pElem->GetText() );
		log(Info, "[XmlLoader::loadXMLFile] configured %s = %d", elemName, value);
	}
	else
		log(Warn, "[XmlLoader::loadXMLFile] Not found, use default, %s = %d", elemName, value);
}

void XmlLoader::extract_string(const char* elemName, std::string& value)
{
	TiXmlElement* pElem = m_pCurNodeHandler->FirstChildElement(elemName).Element();
	if (pElem)
	{
		value = pElem->GetText();
		log(Info, "[XmlLoader::loadXMLFile] configured %s = %s", elemName, value.c_str());
	}
	else
		log(Warn, "[XmlLoader::loadXMLFile] Not found, use default, %s = %s", elemName, value.c_str());
}

