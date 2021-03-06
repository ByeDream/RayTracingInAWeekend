#include "stdafx.h"
#include "RayTracer.h"

#include "D3D12Viewer.h"
#include "OutputImage.h"
#include "InputListener.h"
#include "HomemadeRayTracer.h"
#include "SimpleCamera.h"
#include "World.h"
#include "StepTimer.h"

using namespace std;

#define MAX_LOADSTRING 100
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

HINSTANCE hInst;
char szTitle[MAX_LOADSTRING];
char szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL				Finalize();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

D3D12Viewer *g_d3dViewer = nullptr;
OutputImage *g_outputImage = nullptr;
InputListener *g_inputListener = nullptr;
HomemadeRayTracer *g_hmRayTracer = nullptr;
SimpleCamera *g_camera{ nullptr };
World *g_world{ nullptr };
StepTimer *g_timer{ nullptr };

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	freopen_s(&stream, "CONOUT$", "r", stdin);

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_RAYTRACER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Finalize();

	FreeConsole();

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RAYTRACER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_RAYTRACER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	cout << "Initialize Program ..." << endl;

	hInst = hInstance;

	HWND hWnd = CreateWindow(
		szWindowClass, 
		szTitle, 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WINDOW_WIDTH, WINDOW_HEIGHT, 
		nullptr, 
		nullptr, hInstance, 
		nullptr);
	
	if (!hWnd)
	{
		return FALSE;
	}


	g_inputListener = new InputListener();
	g_outputImage = new OutputImage(WINDOW_WIDTH, WINDOW_HEIGHT, "OutputImage");

	g_world = new World();
	g_camera = new SimpleCamera(g_world, g_inputListener, g_outputImage->m_aspectRatio);
	g_world->ConstructWorld(WORLD_ID_CORNELL_BOX, g_camera);
	g_camera->HelpInfo();
	g_hmRayTracer = new HomemadeRayTracer(g_inputListener, g_outputImage, g_world);
	g_hmRayTracer->OnInit();
	g_hmRayTracer->HelpInfo();

	g_d3dViewer = new D3D12Viewer(hWnd, g_outputImage, g_inputListener, g_hmRayTracer, g_world);
	g_d3dViewer->OnInit();
	g_d3dViewer->HelpInfo();

	g_timer = new StepTimer();

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	cout << "Done" << endl;
	return TRUE;
}


BOOL Finalize()
{
	cout << "Finalize Program ..." << endl;
	if (g_timer)
	{
		delete g_timer;
		g_timer = nullptr;
	}
	if (g_d3dViewer)
	{
		g_d3dViewer->OnDestroy();
		delete g_d3dViewer;
		g_d3dViewer = nullptr;
	}
	
	if (g_hmRayTracer)
	{
		g_hmRayTracer->OnDestroy();
		delete g_hmRayTracer;
		g_hmRayTracer = nullptr;
	}
	if (g_camera)
	{
		delete g_camera;
		g_camera = nullptr;
	}
	if (g_world)
	{
		g_world->DeconstructWorld();
		delete g_world;
		g_world = nullptr;
	}
	if (g_outputImage)
	{
		delete g_outputImage;
		g_outputImage = nullptr;
	}

	if (g_inputListener)
	{
		delete g_inputListener;
		g_inputListener = nullptr;
	}
	cout << "Done" << endl;
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_KEYDOWN:
		if (g_inputListener)
		{
			g_inputListener->NotifyKeyDown(static_cast<UINT8>(wParam));
		}
		break;
	case WM_KEYUP:
		if (g_inputListener)
		{
			g_inputListener->NotifyKeyUp(static_cast<UINT8>(wParam));
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
		if (g_timer)
		{
			g_timer->Tick(nullptr);
			static UINT32 FPSUpdateCounter = 0;
			if (FPSUpdateCounter == 20)
			{
				// Update window text with FPS value.
				char fps[32];
				sprintf_s(fps, "%ufps", g_timer->GetFramesPerSecond());

				std::string windowText = std::string(szTitle) + ": " + std::string(fps);
				SetWindowText(hWnd, windowText.c_str());
				FPSUpdateCounter = 0;
			}
			FPSUpdateCounter++;
		}
		if (g_camera && g_d3dViewer->GetMode() == VMODE_SCENE_VIEWER)
		{
			g_camera->OnUpdate(static_cast<float>(g_timer->GetElapsedSeconds()));
		}
		if (g_world && g_d3dViewer->GetMode() == VMODE_SCENE_VIEWER)
		{
			g_world->OnUpdate(g_camera, static_cast<float>(g_timer->GetElapsedSeconds()));
		}
		if (g_hmRayTracer)
		{
			g_hmRayTracer->OnUpdate(g_camera, g_outputImage);
		}
		if (g_d3dViewer)
		{
			g_d3dViewer->OnUpdate();
			g_d3dViewer->OnRender();
		}

		if (g_inputListener)
		{
			g_inputListener->Clear();
		}
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
