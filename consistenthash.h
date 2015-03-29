#pragma once
#include "uni.h"
#include "logger.h"
#include <stdint.h>
#include <map>
#include <set>
#include <sstream>

class KEY
{
public:
	KEY() : from(0), to(0) {}
	KEY(const KEY& k)
	{
		if (k.from > k.to)
		{
			log(Error, " [ConsistentHash::KEY1] Error!!! from(%u)>to(%u)", k.from, k.to);
			return;
		}
		this->from = k.from;
		this->to = k.to;
	}
	KEY(uint32_t f, uint32_t t)
	{
		if (f > t)
		{
			log(Error, " [ConsistentHash::KEY2] Error!!! from(%u)>to(%u)", f, t);
			return;
		}
		this->from = f;
		this->to = t;
	}
	explicit KEY(uint32_t k)
	{
		this->from = k;
		this->to = k+1;
	}

	bool operator == (const KEY& key) const
	{
		if (this->from < key.to || key.from < this->to)
			return true;
		else
			return false;
	}
	bool operator < (const KEY& key) const
	{
		return this->to <= key.from;
	}
	bool operator > (const KEY& key) const
	{
		return this->from >= key.to;
	}

public:
	uint32_t from;
	uint32_t to;
};

template<typename V>
class ConsistentHash
{
	typedef std::map<KEY, V>	RANGE_MAP;
	typedef typename RANGE_MAP::iterator	RANGE_MAP_IT;

public:
	ConsistentHash(){}
	~ConsistentHash(){}

	inline RANGE_MAP_IT find(const KEY& k)					{ return _ringMap.find(k); }
	inline RANGE_MAP_IT find(uint32_t id32)					{ return _ringMap.find( KEY(id32) ); }
	inline RANGE_MAP_IT end()								{ return _ringMap.end(); }
	inline void insert(uint32_t from, uint32_t to, V v)		{ _ringMap.insert( std::make_pair( KEY(from,to), v) ); }

	void	add(V v);
	void	del(V v);
	bool	hash(uint32_t id32, V& out);
	bool	empty()	{ return _ringMap.empty(); }
	void	show(std::ostringstream& oss);

private:
	RANGE_MAP		_ringMap;
	std::set<V>		_values;
};

template<typename V>
inline void ConsistentHash<V>::add(V v)
{
	_values.insert(v);
	_ringMap.clear();
	uint32_t _from = 0;
	uint32_t _per = 0xFFFFFFFF / _values.size();
	uint32_t _to = _per;
	size_t idx = 0;
	for ( typename std::set<V>::iterator io = _values.begin(); io != _values.end(); ++io)
	{
		if ( ++idx == _values.size() )
		{
			_to = 0xFFFFFFFF;
		}
		else
		{
			_to = _per * idx;
		}
		this->insert(_from, _to, *io);
		_from = _to;
	}
}

template<typename V>
inline void ConsistentHash<V>::del(V v)
{
	_values.erase(v);
	_ringMap.clear();

	if ( !_values.empty() )
	{
		uint32_t _from = 0;
		uint32_t _per = 0xFFFFFFFF / _values.size();
		uint32_t _to = _per;
		size_t idx = 0;
		for ( typename std::set<V>::iterator io = _values.begin(); io != _values.end(); ++io)
		{
			if ( ++idx == _values.size() )
			{
				_to = 0xFFFFFFFF;
			}
			else
			{
				_to = _per * idx;
			}
			this->insert(_from, _to, *io);
			_from = _to;
		}
	}
}

template<typename V>
inline bool ConsistentHash<V>::hash(uint32_t id32, V& out)
{
	RANGE_MAP_IT iFind = this->find( uni::fnvhash(id32) );
	if (this->end() != iFind)
	{
		out = iFind->second;
		return true;
	}
	else
		return false;
}

template<typename V>
inline void ConsistentHash<V>::show(std::ostringstream& oss)
{
	for (RANGE_MAP_IT io = _ringMap.begin(); io != _ringMap.end(); ++io)
	{
		oss << "[" << io->first.from << "," << io->first.to << ")\t" << std::hex << io->second << std::dec << "\r\n";
	}
}

