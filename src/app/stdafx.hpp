#pragma once

//System
#include <string>
#include <vector>
#include <algorithm>

#include <Windows.h>
#include <commdlg.h>

//Deps
#include <imgui_freetype.h>

#ifndef OVERLAY
#include <SDL.h>
#include <SDL_syswm.h>

#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_sdlrenderer.h>
#else
#include <MinHook.h>
#include <kiero.h>

#define KIERO_INCLUDE_D3D9 1
#define KIERO_INCLUDE_D3D10 1
#define KIERO_INCLUDE_D3D11 1
#define KIERO_INCLUDE_OPENGL 1
#define KIERO_USE_MINHOOK 1

#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_dx10.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_opengl3.h>
#endif