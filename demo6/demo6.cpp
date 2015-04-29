#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
#include <math.h>
#include <tchar.h>
#include "Defines.h"
#include "DirectInput.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

#define PI	3.141592654f
#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600
#define WINDOW_TITLE    TEXT("DirectX_3D_Demo")

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_TEX1)  //FVF�����ʽ

struct CUSTOMVERTEX
{
	float x, y, z;
	float u, v;
	CUSTOMVERTEX(float _x, float _y, float _z, float _u, float _v)
		:x(_x), y(_y), z(_z), u(_u), v(_v)
	{

	}
};

IDirect3DDevice9*		g_pDevice = 0;

LPD3DXFONT				g_pTextFPS = 0;
LPD3DXFONT				g_pTextAdaperName = 0;
LPD3DXFONT				g_pTextHelper = 0;
LPD3DXFONT				g_pTextInfo = 0;

TCHAR					g_strFPS[64] = { 0 };
TCHAR					g_strAdaperName[64] = { 0 };
D3DXMATRIX				g_matWorld;

LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;
LPDIRECT3DINDEXBUFFER9  g_pIndexBuffer = NULL;
LPDIRECT3DTEXTURE9		g_pMipTexture = NULL;

DirectInput*			g_pInputs = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT InitD3D(HWND hWnd, HINSTANCE hInstance);
HRESULT InitObject(void);
void OnSize(int w, int h);
void SetMatrix(void);
float GetFPS(void);
void Render(HWND hWnd);
void Update(HWND hWnd);
void Cleanup(void);


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WNDCLASSEX ws;
	memset(&ws, 0, sizeof(WNDCLASSEX));

	ws.cbSize = sizeof(WNDCLASSEX);
	ws.style = CS_HREDRAW | CS_VREDRAW;
	ws.lpfnWndProc = WndProc;
	ws.cbClsExtra = 0;
	ws.cbWndExtra = 0;
	ws.hInstance = hInstance;
	ws.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ws.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	ws.hCursor = LoadCursor(NULL, IDC_ARROW);
	ws.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	ws.lpszMenuName = NULL;
	ws.lpszClassName = TEXT("DirectX_3D_WIN");

	if (!RegisterClassEx(&ws))
	{
		return -1;
	}

	HWND hWnd = CreateWindow(TEXT("DirectX_3D_WIN")
		, WINDOW_TITLE
		, WS_OVERLAPPEDWINDOW
		, CW_USEDEFAULT
		, CW_USEDEFAULT
		, SCREEN_WIDTH
		, SCREEN_HEIGHT
		, NULL
		, NULL
		, hInstance
		, NULL);

	if (hWnd == NULL || hWnd == INVALID_HANDLE_VALUE)
	{
		return -2;
	}

	if (FAILED(InitD3D(hWnd, hInstance)))
	{
		return -3;
	}

	MoveWindow(hWnd, 200, 200, SCREEN_WIDTH, SCREEN_HEIGHT, true);
	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

	g_pInputs = new DirectInput;
	g_pInputs->Initialize(hWnd, hInstance, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update(hWnd);
			Render(hWnd);
		}
	}

	UnregisterClass(TEXT("DirectX_3D_WIN"), ws.hInstance);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_PAINT:
		Render(hWnd);
		ValidateRect(hWnd, NULL);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hWnd);
		}
		break;
	case WM_SIZE:
		OnSize((short)LOWORD(lParam), (short)HIWORD(lParam));
		break;
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void OnSize(int w, int h)
{
	TCHAR szBuffer[64] = { 0 };
	_stprintf_s(szBuffer, TEXT("w=%d, h=%d\n"), w, h);
	OutputDebugString(szBuffer);
}

HRESULT InitD3D(HWND hWnd, HINSTANCE hInstance)
{
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL) return E_FAIL;

	int vp = 0;
	D3DCAPS9 caps;
	if (FAILED(pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
	{
		return E_FAIL;
	}

	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else
	{
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 2;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = true;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vp, &d3dpp, &g_pDevice)))
	{
		return E_FAIL;
	}

	TCHAR szName[64] = TEXT("��ǰ�Կ��ͺ�:");
	D3DADAPTER_IDENTIFIER9 adapter;
	pD3D->GetAdapterIdentifier(0, 0, &adapter);
	int len = MultiByteToWideChar(CP_ACP, 0, adapter.Description, -1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, adapter.Description, -1, g_strAdaperName, len);
	_tcscat_s(szName, g_strAdaperName);
	_tcscpy_s(g_strAdaperName, szName);

	if (InitObject() != S_OK)
	{
		SAFE_RELEASE(pD3D);
		return E_FAIL;
	}

	SAFE_RELEASE(pD3D);
	return S_OK;
}

HRESULT InitObject(void)
{
	srand(unsigned(time(NULL)));

	//��������
	if (FAILED(D3DXCreateFont(g_pDevice, 36, 0, 0, 1000, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, TEXT("Calibri"), &g_pTextFPS)))
	{
		return E_FAIL;
	}

	if (FAILED(D3DXCreateFont(g_pDevice, 20, 0, 1000, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, TEXT("��������"), &g_pTextAdaperName)))
	{
		return E_FAIL;
	}

	if (FAILED(D3DXCreateFont(g_pDevice, 23, 0, 1000, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, TEXT("΢���ź�"), &g_pTextHelper)))
	{
		return E_FAIL;
	}

	if (FAILED(D3DXCreateFont(g_pDevice, 26, 0, 1000, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, TEXT("����"), &g_pTextInfo)))
	{
		return E_FAIL;
	}
	//�������㻺��
	if (FAILED(g_pDevice->CreateVertexBuffer(24 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL;
	}
	//������������
	if (FAILED(g_pDevice->CreateIndexBuffer(36 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIndexBuffer, NULL)))
	{
		return E_FAIL;
	}

	CUSTOMVERTEX * pVertices = NULL;
	if (FAILED(g_pVertexBuffer->Lock(0, sizeof(CUSTOMVERTEX), (void**)&pVertices, 0)))
	{
		return E_FAIL;
	}

	// ���涥������
	pVertices[0] = CUSTOMVERTEX(-10.0f, 10.0f, -10.0f, 0.0f, 0.0f);
	pVertices[1] = CUSTOMVERTEX(10.0f, 10.0f, -10.0f, 2.0f, 0.0f);
	pVertices[2] = CUSTOMVERTEX(10.0f, -10.0f, -10.0f, 2.0f, 2.0f);
	pVertices[3] = CUSTOMVERTEX(-10.0f, -10.0f, -10.0f, 0.0f, 2.0f);

	// ���涥������
	pVertices[4] = CUSTOMVERTEX(10.0f, 10.0f, 10.0f, 0.0f, 0.0f);
	pVertices[5] = CUSTOMVERTEX(-10.0f, 10.0f, 10.0f, 2.0f, 0.0f);
	pVertices[6] = CUSTOMVERTEX(-10.0f, -10.0f, 10.0f, 2.0f, 2.0f);
	pVertices[7] = CUSTOMVERTEX(10.0f, -10.0f, 10.0f, 0.0f, 2.0f);

	// ���涥������
	pVertices[8] = CUSTOMVERTEX(-10.0f, 10.0f, 10.0f, 0.0f, 0.0f);
	pVertices[9] = CUSTOMVERTEX(10.0f, 10.0f, 10.0f, 2.0f, 0.0f);
	pVertices[10] = CUSTOMVERTEX(10.0f, 10.0f, -10.0f, 2.0f, 2.0f);
	pVertices[11] = CUSTOMVERTEX(-10.0f, 10.0f, -10.0f, 0.0f, 2.0f);

	// ���涥������
	pVertices[12] = CUSTOMVERTEX(-10.0f, -10.0f, -10.0f, 0.0f, 0.0f);
	pVertices[13] = CUSTOMVERTEX(10.0f, -10.0f, -10.0f, 2.0f, 0.0f);
	pVertices[14] = CUSTOMVERTEX(10.0f, -10.0f, 10.0f, 2.0f, 2.0f);
	pVertices[15] = CUSTOMVERTEX(-10.0f, -10.0f, 10.0f, 0.0f, 2.0f);

	// ����涥������
	pVertices[16] = CUSTOMVERTEX(-10.0f, 10.0f, 10.0f, 0.0f, 0.0f);
	pVertices[17] = CUSTOMVERTEX(-10.0f, 10.0f, -10.0f, 1.0f, 0.0f);
	pVertices[18] = CUSTOMVERTEX(-10.0f, -10.0f, -10.0f, 1.0f, 1.0f);
	pVertices[19] = CUSTOMVERTEX(-10.0f, -10.0f, 10.0f, 0.0f, 1.0f);

	// �Ҳ��涥������
	pVertices[20] = CUSTOMVERTEX(10.0f, 10.0f, -10.0f, 0.0f, 0.0f);
	pVertices[21] = CUSTOMVERTEX(10.0f, 10.0f, 10.0f, 1.0f, 0.0f);
	pVertices[22] = CUSTOMVERTEX(10.0f, -10.0f, 10.0f, 1.0f, 1.0f);
	pVertices[23] = CUSTOMVERTEX(10.0f, -10.0f, -10.0f, 0.0f, 1.0f);

	g_pVertexBuffer->Unlock();

	WORD * pIndices = NULL;
	if (FAILED(g_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0)))
	{
		return E_FAIL;
	}

	// ������������
	pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
	pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

	// ������������
	pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
	pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

	// ������������
	pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
	pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

	// ������������
	pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
	pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

	// �������������
	pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
	pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

	// �Ҳ�����������
	pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
	pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;

	g_pIndexBuffer->Unlock();

	D3DXCreateTextureFromFileEx(g_pDevice
		, TEXT("tex.jpg")
		, 0, 0, 6, 0
		, D3DFMT_X8R8G8B8
		, D3DPOOL_MANAGED
		, D3DX_DEFAULT
		, D3DX_DEFAULT
		, 0xff000000
		, 0, 0, &g_pMipTexture);

	//���ò���
	D3DMATERIAL9 material;
	memset(&material, 0, sizeof(D3DMATERIAL9));
	material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	g_pDevice->SetMaterial(&material);

	//������Ⱦ״̬
	g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	g_pDevice->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));//������
	//�������Թ���
	g_pDevice->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 3);
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);

	return S_OK;
}

void SetMatrix(void)
{
	//����任���������  
	//D3DXMATRIX matWorld, rx, ry, rz;
	//D3DXMatrixIdentity(&matWorld);
	//D3DXMatrixRotationX(&rx, PI * (timeGetTime() / 1000.0f));
	//D3DXMatrixRotationY(&ry, PI * (timeGetTime() / 1000.0f / 2.0f));
	//D3DXMatrixRotationZ(&rz, PI * (timeGetTime() / 1000.0f / 3.0f));
	//matWorld = rx * ry * rz * matWorld;
	//g_pDevice->SetTransform(D3DTS_WORLD, &matWorld);

	//ȡ���任���������  
	D3DXMATRIX matView;
	D3DXVECTOR3 eye(0.0f, 0.0f, -50.0f);
	D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &eye, &at, &up);
	g_pDevice->SetTransform(D3DTS_VIEW, &matView);

	//ͶӰ�任���������  
	D3DXMATRIX matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, PI / 4.0f, 800.0f / 600.0f, 1.0f, 1000.0f);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &matProjection);

	//�ӿڱ任������ 
	D3DVIEWPORT9 vp;
	vp.X = 0;
	vp.Y = 0;
	vp.Width = SCREEN_WIDTH;
	vp.Height = SCREEN_HEIGHT;
	vp.MinZ = 0.0f;
	vp.MaxZ = 1.0f;
	g_pDevice->SetViewport(&vp);
}

void Update(HWND hWnd)
{
	g_pInputs->GetInput();

	// �����ظ�����Ѱַģʽ
	if (g_pInputs->IsKeyDown(DIK_1))
	{
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	}
	// ���þ�������Ѱַģʽ
	if (g_pInputs->IsKeyDown(DIK_2))
	{
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
	}
	// ���ü�ȡ����Ѱַģʽ
	if (g_pInputs->IsKeyDown(DIK_3))
	{
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}
	// ���ñ߿�����Ѱַģʽ
	if (g_pInputs->IsKeyDown(DIK_4))
	{
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		g_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
	}

	static float fPosX = 0.0f, fPosY = 0.0f, fPosZ = 0.0f;
	// ��ס���������϶���Ϊƽ�Ʋ��� 
	if (g_pInputs->IsMouseButtonDown(0))
	{
		fPosX += g_pInputs->MouseDX() * 0.08f;
		fPosY += g_pInputs->MouseDY() * -0.08f;
	}

	//�����֣�Ϊ�۲���������� 
	fPosZ += g_pInputs->MouseDZ() * 0.02f;

	if (g_pInputs->IsKeyDown(DIK_A)) fPosX -= 0.005f;
	if (g_pInputs->IsKeyDown(DIK_D)) fPosX += 0.005f;
	if (g_pInputs->IsKeyDown(DIK_W)) fPosY += 0.005f;
	if (g_pInputs->IsKeyDown(DIK_S)) fPosY -= 0.005f;

	D3DXMatrixTranslation(&g_matWorld, fPosX, fPosY, fPosZ);

	static float fAngleX = PI / 6.0f, fAngleY = PI / 6.0f;
	// ��ס����Ҽ����϶���Ϊ��ת����
	if (g_pInputs->IsMouseButtonDown(1))
	{
		fAngleX += g_pInputs->MouseDY() * -0.01f;
		fAngleY += g_pInputs->MouseDX() * -0.01f;
	}
	//��ת����
	if (g_pInputs->IsKeyDown(DIK_UP)) fAngleX += 0.005f;
	if (g_pInputs->IsKeyDown(DIK_DOWN)) fAngleX -= 0.005f;
	if (g_pInputs->IsKeyDown(DIK_LEFT)) fAngleY += 0.005f;
	if (g_pInputs->IsKeyDown(DIK_RIGHT)) fAngleY -= 0.005f;

	D3DXMATRIX Rx, Ry;
	D3DXMatrixRotationX(&Rx, fAngleX);
	D3DXMatrixRotationY(&Ry, fAngleY);

	g_matWorld = Rx * Ry * g_matWorld;
	g_pDevice->SetTransform(D3DTS_WORLD, &g_matWorld);

	SetMatrix();
}

void Render(HWND hWnd)
{
	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	g_pDevice->BeginScene();

	g_pDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
	g_pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
	g_pDevice->SetIndices(g_pIndexBuffer);

	//��������
	g_pDevice->SetTexture(0, g_pMipTexture);

	g_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);

	RECT rect;
	GetClientRect(hWnd, &rect);

	int count = _stprintf_s(g_strFPS, 20, TEXT("FPS : %0.3f"), GetFPS());
	g_pTextFPS->DrawText(NULL, g_strFPS, count, &rect, DT_TOP | DT_RIGHT, D3DCOLOR_RGBA(0, 239, 136, 255));

	g_pTextAdaperName->DrawText(NULL, g_strAdaperName, -1, &rect, DT_TOP | DT_LEFT, D3DXCOLOR(1.0f, 0.5f, 0.0f, 1.0f));

	rect.top = 30;
	static TCHAR strInfo[256] = { 0 };
	_stprintf_s(strInfo, -1, TEXT("ģ������:(%0.2f, %0.2f, %0.2f)"), g_matWorld._41, g_matWorld._42, g_matWorld._43);
	g_pTextHelper->DrawText(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(135, 239, 136, 255));

	rect.left = 0; rect.top = 380;
	g_pTextInfo->DrawText(NULL, TEXT("����˵��:"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(235, 123, 230, 255));

	rect.top += 35;
	g_pTextHelper->DrawText(NULL, TEXT("    ��ס���������϶���ƽ��ģ��"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    ��ס����Ҽ����϶�����תģ��"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    ���������֣�����ģ��"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    W��S��A��D����ƽ��ģ��"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    �ϡ��¡����ҷ��������תģ��"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    ������1,2,3,4���ּ���������Ѱַģʽ֮���л�"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));
	rect.top += 25;
	g_pTextHelper->DrawText(NULL, TEXT("    ESC�� : �˳�����"), -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 200, 0, 255));

	g_pDevice->EndScene();
	g_pDevice->Present(NULL, NULL, NULL, NULL);
}

float GetFPS(void)
{
	static float fps = 0.0f;
	static int frameCount = 0;
	static float curTime = 0.0f;
	static float lastTime = 0.0f;

	frameCount++;

	curTime = timeGetTime() * 0.001f;

	if (curTime - lastTime > 1.0f)
	{
		fps = frameCount / (curTime - lastTime);
		lastTime = curTime;
		frameCount = 0;
	}

	return fps;
}

void Cleanup()
{
	SAFE_DELETE(g_pInputs);
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pMipTexture);
	SAFE_RELEASE(g_pTextAdaperName);
	SAFE_RELEASE(g_pTextHelper);
	SAFE_RELEASE(g_pTextInfo);
	SAFE_RELEASE(g_pTextFPS);
	SAFE_RELEASE(g_pDevice);
}