////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine.cpp
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine
//  KRodin: TODO: ��������� �������� � ���������� �� ��� ���� �� script_storage.
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_engine.h"
#include "ai_space.h"
#include "object_factory.h"
#include "../lua_tools.h"
#include "../../build_config_defines.h"

extern void export_classes(lua_State *L);

#ifdef LUAICP_COMPAT
void luaicp_error_handler(lua_State *L)
{
	lua_getglobal(L, "AtPanicHandler");
	if lua_isfunction(L, -1)
		lua_call(L, 0, 0);
	else
		lua_pop(L, 1);
}
#endif

CScriptEngine::CScriptEngine() //fixed
{
	//KRodin: luabind_allocator �������� � ResourceManager_Scripting.cpp, �.� ��� luabind �������� ������ �����.
	//
	m_stack_level = 0;
	m_reload_modules = false;
	m_last_no_file_length = 0;
	*m_last_no_file = 0;
}

CScriptEngine::~CScriptEngine() //fixed
{
	g_game_lua = nullptr;
}

void CScriptEngine::unload() //fixed
{
	lua_settop(lua(), m_stack_level);
	m_last_no_file_length	= 0;
	*m_last_no_file			= 0;
}

int CScriptEngine::lua_panic(lua_State *L) //Fixed
{
	print_output(L, "[CScriptEngine::lua_panic]", LUA_ERRRUN);
	return 0;
}

#ifdef LUABIND_NO_EXCEPTIONS
void CScriptEngine::lua_error(lua_State *L) //Fixed
{
	print_output(L, "", LUA_ERRRUN);

	#ifndef LUAICP_COMPAT   
		auto error = lua_tostring(L, -1);
		Debug.fatal(DEBUG_INFO, "[CScriptEngine::lua_error]: %s", error ? error : "NULL");
	#endif
}
#endif

int CScriptEngine::lua_pcall_failed(lua_State *L) //Fixed
{
	print_output(L, "", LUA_ERRRUN);
	#ifndef LUAICP_COMPAT   
		Debug.fatal(DEBUG_INFO, "[CScriptEngine::lua_pcall_failed]: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
	#endif
	if (lua_isstring(L, -1))
		lua_pop(L, 1);
	return LUA_ERRRUN;
}

#ifdef LUABIND_NO_EXCEPTIONS
void lua_cast_failed(lua_State *L, LUABIND_TYPE_INFO info) //fixed
{
	CScriptEngine::print_output(L, "", LUA_ERRRUN);
	//Debug.fatal(DEBUG_INFO, "LUA error: cannot cast lua value to %s", info.name()); //KRodin: ��� �������� �������� �� ����.
}
#endif

int auto_load(lua_State *L) //fixed
{
	if ((lua_gettop(L) < 2) || !lua_istable(L,1) || !lua_isstring(L,2))
	{
		lua_pushnil(L);
		return 1;
	}
	ai().script_engine().process_file_if_exists(lua_tostring(L, 2), false);
	lua_rawget(L, 1);
	return 1;
}

void CScriptEngine::setup_auto_load() //fixed
{
	lua_pushstring(lua(), GlobalNamespace);
	lua_gettable(lua(), LUA_GLOBALSINDEX); 
	int value_index	= lua_gettop(lua());  // alpet: �� ��������� ���������� � ����� ������ �����������
	luaL_newmetatable(lua(), "XRAY_AutoLoadMetaTable");
	lua_pushstring(lua(), "__index");
	lua_pushcfunction(lua(), auto_load);
	lua_settable(lua(), -3);
	// luaL_getmetatable(lua(), "XRAY_AutoLoadMetaTable");
	lua_setmetatable(lua(), value_index);
}

void CScriptEngine::init() //Fixed
{
	//Msg("[CScriptEngine::init] Starting LuaJIT!");
	lua_State* LSVM = luaL_newstate(); //��������� LuaJIT. ������ ���� �� ������� ���.
	R_ASSERT2(LSVM, "! ERROR : Cannot initialize LUA VM!");
	reinit(LSVM);
	luabind::open(LSVM); //������ ��������
//--------------��������� ��������------------------//
#ifdef LUABIND_NO_EXCEPTIONS
	luabind::set_error_callback(lua_error);
	luabind::set_cast_failed_callback(lua_cast_failed);
#endif
	luabind::set_pcall_callback(lua_pcall_failed); //KRodin: �� ���������������� �� � ���� ������!!!
	lua_atpanic(LSVM, lua_panic);
//-----------------------------------------------------//
	export_classes(LSVM); //��� �������������� ��� ��������� �������, ��������������� � �������
	luaL_openlibs(LSVM); //������������� ������� LuaJIT
	g_game_lua = LSVM; //KRodin: �� �������, ����� ��� � �����.
	setup_auto_load(); //���������� �����������
	bool save = m_reload_modules;
	m_reload_modules = true;
	process_file_if_exists(GlobalNamespace, false); //���������� _G.script
	m_reload_modules = save;

	m_stack_level = lua_gettop(LSVM); //?

	register_script_classes(); //������, ������ class_registrator.script
	object_factory().register_script(); //����������� �������
	//Msg("[CScriptEngine::init] LuaJIT Started!");
}

void CScriptEngine::parse_script_namespace(const char *name, char *ns, u32 nsSize, char *func, u32 funcSize) //Fixed
{
	auto p = strrchr(name, '.');
	if (!p)
	{
		xr_strcpy(ns, nsSize, GlobalNamespace);
		p = name - 1;
	}
	else
	{
		VERIFY(u32(p - name + 1) <= nsSize);
		strncpy(ns, name, p - name);
		ns[p - name] = 0;
	}
	xr_strcpy(func, funcSize, p + 1);
}

bool CScriptEngine::process_file_if_exists(const char* file_name, bool warn_if_not_exist) //KRodin: ������� ��������� ���������� �� ������ �� �����. ���� ���������� - ���������� ��� � do_file. ���������� �� process_file, auto_load � �� ������.
{
#ifdef DEBUG
	Msg("[CScriptEngine::process_file_if_exists] loading file: [%s]", file_name); //�������� ����� ����������... ���� ���-�� � ���� ������.
#endif
	u32 string_length = xr_strlen(file_name);
	if (!warn_if_not_exist && no_file_exists(file_name, string_length)) //��� ������ ��� ����������� ������, ���� ���� ���� ���� ��� ��������� ��� ����� ���, ��������� �� ���������, ���� �� ��.
		return false;
	string_path S,S1;
	if (m_reload_modules || (*file_name && !namespace_loaded(file_name)))
	{
		FS.update_path(S, "$game_scripts$", strconcat(sizeof(S1), S1, file_name, ".script"));
		if (!warn_if_not_exist && !FS.exist(S))
		{
#ifdef DEBUG
			Msg("-------------------------");
			Msg("[CScriptEngine::process_file_if_exists] WARNING: Access to nonexistent variable '%s' or loading nonexistent script '%s'", file_name, S1);
			print_stack();
			Msg("-------------------------");
#endif
			add_no_file(file_name, string_length);
			return false;
		}
#ifdef DEBUG
		Msg("[CScriptEngine::process_file_if_exists] loading script: [%s]", S1);
#endif
		m_reload_modules = false;
		do_file(S, *file_name ? file_name : GlobalNamespace);
	}
	return true;
}

bool CScriptEngine::process_file(const char* file_name) //fixed
{ return process_file_if_exists(file_name, true); }

bool CScriptEngine::process_file(const char* file_name, bool reload_modules) //fixed
{
	m_reload_modules = reload_modules;
	bool result = process_file_if_exists(file_name, true);
	m_reload_modules = false;
	return result;
}

void CScriptEngine::register_script_classes()
{
	string_path					S;
	FS.update_path				(S,"$game_config$","script.ltx");
	CInifile					*l_tpIniFile = xr_new<CInifile>(S);
	R_ASSERT					(l_tpIniFile);

	if (!l_tpIniFile->section_exist("common")) {
		xr_delete				(l_tpIniFile);
		return;
	}

	shared_str m_class_registrators		= READ_IF_EXISTS(l_tpIniFile,r_string,"common","class_registrators","");
	xr_delete					(l_tpIniFile);

	u32							n = _GetItemCount(*m_class_registrators);
	string256					I;
	for (u32 i=0; i<n; ++i) {
		_GetItem				(*m_class_registrators,i,I);
		luabind::functor<void>	result;
		if (!functor(I,result)) {
			Msg("!![CScriptEngine::register_script_classes()] Cannot load class registrator [%s]!", I);
			continue;
		}
		result					(const_cast<CObjectFactory*>(&object_factory()));
	}
}

bool CScriptEngine::function_object(const char* function_to_call, luabind::object &object, int type) //fixed
{
	if (!xr_strlen(function_to_call))
		return false;
	string256 name_space, function;
	parse_script_namespace(function_to_call, name_space, sizeof(name_space), function, sizeof(function));
	if (xr_strcmp(name_space, GlobalNamespace))
	{
		auto file_name = strchr(name_space, '.');
		if (!file_name)
			process_file(name_space);
		else
		{
			*file_name = 0;
			process_file(name_space);
			*file_name = '.';
		}
	}

	if (!this->object(name_space, function, type))
		return false;
	auto lua_namespace = this->name_space(name_space);
	object = lua_namespace[function];
	return true;
}

bool CScriptEngine::no_file_exists(const char* file_name, u32 string_length) //fixed
{
	if (m_last_no_file_length != string_length)
		return false;
	return !memcmp(m_last_no_file, file_name, string_length);
}

void CScriptEngine::add_no_file(const char* file_name, u32 string_length) //fixed
{
	m_last_no_file_length = string_length;
	std::memcpy(m_last_no_file, file_name, string_length + 1);
}

void log_script_error(const char* format, ...) //���������� �� ���� ����, ���� � ������� �������. �� ���� ����� ���������.
{
	string1024 line_buf;
	va_list mark;	
	va_start(mark, format);
	int sz = _vsnprintf(line_buf, sizeof(line_buf)-1, format, mark); 	
	line_buf[sizeof(line_buf) - 1] = 0;
	va_end(mark);

	ai().script_engine().script_log(ScriptStorage::ELuaMessageType::eLuaMessageTypeError, line_buf);
}
 
DLL_API lua_State* game_lua() //Fixed
{
	R_ASSERT(g_game_lua);
	return g_game_lua;
}

void CScriptEngine::collect_all_garbage()
{
	lua_gc(lua(), LUA_GCCOLLECT, 0);
	lua_gc(lua(), LUA_GCCOLLECT, 0);
	lua_gc(lua(), LUA_GCCOLLECT, 0);
}
