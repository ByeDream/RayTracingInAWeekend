#pragma once

#include "SimpleMesh.h"

namespace
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	inline D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(PrimitiveType type)
	{
		D3D12_PRIMITIVE_TOPOLOGY ret = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		switch (type)
		{
		case kPrimitiveTypePoints:
			ret = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case kPrimitiveTypeLineList:
			ret = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case kPrimitiveTypeLineStrip:
			ret = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case kPrimitiveTypeTriList:
			ret = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case kPrimitiveTypeTriStrip:
			ret = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		}
		return ret;
	}

	inline void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
	{
		ComPtr<IDXGIAdapter1> adapter;
		*ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*ppAdapter = adapter.Detach();
	}

	// Assign a name to the object to aid with debugging.
#if defined(_DEBUG)
	inline void SetName(ID3D12Object* pObject, LPCWSTR name)
	{
		pObject->SetName(name);
	}
	inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
	{
		WCHAR fullName[50];
		if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		{
			pObject->SetName(fullName);
		}
	}
#else
	inline void SetName(ID3D12Object*, LPCWSTR)
	{
	}
	inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
	{
	}
#endif
}

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName(x.Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed(x[n].Get(), L#x, n)
