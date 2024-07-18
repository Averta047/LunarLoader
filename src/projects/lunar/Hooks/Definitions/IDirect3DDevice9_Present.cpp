#include "../Hooks.h"

DEFINE_HOOK(Direct3DDevice9_Present, void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion)
{
	Func.Original<FN>()(ecx, edx, pDevice, pSrcRect, pDestRect, hDestWindow, pDirtyRegion);
}