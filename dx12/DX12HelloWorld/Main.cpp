#include "HelloWorldApp.h"
#include <memory>


std::unique_ptr<D3DApp> app = nullptr;;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return app->MsgProc(hwnd, msg, wParam, lParam);
}

int	WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
#if	defined(DEBUG)	|	defined(_DEBUG)		
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        app = std::make_unique<D3DAppHelloWorld>(hInstance);
        if (app->Initialize(&MainWndProc))
        {
            app->Run();
        }
    }
    catch (DxException & e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

    return 0;
}

