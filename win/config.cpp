#include "stdafx.h"
#include "XEdit.h"
#include "config.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

#define SQL_STMT_MAX_LEN	128
static constexpr char* default_AI_KEY  = "03339A1C8FDB6AFF46845E49D120E0400021E161B6341858585C2E25CA3D9C01CA";
static constexpr char* default_AI_URL  = "https://zterm.ai/v1";
static constexpr char* default_AI_FONT = "Courier New";
static constexpr char* default_AI_PWD  = "ZTerm@AI";

static constexpr U16 default_AI_SIZE = 11;
static constexpr U16 default_AI_TIMEOUT = 60;

/* a fix value to cache the start time point */
static ULONGLONG WT_TIME20000101 = 0;
/* get the time of 2000/01/01 00:00:00 */
static bool Get2000UTCTime64()
{
	bool bRet = false;
	FILETIME ft2000;
	SYSTEMTIME st2000;
	st2000.wYear = 2000;
	st2000.wMonth = 1;
	st2000.wDay = 1;
	st2000.wHour = 0;
	st2000.wMinute = 0;
	st2000.wSecond = 0;
	st2000.wMilliseconds = 0;
	bRet = (bool)SystemTimeToFileTime(&st2000, &ft2000);
	WT_TIME20000101 = ((ULONGLONG)ft2000.dwHighDateTime << 32) + ft2000.dwLowDateTime;
	assert(bRet);
	return bRet;
}

/* get the seconds since 2000/01/01 00:00:00 */
static S64 GetCurrentUTCTime64()
{
	SYSTEMTIME st;
	FILETIME ft;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	ULONGLONG tm_now = ((ULONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
	assert(WT_TIME20000101 > 0);
	S64 tm = (S64)((tm_now - WT_TIME20000101) / 10000000); /* in second */
	return tm;
}

static U32 GenerateKeyPair(U8* sec_Key, U8* pub_Key)
{
	int rc;
	U32 ret = WT_FAIL;
	U8  randomize[AI_SEC_KEY_LENGTH];
	U8  sk[AI_SEC_KEY_LENGTH] = { 0 };
	U8  pk[AI_PUB_KEY_LENGTH] = { 0 };
	AES256_ctx ctxAES = { 0 };

	assert(sec_Key && pub_Key);

	secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (ctx)
	{
		secp256k1_pubkey pubkey;
		size_t pklen = 33;
		U8 tries = 255;
		/* Randomizing the context is recommended to protect against side-channel
		 * leakage See `secp256k1_context_randomize` in secp256k1.h for more
		 * information about it. This should never fail. */
		if (WT_OK == wt_GenerateRandom32Bytes(randomize))
		{
			rc = secp256k1_context_randomize(ctx, randomize);
			assert(rc);
		}
		else
		{
			secp256k1_context_destroy(ctx);
			return WT_RANDOM_NUMBER_ERROR;
		}
		// we try to generate a new secret key
		while (tries)
		{
			ret = wt_GenerateRandom32Bytes(sk);
			rc = secp256k1_ec_seckey_verify(ctx, sk);
			if (WT_OK == ret && 1 == rc)
				break;
			tries--;
		}

		if (0 == tries) // we cannot generate the 32 bytes secret key
		{
			secp256k1_context_destroy(ctx);
			return WT_SK_GENERATE_ERR;
		}

		rc = secp256k1_ec_pubkey_create(ctx, &pubkey, sk);
		if (1 == rc)
		{
			rc = secp256k1_ec_pubkey_serialize(ctx, pk, &pklen, &pubkey, SECP256K1_EC_COMPRESSED);
			if (pklen == 33 && rc == 1)
			{
				U8 hash[AI_HASH256_LENGTH];
				wt_sha256_hash((U8*)default_AI_PWD, 8, hash);
				wt_AES256_init(&ctxAES, hash);
				wt_AES256_encrypt(&ctxAES, 2, sec_Key, sk);
				for (U8 i = 0; i < AI_PUB_KEY_LENGTH; i++) pub_Key[i] = pk[i];
				ret = WT_OK;
			}
		}
		secp256k1_context_destroy(ctx);
	}
	return ret;
}

static U32 LoadConfigFromDB(XConfig* cf, WCHAR* path)
{
	bool bAvailable = false;
	U8* utf8Txt;
	U8* blob;
	U32 utf8Len, blob_bytes, ret = WT_FAIL;
	int rc, idx;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	char sql[SQL_STMT_MAX_LEN + 1] = { 0 };
	WIN32_FILE_ATTRIBUTE_DATA fileInfo = { 0 };
	U8 random[32];

	/* generate a 64 bytes random data as the session id from the client */
	if (wt_GenerateRandom32Bytes(random) != WT_OK)
	{
		DWORD pid, tid;
		ULONGLONG tm_now;
		U8* p;
		U8 rnd[16];
		SYSTEMTIME st;
		FILETIME ft;

		pid = GetCurrentProcessId();
		tid = GetCurrentThreadId();
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
		tm_now = ((ULONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;

		p = (U8*)(&pid);
		rnd[0] = *p++;
		rnd[1] = *p++;
		rnd[2] = *p++;
		rnd[3] = *p;

		p = (U8*)(&tid);
		rnd[4] = *p++;
		rnd[5] = *p++;
		rnd[6] = *p++;
		rnd[7] = *p;

		p = (U8*)(&tm_now);
		for (U8 k = 0; k < 8; k++) rnd[k + 8] = *p++;

		wt_sha256_hash(rnd, 16, random);
	}
	//wt_Raw2HexString(random, 32, cf->sessionId, NULL);
	//cf->sessionId[AI_SESSION_LENGTH] = '\0';

	Get2000UTCTime64();

	if (GetFileAttributesExW(path, GetFileExInfoStandard, &fileInfo) != 0)
	{
		if (!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			bAvailable = true;
	}

	rc = sqlite3_open16(path, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return false;
	}

	if (bAvailable == false)
	{
		U8 sk[AI_SEC_KEY_LENGTH];
		U8 pk[AI_PUB_KEY_LENGTH];
		ret = GenerateKeyPair(sk, pk);
		/* create the table at first */
		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE c(id INTEGER PRIMARY KEY,tp INTEGER NOT NULL,it INTEGER,tx TEXT,ft REAL,bo BLOB)",
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}

		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE m(id INTEGER PRIMARY KEY AUTOINCREMENT,dt INTEGER,rs CHAR(1),pk CHAR(66),si CHAR(64),tx TEXT)",
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}

		/* insert some initial data into the tables */
		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,tx) VALUES(%d,1,'%s')", ZT_PARAM_AI_KEY, default_AI_KEY);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,tx) VALUES(%d,1,'%s')", ZT_PARAM_AI_URL, default_AI_URL);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,it) VALUES(%d,0,%d)", ZT_PARAM_AI_PROP, AI_DEFAULT_PROP);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,tx) VALUES(%d,1,'%s')", ZT_PARAM_AI_FONT, default_AI_FONT);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,it) VALUES(%d,0,%d)", ZT_PARAM_AI_SIZE, default_AI_SIZE);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,it) VALUES(%d,0,%d)", ZT_PARAM_AI_TIMEOUT, default_AI_TIMEOUT);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,it) VALUES(%d,0,%d)", ZT_PARAM_AI_PROXY_TYPE, AI_CURLPROXY_NO_PROXY);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN,
			"INSERT INTO c(id,tp,tx) VALUES(%d,1,'')", ZT_PARAM_AI_PROXY);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto _exit_LoadConf;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc)
		{
			sqlite3_finalize(stmt);
			goto _exit_LoadConf;
		}
		sqlite3_finalize(stmt);
	}

	ret = WT_OK;
#if 0
	memcpy_s(cf->ai_key, AI_APP_KEY_LENGTH, default_AI_KEY, AI_APP_KEY_LENGTH);
	cf->ai_key[AI_APP_KEY_LENGTH] = '\0';
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT tx FROM c WHERE id=%d", ZT_PARAM_AI_KEY);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			utf8Txt = (U8*)sqlite3_column_text(stmt, 0);
			if (utf8Txt)
			{
				utf8Len = (U32)strlen((const char*)utf8Txt);
				if (utf8Len == AI_APP_KEY_LENGTH)
				{
					if (wt_IsHexString(utf8Txt, AI_APP_KEY_LENGTH))
					{
						memcpy_s(cf->ai_key, AI_APP_KEY_LENGTH, utf8Txt, AI_APP_KEY_LENGTH);
						cf->ai_key[AI_APP_KEY_LENGTH] = '\0';
					}
				}
			}
		}
		sqlite3_finalize(stmt);
	}

	utf8Len = strlen(default_AI_URL);
	ATLASSERT(utf8Len < 256);
	memcpy_s(cf->ai_url, 256, default_AI_URL, utf8Len);
	cf->ai_url[utf8Len] = '\0';
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT tx FROM c WHERE id=%d", ZT_PARAM_AI_URL);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			utf8Txt = (U8*)sqlite3_column_text(stmt, 0);
			if (utf8Txt)
			{
				utf8Len = (U32)strlen((const char*)utf8Txt);
				if (utf8Len < 256)
				{
					memcpy_s(cf->ai_url, 256, utf8Txt, utf8Len);
					cf->ai_url[utf8Len] = '\0';
				}
			}
		}
		sqlite3_finalize(stmt);
	}

	cf->ai_property = ZT_AI_DEFAULT_PROP;
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT it FROM c WHERE id=%d", ZT_PARAM_AI_PROP);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			cf->ai_property = (U32)sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	utf8Len = strlen(default_AI_FONT);
	ATLASSERT(utf8Len < 32);
	memcpy_s(cf->ai_font, 32, default_AI_FONT, utf8Len);
	cf->ai_font[utf8Len] = '\0';
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT tx FROM c WHERE id=%d", ZT_PARAM_AI_FONT);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			utf8Txt = (U8*)sqlite3_column_text(stmt, 0);
			if (utf8Txt)
			{
				utf8Len = (U32)strlen((const char*)utf8Txt);
				if (utf8Len < 32)
				{
					memcpy_s(cf->ai_font, 32, utf8Txt, utf8Len);
					cf->ai_font[utf8Len] = '\0';
				}
			}
		}
		sqlite3_finalize(stmt);
	}

	cf->ai_size = default_AI_SIZE;
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT it FROM c WHERE id=%d", ZT_PARAM_AI_SIZE);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			cf->ai_size = (U16)sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	cf->ai_timeout = default_AI_TIMEOUT;
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT it FROM c WHERE id=%d", ZT_PARAM_AI_TIMEOUT);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			cf->ai_timeout = (U16)sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	cf->ai_proxytype = AI_CURLPROXY_NO_PROXY;
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT it FROM c WHERE id=%d", ZT_PARAM_AI_PROXY_TYPE);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			U8 pxtype = (U8)sqlite3_column_int(stmt, 0);
			if (pxtype >= AI_CURLPROXY_NO_PROXY && pxtype <= AI_CURLPROXY_TYPE_MAX)
				cf->ai_proxytype = pxtype;
		}
		sqlite3_finalize(stmt);
	}

	cf->ai_proxy[0] = '\0';
	sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT tx FROM c WHERE id=%d", ZT_PARAM_AI_PROXY);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW == rc)
		{
			utf8Txt = (U8*)sqlite3_column_text(stmt, 0);
			if (utf8Txt)
			{
				utf8Len = (U32)strlen((const char*)utf8Txt);
				if (utf8Len && utf8Len < 256)
				{
					memcpy_s(cf->ai_proxy, 256, utf8Txt, utf8Len);
					cf->ai_proxy[utf8Len] = '\0';
				}
			}
		}
		sqlite3_finalize(stmt);
	}
#endif
_exit_LoadConf:
	sqlite3_close(db);
	return ret;
}

U32 zt_LoadConfiguration(HINSTANCE hInstance, XConfig* cf, WCHAR* path, U16 max_len)
{
	U32 ret = WT_FAIL;
	DWORD len, i = 0;

	ATLASSERT(cf);
	ATLASSERT(path);

	len = GetModuleFileNameW(hInstance, path, max_len);
	if (len > 0)
	{
		for (i = len - 1; i > 0; i--)
		{
			if (path[i] == L'\\')
				break;
		}
	}

	if (i > 0 || i < (max_len - 6))
	{
		path[i + 1] = L'a';
		path[i + 2] = L'i';
		path[i + 3] = L'.';
		path[i + 4] = L'd';
		path[i + 5] = L'b';
		path[i + 6] = L'\0';

		ret = LoadConfigFromDB(cf, path);
	}
	return ret;
}
