/*
DEMO program for Cached-Freetype Font class
*/

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "Font.h"

// procedure func
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

TCHAR winname[] = L"freetype_test";
TCHAR clsname[] = L"freetype_test";

#define FAIL_CHECK(func, msg)\
	if (!func) {\
		OutputDebugString(msg);\
		return 1;\
	}

unsigned int* bitmap = 0;

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)

{
	// prepare bitmap
	bitmap = (unsigned int *)malloc(640 * 480 * sizeof(int));
	memset(bitmap, 0xFF, 640 * 480 * sizeof(int));

	// prepare input_text (should be UTF8 encoding)
	FILE *fp = fopen("input_text.txt", "r");
	char str[256];
	fread(str, 1, 256, fp);
	char *_p = str; while (*_p != '\n' && *_p != 0) ++_p;	*_p = 0;
	fclose(fp);
	int i = strlen(str);
	if (str[0] == (char)0xEF && str[1] == (char)0xBB && str[2] == (char)0xBF) {
		// exclude UTF8 BOM
		memcpy(str, str + 3, i);
		i -= 3;
	}

	// Let's initialize font first ...
	Font font;
	font.LoadFont("NanumBarunGothic.ttf", 30);
	FontTexture fnt;
	font.RenderBitmap(str, &fnt);
	for (int y = 0; y < fnt.height; y++) {
		for (int x = 0; x < fnt.width && x < 640; x++) {
			bitmap[x + (y + 120) * 640] = fnt.p[x + y * fnt.width] << 16;
		}
	}
	free(fnt.p);


	// generate window handle
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = clsname;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	//wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION)); wcex.hCursor = LoadCursor(NULL, IDC_ARROW); wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = NULL; wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	FAIL_CHECK(RegisterClassEx(&wcex), L"class registration failed\n");

	HWND hWnd = CreateWindow( winname, clsname, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL );
	FAIL_CHECK(hWnd, L"window creation failed\n");
	ShowWindow(hWnd, SW_SHOW);

	// dispatch message
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc, memdc;
	HBITMAP map;
	TCHAR greeting[] = _T("Hello, World!");
	switch (message) {
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{
			map = CreateBitmap(640, 480, 1, 8 * 4, (void*)bitmap);
			memdc = CreateCompatibleDC(hdc);
			SelectObject(memdc, map);
			BitBlt(hdc, 0, 0, 640, 480, memdc, 0, 0, SRCCOPY);
			DeleteDC(memdc);
			DeleteObject(map);
		}
		TextOut(hdc, 5, 5, greeting, _tcslen(greeting));
		EndPaint(hWnd, &ps);
		break;
	case WM_SIZE:
		InvalidateRect(hWnd, 0, FALSE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
} 