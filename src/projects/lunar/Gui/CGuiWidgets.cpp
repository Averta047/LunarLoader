#include "CGuiWidgets.h"

#include "imgui.h"
#include "imgui_internal.h"

using namespace LunarGui;

bool CGuiWidgets::Begin(const char* name, bool* p_open, PanelFlags flags)
{
	return ImGui::Begin(name, p_open, flags);
}

void CGuiWidgets::End()
{
	ImGui::End();
}

void CGuiWidgets::Label(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
}

bool CGuiWidgets::Button(const char* label, const Vector2& size_arg)
{
	return ImGui::ButtonEx(label, { size_arg.x , size_arg.y }, ImGuiButtonFlags_None);
}

Vector2 CGuiWidgets::GetCurrentWindowPos()
{
	auto pos = ImGui::GetWindowPos();
	return { pos.x , pos.y };
}

Vector2 CGuiWidgets::GetCurrentWindowSize()
{
	auto size = ImGui::GetWindowSize();
	return { size.x , size.y };
}

void CGuiWidgets::SetCurrentWindowPos(const Vector2& pos, Condition cond)
{
	ImGui::SetWindowPos({ pos.x , pos.y }, cond);
}

void CGuiWidgets::SetCurrentWindowSize(const Vector2& size, Condition cond)
{
	ImGui::SetWindowPos({ size.x , size.y }, cond);
}