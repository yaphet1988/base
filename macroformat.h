#pragma once


#define SAFE_DELETE(p)  { if (p) {delete p; p = NULL;} }
#define CHECK_NULL(p)   { if(!p) return; }

//------------- Form Func Map ------------------------
#define BEGIN_FORM_FUNC_MAP(uri) \
	switch(uri) \
		{ \

#define ON_RESPONE_FUNC(pkgType, FUNC, unPack, pLink) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal(unPack); \
			FUNC(pkg, pLink); \
		} \
		break; \

#define ON_RESPONE_APP_FUNC(pkgType, FUNC, unPack, pLink) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal(unPack); \
			FUNC(pkg, pLink, unPack.appid()); \
		} \
		break; \

#define ON_RESPONE_SRC_LEN_FUNC(pkgType, FUNC, unPack, pLink, data, len) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal(unPack); \
			FUNC(pkg, pLink, data, len); \
		} \
		break; \

#define ON_RESPONE_APP_SRC_LEN_FUNC(pkgType, FUNC, unPack, pLink, data, len) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal(unPack); \
			FUNC(pkg, pLink, unPack.appid(), data, len); \
		} \
		break; \

#define ON_RESPONE_INCOMP_MARSHAL_FUNC(pkgType, FUNC, unPack, pLink, data, len) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal_incomplete(unPack); \
			FUNC(pkg, pLink, data, len); \
		} \
		break; \

#define ON_RESPONE_APP_INCOMP_MARSHAL_FUNC(pkgType, FUNC, unPack, pLink, data, len) \
		case pkgType::uri: \
		{ \
			pkgType pkg; \
			pkg.unmarshal_incomplete(unPack); \
			FUNC(pkg, pLink, unPack.appid(), data, len); \
		} \
		break; \

#define ON_RESPONE_DEFAULT_FUNC(FUNC, unPack, pLink, data, len) \
		default: \
		{ \
			FUNC(unPack, pLink, data, len); \
		} \

#define END_FORM_FUNC_MAP }

//------------  console form --------------------------
#define ON_CONSOLE_RESPONE_FUNC(cmdType, FUNC, cmd, params, pLink) \
	if ( cmdType == cmd ) \
		{ \
			FUNC(params, pLink); \
			return; \
		} \

#define ON_CONSOLE_DEFAULT_FUNC(FUNC, pLink) \
		else \
		{ \
			FUNC(pLink); \
		}

//------------- Hash Form Map ---------------------------
#define BEGIN_HASH_FORM_MAP(hash_mod_var) \
	switch(hash_mod_var) \
		{ \

#define ON_HASH_FUNC(hash_mod, FUNC, uinfo, svc_type, const_hash_id, pdata, len) \
		case hash_mod: \
		{ \
			FUNC(uinfo, svc_type, const_hash_id, pdata, len); \
		} \
		break; \

#define ON_HASH_DEFAULT_FUNC(hash_mod_const, FUNC, svcType) \
		default: \
		{ \
			FUNC(hash_mod_const, svcType); \
		} \

#define END_HASH_FORM_MAP }

//------------- SQL Form Map ---------------------------
#define BEGIN_SQL_FORM_MAP(K) \
	switch(K) \
		{ \

#define ON_DIGIT_SEMANTIC(sql, TAG, column, V) \
		case TAG: \
		{ \
			sql << column << "=" << V; \
		} \
		break; \

#define ON_STRING_SEMANTIC(sql, TAG, column, V, LEN_LIMIT) \
		case TAG: \
		{ \
			if (V.length() > LEN_LIMIT) \
				continue; \
			sql << column << "='" << V.c_str() << "'"; \
		} \
		break; \

#define END_SQL_FORM_MAP }

