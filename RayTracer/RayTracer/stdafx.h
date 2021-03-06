#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

//DX12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include "d3dx12.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <random>
#include <vector>
#include <map>
#include <algorithm>

using Microsoft::WRL::ComPtr;

// DXMath
#define _USE_MATH_DEFINES
#include <DirectXMath.h>
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
using DirectX::XMVECTOR;
using DirectX::XMMATRIX;

// OpenMP
#include <omp.h>


