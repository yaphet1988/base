#pragma once
#include "ilink.h"
#include "sockbuf.h"
#include <string>

enum
{
	STATE_PARSE_HEADER = 1,
	STATE_PARSE_BODY = 2,
	STATE_PARSE_DONE = 3
};

struct websockframe
{
	uint8_t state;
	uint8_t FIN; //1 bit
	uint8_t RSV123; //3 bits
	uint8_t opcode; //4 bits
	uint8_t MASK; //1 bit
	uint64_t payload_len; //7/16/64 bits
	uint32_t Masking_key; //32 bits, if MASK set to 1
	inputbuf_t payload_data;

	websockframe() : state(STATE_PARSE_HEADER), FIN(0), RSV123(0), opcode(0), MASK(0), payload_len(0), Masking_key(0) {}
};

class WebSockLink
	: public ILink
{
public:
	WebSockLink();
	~WebSockLink();
private:
	WebSockLink(int fd); //for function accept calls only, to avoid fd leak!

public:
	//fdbase
	virtual void on_socket_read();
	virtual void on_socket_write();
	virtual void on_socket_error();
	//ilink
	virtual int send(const char* data, size_t len, uint32_t ip=0, uint16_t port=0);

public:
	int handshake(uint32_t ip, uint16_t port); //connect and handshake, unavailable now
	int listen(uint16_t base_port);
	int set_non_block();
	int close();
	WebSockLink* accept();

private:
	int pre_handle_data(const char* data, size_t len);
	int handle_handshake(const char* data, size_t len);
	int handle_frame_data(const char* data, size_t len);
	int encode_frame(const char* data, size_t len);
	int decode_frame(const char* data, size_t len);
	int respone_handshake(const std::string& acceptkey);
	std::string create_acceptkey(const std::string& clientkey);

private:
	bool	m_bListen;
	bool	m_bConnected;
	bool	m_bHandshakeDone;
	std::string	m_client_key;
	std::string m_accept_key;
	inputbuf_t	m_raw_input; //websocket raw data input buf
	inputbuf_t	m_app_input; //app input data after decode frame
	outputbuf_t	m_output;
};
