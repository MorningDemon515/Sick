#include <iostream>
#include <math.h>
#include <windows.h>

#include <ddraw.h>
#include <d3d.h>

//加载图片
#define FREEIMAGE_LIB
#include "../freeimage241/Source/FreeImage.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool IsDarkThemeByColor();

typedef HRESULT (WINAPI *PDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
void SetDarkMode(HWND hwnd);

//顶点格式
struct Vertex
{
	float x, y, z;
	float u, v;
};
#define VERTEX_FVF (D3DFVF_XYZ | D3DFVF_TEX1)

//枚举设备，寻找支持Z缓存深度的格式
HRESULT WINAPI EnumZBufferCallback(DDPIXELFORMAT* lpDDPixelFormat, VOID* lpContext) {
    DDPIXELFORMAT* lpBestFormat = (DDPIXELFORMAT*)lpContext;
 
    if (lpDDPixelFormat->dwZBufferBitDepth == 16) {
        *lpBestFormat = *lpDDPixelFormat;
        return D3DENUMRET_CANCEL;
    }
    return D3DENUMRET_OK;
}

int main()
{
	FreeImage_Initialise();

	//没有WinMain的懒人做法
	HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOWDEFAULT;

	//HDC hDC = NULL;

	WNDCLASS wc;

	wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = 0;
    wc.lpszClassName = "Window Class";

	 RegisterClass(&wc);

	 //调整客户端区为正确的大小
	 RECT wr = { 0, 0, 800, 600 }; 
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	 HWND windowHandle = CreateWindowA(
        "Window Class",
        "Sick - Direct3D 7",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wr.right - wr.left,
        wr.bottom - wr.top,
        0,
        0,
        hInstance,
        0
    );

	//不能自由调整窗口大小
    DWORD dwStyle = GetWindowLong(windowHandle, GWL_STYLE);
	dwStyle ^= WS_SIZEBOX | WS_MAXIMIZEBOX;

	SetWindowLong(windowHandle, GWL_STYLE, dwStyle);

	//将窗口移到中央
	int cx = GetSystemMetrics(SM_CXSCREEN);
	int cy = GetSystemMetrics(SM_CYSCREEN);

	MoveWindow(windowHandle, (cx - 800)/2, (cy - 600)/2, 800, 600, TRUE);

	if(IsDarkThemeByColor())
    {
        SetDarkMode(windowHandle);
    }

	UpdateWindow(windowHandle);
	ShowWindow(windowHandle,nCmdShow);

	//DirectDraw COM组件
	LPDIRECTDRAW7        lpDD = NULL;
    LPDIRECTDRAWSURFACE7 lpDDSPrimary = NULL;
    LPDIRECTDRAWSURFACE7 lpDDSBack = NULL;
	LPDIRECTDRAWCLIPPER lpDDClipper = NULL;

	DirectDrawCreateEx(NULL, (VOID**)&lpDD, IID_IDirectDraw7, NULL);
	lpDD->SetCooperativeLevel(windowHandle, DDSCL_NORMAL);//窗口模式
	//lpDD->SetDisplayMode(800, 600, 32, 0, 0); //用于全屏

    DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL);//创建主表面

	lpDD->CreateClipper(0, &lpDDClipper, NULL);
	lpDDClipper->SetHWnd(0, windowHandle);
	lpDDSPrimary->SetClipper(lpDDClipper);
	lpDDClipper->Release();//创建裁剪器，确保画面显示正确

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = 800;
	ddsd.dwHeight = 600;
	lpDD->CreateSurface(&ddsd, &lpDDSBack, NULL);//创建后缓存

	//Direct3D 7 COM组件
	LPDIRECT3D7       lpD3D = NULL;
	LPDIRECT3DDEVICE7 lpD3DDevice = NULL;

	lpDD->QueryInterface(IID_IDirect3D7, (VOID**)&lpD3D);

	DDPIXELFORMAT z_pf;
	ZeroMemory(&z_pf, sizeof(z_pf));
	z_pf.dwSize = sizeof(z_pf);

	lpD3D->EnumZBufferFormats(IID_IDirect3DHALDevice, EnumZBufferCallback, (VOID*)&z_pf);

	DDSURFACEDESC2 ddsdZ;
	ZeroMemory(&ddsdZ, sizeof(ddsdZ));
	ddsdZ.dwSize = sizeof(ddsdZ);
	ddsdZ.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsdZ.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
	ddsdZ.dwWidth  = 800;
	ddsdZ.dwHeight = 600;
	ddsdZ.ddpfPixelFormat = z_pf;

	LPDIRECTDRAWSURFACE7 lpDDSZBuffer = NULL;
	lpDD->CreateSurface(&ddsdZ, &lpDDSZBuffer, NULL);//创建Z缓存

    DDSURFACEDESC2 ddsd_current;
    ZeroMemory(&ddsd_current, sizeof(ddsd_current));
    ddsd_current.dwSize = sizeof(ddsd_current);
    lpDD->GetDisplayMode(&ddsd_current);

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat = ddsd_current.ddpfPixelFormat;
	lpDD->CreateSurface(&ddsd, &lpDDSBack, NULL);//创建支持3D画面的后缓存

	lpDDSBack->AddAttachedSurface(lpDDSZBuffer);//Z缓存绑定到后缓存

	lpD3D->CreateDevice(IID_IDirect3DHALDevice, lpDDSBack, &lpD3DDevice);//创建Direct3D 7设备

	//正方体顶点和索引
    Vertex vertices[] = {

    {-0.5f,  0.5f, -0.5f,  0.0f, 0.0f},
    { 0.5f,  0.5f, -0.5f,  1.0f, 0.0f}, 
    { 0.5f, -0.5f, -0.5f,  1.0f, 1.0f}, 
    {-0.5f, -0.5f, -0.5f,  0.0f, 1.0f}, 

    {-0.5f,  0.5f,  0.5f,  1.0f, 0.0f}, 
    { 0.5f,  0.5f,  0.5f,  0.0f, 0.0f}, 
    { 0.5f, -0.5f,  0.5f,  0.0f, 1.0f}, 
    {-0.5f, -0.5f,  0.5f,  1.0f, 1.0f},

    {-0.5f,  0.5f,  0.5f,  0.0f, 0.0f},
    { 0.5f,  0.5f,  0.5f,  1.0f, 0.0f},
    { 0.5f,  0.5f, -0.5f,  1.0f, 1.0f},
    {-0.5f,  0.5f, -0.5f,  0.0f, 1.0f},

    {-0.5f, -0.5f,  0.5f,  0.0f, 1.0f},
    { 0.5f, -0.5f,  0.5f,  1.0f, 1.0f},
    { 0.5f, -0.5f, -0.5f,  1.0f, 0.0f},
    {-0.5f, -0.5f, -0.5f,  0.0f, 0.0f},

    {-0.5f,  0.5f,  0.5f,  0.0f, 0.0f},
    {-0.5f,  0.5f, -0.5f,  1.0f, 0.0f},
    {-0.5f, -0.5f, -0.5f,  1.0f, 1.0f},
    {-0.5f, -0.5f,  0.5f,  0.0f, 1.0f},

    { 0.5f,  0.5f,  0.5f,  1.0f, 0.0f},
    { 0.5f,  0.5f, -0.5f,  0.0f, 0.0f},
    { 0.5f, -0.5f, -0.5f,  0.0f, 1.0f},
    { 0.5f, -0.5f,  0.5f,  1.0f, 1.0f},
	};

	WORD indices[] = {
		0,  1,  2,  0,  2,  3,
    4,  6,  5,  4,  7,  6, 
    8,  9,  10, 8,  10, 11, 
    12, 14, 13, 12, 15, 14,
    16, 17, 18, 16, 18, 19,
    20, 22, 21, 20, 23, 22 
	};

	//创建纹理
	LPDIRECTDRAWSURFACE7 lpTexture = NULL;

    const char* imagepath = "image.jpg";

	FIBITMAP* dib = NULL;
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(imagepath, 0);
    dib = FreeImage_Load(fif, imagepath);

	FIBITMAP* temp = FreeImage_ConvertTo32Bits(dib);
	FreeImage_Unload(dib);
	dib = temp;

	int im_width  = FreeImage_GetWidth(dib);
	int im_height = FreeImage_GetHeight(dib);

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwWidth  = im_width;
	ddsd.dwHeight = im_height;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;

	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
	ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
	ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;

	lpDD->CreateSurface(&ddsd, &lpTexture, NULL);

	//将加载的图像写入纹理
	lpTexture->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);

	BYTE* pDest = (BYTE*)ddsd.lpSurface;  
    BYTE* pSrc  = (BYTE*)FreeImage_GetBits(dib);
    
    int pitchDest = ddsd.lPitch;             
    int pitchSrc  = FreeImage_GetPitch(dib);  

    for (int y = 0; y < im_height; y++)
    {
        BYTE* srcLine = pSrc + (im_height - 1 - y) * pitchSrc;
        BYTE* dstLine = pDest + y * pitchDest;

        memcpy(dstLine, srcLine, im_width * 4);
    }

    lpTexture->Unlock(NULL);

	//设置MVP矩阵
	D3DMATRIX identity = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	D3DMATRIX matWorld = identity;
	D3DMATRIX matView = identity;

    matView._43 = 3.0f;

	D3DMATRIX matProj = identity;

    float aspect = 1.3333333333f;// 800 / 600
	float fov = 0.41421356f;// tan(45° / 2)
	float n = 0.1f, f = 100.0f;

	matProj._11 = 1.0f / (aspect * fov);
	matProj._22 = 1.0f / (fov);
	matProj._33 = f / (f - n);
	matProj._43 = -(n * f) / (f - n);
	matProj._34 = 1.0f;
	matProj._44 = 0.0f;

	D3DVIEWPORT7 vp;
	vp.dwX = 0;
	vp.dwY = 0;
	vp.dwWidth = 800;
	vp.dwHeight = 600;
	vp.dvMinZ = 0.0f;
	vp.dvMaxZ = 1.0f;

	lpD3DDevice->SetViewport(&vp);

	MSG msg;
    bool run = true;
	float angle = 0.0f;
    while(run)
    {
        if(PeekMessage(&msg,0,0,0,PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                run = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
			/*
			DDBLTFX ddbltfx;
			ZeroMemory(&ddbltfx, sizeof(ddbltfx));
			ddbltfx.dwSize = sizeof(ddbltfx);
			ddbltfx.dwFillColor = RGB(0, 0, 0);

            lpDDSBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
            */

			lpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DRGBA(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
			lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);//设置Z-Buffer
			lpD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);//关闭光照

			lpD3DDevice->BeginScene();

			//设置纹理
			lpD3DDevice->SetTexture(0, lpTexture);

			lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
			lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

			lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
			lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_LINEAR);

			angle += 0.001f;
			float s = sinf(angle);
            float c = cosf(angle);
			matWorld._11 = c;
			matWorld._13 = s;
			matWorld._31 = -s;
			matWorld._33 = c;

			//设置MVP矩阵
			lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &matWorld);
			lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);
			lpD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);

			//lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, VERTEX_FVF, vertices, 3, 0);
			lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, VERTEX_FVF, vertices, 24, indices, 36, 0);//索引渲染

			lpD3DDevice->EndScene();
    

			RECT rcDest;
            GetClientRect(windowHandle, &rcDest);
            ClientToScreen(windowHandle, (LPPOINT)&rcDest.left);
            ClientToScreen(windowHandle, (LPPOINT)&rcDest.right);

			lpDDSPrimary->Blt(&rcDest, lpDDSBack, NULL, DDBLT_WAIT, NULL);//显示后缓存图像
            
        }

    }

    lpTexture->Release();

    lpD3D->Release();
	lpD3DDevice->Release();

	lpDD->Release();
	lpDDSPrimary->Release();
	lpDDSBack->Release();
	lpDDSZBuffer->Release();
//释放内存
	UnregisterClass("Window Class", hInstance);
	FreeImage_DeInitialise();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
		break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

bool IsDarkThemeByColor() {
    DWORD value = 1;
    DWORD size = sizeof(DWORD);

    HKEY hKey;
    if (RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_READ,
        &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExA(
            hKey,
            "AppsUseLightTheme",
            NULL,
            NULL,
            (LPBYTE)&value,
            &size);

        RegCloseKey(hKey);
    }

    return value == 0;
}

void SetDarkMode(HWND hwnd)
{
    HMODULE hDwmapi = LoadLibrary(TEXT("dwmapi.dll"));
    if (hDwmapi) {
        PDwmSetWindowAttribute DwmSetWindowAttribute = (PDwmSetWindowAttribute)GetProcAddress(hDwmapi, "DwmSetWindowAttribute");
        if (DwmSetWindowAttribute) {
            BOOL useDarkMode = TRUE;
            DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

        }
        FreeLibrary(hDwmapi);
    }
}
