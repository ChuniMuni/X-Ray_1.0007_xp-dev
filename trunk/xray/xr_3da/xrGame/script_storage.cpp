////////////////////////////////////////////////////////////////////////////
//	Module 		: script_storage.cpp
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Storage
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_storage.h"
#include "../lua_tools.h"
#include "../../build_config_defines.h"
#include "script_engine.h"
#include "ai_space.h"

static constexpr const char* FILE_HEADER = "\
local function script_name() \
return '%s' \
end; \
local this; \
module('%s', package.seeall, function(m) this = m end); \
%s";

CScriptStorage::CScriptStorage() //fixed
{
	m_virtual_machine = nullptr;
}

CScriptStorage::~CScriptStorage() //fixed
{
	//Msg("[CScriptStorage] Closing LuaJIT - start");
	if (m_virtual_machine)
		lua_close(m_virtual_machine); //��� ��� ����������� LuaJIT.
	//Msg("[CScriptStorage] Closing LuaJIT - end");
}

void CScriptStorage::reinit(lua_State *LSVM) //KRodin: fixed
{
	if (m_virtual_machine) //��� ����������, ����� ���������� ��� �������� ���� �� ����� ������ ������� 
	{
		//Msg("[CScriptStorage] Found working LuaJIT WM! Close it!");
		lua_close(m_virtual_machine);
	}
	m_virtual_machine = LSVM;
#ifdef LUAICP_COMPAT
	#include "luaicp_attach.inc" // added by alpet 05.07.14
#endif
}

void CScriptStorage::print_stack() //KRodin: fixed
{
	Msg("%s", get_lua_traceback(lua(), 0));
}

void CScriptStorage::script_log(ScriptStorage::ELuaMessageType tLuaMessageType, const char* caFormat, ...) //������������ � ����� ������ ������ //����� ����� ����� � ���.
{
#ifdef DEBUG
	va_list marker;
	va_start(marker, caFormat);
	//
	LPCSTR S = "";
	LPSTR S1;
	string4096 S2;
	switch (tLuaMessageType)
	{
		case ScriptStorage::eLuaMessageTypeInfo: {
			S = "[LUA INFO]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeError: {
			S = "[LUA ERROR]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeMessage: {
			S = "[LUA MESSAGE]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeHookCall: {
			S = "[LUA HOOK_CALL]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeHookReturn: {
			S = "[LUA HOOK_RETURN]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeHookLine: {
			S = "[LUA HOOK_LINE]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeHookCount: {
			S = "[LUA HOOK_COUNT]";
			break;
		}
		case ScriptStorage::eLuaMessageTypeHookTailReturn: {
			S = "[LUA HOOK_TAIL_RETURN]";
			break;
		}
		default: NODEFAULT;
	}
	xr_strcpy(S2, S);
	S1 = S2 + xr_strlen(S);
	vsprintf(S1, caFormat, marker);
	Msg("-----------------------------------------");
	Msg("[script_log] %s", S2);
	ai().script_engine().print_stack();
	Msg("-----------------------------------------");
	va_end(marker);
#endif
}

bool CScriptStorage::load_buffer(lua_State *L, const char* caBuffer, size_t tSize, const char* caScriptName, const char* caNameSpaceName) //KRodin: ��� ������� ����������� ���������� ������� ��������� FILE_HEADER � ����� ����� ��������� ��� � lua
{
	int l_iErrorCode;
	if (strcmp(GlobalNamespace, caNameSpaceName)) //��� ������� ����� _G
	{
		//KRodin: ���������, �.�. � ��������� ��� ����������� �����, �� ��� ������, ��������.
		int buf_len = std::snprintf(nullptr, 0, FILE_HEADER, caNameSpaceName, caNameSpaceName, caBuffer);
		auto strBuf = std::make_unique<char[]>(buf_len + 1);
		std::snprintf(strBuf.get(), buf_len + 1, FILE_HEADER, caNameSpaceName, caNameSpaceName, caBuffer);
		//Log("[CScriptStorage::load_buffer(1)] Loading buffer:");
		//Log(strBuf.get());
		l_iErrorCode = luaL_loadbuffer(L, strBuf.get(), buf_len /*+ 1 ����-���������� �� ����� ������ ������*/, caScriptName);
	}
	else //_G.script � ������ ��.
	{
		//Log("[CScriptStorage::load_buffer(2)] Loading buffer:");
		//Log(caBuffer);
		l_iErrorCode = luaL_loadbuffer(L, caBuffer, tSize, caScriptName);
	}
	if (l_iErrorCode)
	{
		print_output(L, caScriptName, l_iErrorCode);
		R_ASSERT(false); //�� ����������������!
		return false;
	}
	return true;
}

bool CScriptStorage::do_file(const char* caScriptName, const char* caNameSpaceName) //KRodin: ��� ������� ��������� ������ � ����� � ��������� ��� ���������� � ������� load_buffer, ����� ����� ������ ��������� ������.
{
	string_path l_caLuaFileName;
	auto l_tpFileReader = FS.r_open(caScriptName);
	if (!l_tpFileReader) { //�������� �� ������?
		Msg("!![CScriptStorage::do_file] Cannot open file [%s]", caScriptName);
		return false;
	}
	strconcat(sizeof(l_caLuaFileName), l_caLuaFileName, "@", caScriptName); //KRodin: �������� ���� � ���� @f:\games\s.t.a.l.k.e.r\gamedata\scripts\class_registrator.script
	//
	//KRodin: ����������. ������ ���������� ������� ����� �������� ���������, ��� ������ �� �����.
	auto strBuf = std::make_unique<char[]>(l_tpFileReader->length() + 1);
	strncpy(strBuf.get(), (const char*)l_tpFileReader->pointer(), l_tpFileReader->length());
	strBuf.get()[l_tpFileReader->length()] = 0;
	//
	load_buffer(lua(), strBuf.get(), (size_t)l_tpFileReader->length(), l_caLuaFileName, caNameSpaceName);
	FS.r_close(l_tpFileReader);

	int	l_iErrorCode = lua_pcall(lua(), 0, 0, 0); //KRodin: ��� ����� ������� �� ��������!
	if (l_iErrorCode)
	{
		print_output(lua(), caScriptName, l_iErrorCode);
		R_ASSERT(false); //�� ����������������!
		return false;
	}
	return true;
}

bool CScriptStorage::namespace_loaded(const char* name, bool remove_from_stack) //KRodin: ������, ������� ���������, �������� �� ������.
{
	int start = lua_gettop(lua());
	lua_pushstring(lua(), GlobalNamespace);
	lua_rawget(lua(), LUA_GLOBALSINDEX); 
	string256 S2;
	xr_strcpy(S2, name);
	auto S = S2;
	for (;;)
	{
		if (!xr_strlen(S))
		{
			VERIFY(lua_gettop(lua()) >= 1);
			lua_pop(lua(), 1);
			VERIFY(start == lua_gettop(lua()));
			return false;
		}
		auto S1 = strchr(S, '.');
		if (S1)
			*S1 = 0; 
		lua_pushstring(lua(),S); 
		lua_rawget(lua(),-2); 
		if (lua_isnil(lua(), -1))
		{ 
			//lua_settop(lua(),0);
			VERIFY(lua_gettop(lua()) >= 2);
			lua_pop(lua(),2); 
			VERIFY(start == lua_gettop(lua()));
			return false; //there is no namespace!
		}
		else if (!lua_istable(lua(),-1))
		{ 
			//lua_settop(lua(), 0);
			VERIFY(lua_gettop(lua()) >= 1);
			lua_pop(lua(), 1); 
			VERIFY(start == lua_gettop(lua()));
			R_ASSERT3(false, "Error : the namespace is already being used by the non-table object! Name: ", S);
			return false; 
		} 
		lua_remove(lua(), -2); 
		if (S1)
			S = ++S1; 
		else
			break; 
	} 
	if (!remove_from_stack)
		VERIFY(lua_gettop(lua()) == start + 1);
	else
	{
		VERIFY(lua_gettop(lua()) >= 1);
		lua_pop(lua(), 1);
		VERIFY(lua_gettop(lua()) == start);
	}
	return true; 
}

bool CScriptStorage::object(const char* identifier, int type) //KRodin: fixed
{
	int start = lua_gettop(lua());
	lua_pushnil(lua()); 
	while (lua_next(lua(), -2))
	{ 
		if (lua_type(lua(), -1) == type && !xr_strcmp(identifier, lua_tostring(lua(), -2)))
		{ 
			VERIFY(lua_gettop(lua()) >= 3);
			lua_pop(lua(), 3); 
			VERIFY(lua_gettop(lua()) == start - 1);
			return true; 
		} 
		lua_pop(lua(), 1); 
	} 
	VERIFY(lua_gettop(lua()) >= 1);
	lua_pop(lua(), 1); 
	VERIFY(lua_gettop(lua()) == start - 1);
	return false; 
}

bool CScriptStorage::object(const char* namespace_name, const char* identifier, int type) //KRodin: fixed
{
	int start = lua_gettop(lua());
	if (xr_strlen(namespace_name) && !namespace_loaded(namespace_name, false))
	{
		VERIFY(lua_gettop(lua()) == start);
		return false; 
	}
	bool result = object(identifier, type);
	VERIFY(lua_gettop(lua()) == start);
	return result; 
}

luabind::object CScriptStorage::name_space(const char* namespace_name) //KRodin: fixed
{
	string256 S1;
	xr_strcpy(S1,namespace_name);
	auto S = S1;
	auto lua_namespace = luabind::get_globals(lua());
	for (;;)
	{
		if (!xr_strlen(S))
			return lua_namespace;
		auto I = strchr(S, '.');
		if (!I)
			return lua_namespace[S];
		*I = 0;
		lua_namespace = lua_namespace[S];
		S = I + 1;
	}
}

bool CScriptStorage::print_output(lua_State *L, const char* caScriptFileName, int errorCode) //KRodin: fixed, ���������� �� ���������� ����, � �.�. �� �������� lua_error, lua_pcall_failed, lua_cast_failed, lua_panic
{
	auto Prefix = "";
	if (errorCode) {
		switch (errorCode){
			case LUA_ERRRUN: {
				Prefix = "SCRIPT RUNTIME ERROR";
				break;
			}
			case LUA_ERRMEM: {
				Prefix = "SCRIPT ERROR (memory allocation)";
				break;
			}
			case LUA_ERRERR: {
				Prefix = "SCRIPT ERROR (while running the error handler function)";
				break;
			}
			case LUA_ERRFILE: {
				Prefix = "SCRIPT ERROR (while running file)";
				break;
			}
			case LUA_ERRSYNTAX: {
				Prefix = "SCRIPT SYNTAX ERROR";
				break;
			}
			case LUA_YIELD: {
				Prefix = "Thread is yielded";
				break;
			}
			default: NODEFAULT;
		}
	}

	auto traceback = get_lua_traceback(L, 0);

	if (!lua_isstring(L, -1)) //�� �������! ����� ����� ���e�� ��� ����!
	{
		Msg("----------------------------------------------");
		Msg("[print_output(%s)] %s!\n%s", caScriptFileName, Prefix, traceback);
		Msg("----------------------------------------------");
		return false;
	}

	auto S = lua_tostring(L, -1);
	Msg("----------------------------------------------");
	Msg("[print_output(%s)] %s:\n%s\n%s", caScriptFileName, Prefix, S, traceback);
	Msg("----------------------------------------------");
#ifdef LUAICP_COMPAT
	lua_getglobal(L, "DebugDumpAll");
	lua_pcall(L, 0, 0, -1);
#endif
	return true;
}
