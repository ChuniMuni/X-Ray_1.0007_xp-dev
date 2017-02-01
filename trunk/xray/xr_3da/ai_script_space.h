//KRodin: � ���� ����� ������������ Luabind. ������ ����� � ������ �����.
#pragma once

#pragma comment( lib, "luabind.lib"	)

#pragma warning(push)

#pragma warning(disable:4244) // ��������: �������������� "..." � "...", �������� ������ ������
#pragma warning(disable:4995) // "...": ��� �������� ��� #pragma deprecated
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4456) // declaration of 'x' hides previous local declaration
#pragma warning(disable:4458) // declaration of 'x' hides class member
#pragma warning(disable:4459) // declaration of 'x' hides global declaration
#pragma warning(disable:4913) // user defined binary operator 'x' exists but no overload could convert all operands
#pragma warning(disable:4297) // function assumed not to throw exception but does
#pragma warning(disable:4275) // class "...", �� ���������� dll-�����������, ����������� � �������� �������� ��� "..."
#pragma warning(disable:4389) // �������������� ����� �� ������ � ��� �����
#pragma warning(disable:4101) // "...": ���������������� ��������� ����������
#pragma warning(disable:4702) // ������������ ���

extern "C" {
# include <lualib.h>
}

#include <luabind/luabind.hpp>
#include <luabind/class.hpp>
#include <luabind/object.hpp>
#include <luabind/operator.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/iterator_policy.hpp>
#include "xrScriptEngine/Functor.hpp"

#pragma warning(pop)
