// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Third generation by Oles.

#ifndef stdafxH
#define stdafxH

#pragma once

#include "../xrCore/xrCore.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include "include\DPlay\dplay8.h" //KRodin: ������� ����������� ����
#include "dxerr.h" //KRodin: ������ ���� �� NET_Client.cpp � NET_Server.cpp
#ifdef DEBUG
	#pragma comment (lib, "dxerr.lib") //KRodin: ����� ���������� ���, ��� ����� Additional Dependencies
	int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf; //KRodin: ����������� �������� : http://stackoverflow.com/questions/31053670/unresolved-external-symbol-vsnprintf-in-dxerr-lib
#endif
#pragma warning(pop)
#include "NET_Shared.h"

#define _RELEASE(x)			{ if(x) { (x)->Release();       (x)=NULL; } }
#define _SHOW_REF(msg, x)   { if(x) { x->AddRef(); Log(msg,u32(x->Release()));}}

#endif //stdafxH
