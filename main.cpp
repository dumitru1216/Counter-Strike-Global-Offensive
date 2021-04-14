#include "source.hpp"
#include "visitor.hpp"
#include "hooge.hpp"
#include "icons.hpp"
//#include "threading/threading.h"
//#include "threading/shared_mutex.h"
#include "sdk.hpp"

//#include <thread>
#define _ITERATOR_DEBUG_LEVEL 0

DWORD installed;

class init_font
{
public:
	init_font(void* font, uint32_t length)
	{
		if (handle = AddFontMemResourceEx(font, length, nullptr, &installed); handle == nullptr)
			return;

		VirtualProtect(font, length, PAGE_READWRITE, 0);
		memset(font, 0, length);
	}

private:
	HANDLE handle;
};

//static Semaphore dispatchSem;
//static SharedMutex smtx;
//
//using ThreadIDFn = int(_cdecl*)();
//
//ThreadIDFn AllocateThreadID;
//ThreadIDFn FreeThreadID;
//
//int AllocateThreadIDWrapper() {
//	return AllocateThreadID();
//}
//
//int FreeThreadIDWrapper() {
//	return FreeThreadID();
//}
//
//template<typename T, T& Fn>
//static void AllThreadsStub(void*) {
//	dispatchSem.Post();
//	smtx.rlock();
//	smtx.runlock();
//	Fn();
//}
//
////TODO: Build this into the threading library
//template<typename T, T& Fn>
//static void DispatchToAllThreads(void* data, bool t = false) {
//	smtx.wlock();
//
//	for (size_t i = 0; i < Threading::numThreads; i++)
//		Threading::QueueJobRef(AllThreadsStub<T, Fn>, data);
//
//	if (t) {
//		for (size_t i = 0; i < Threading::numThreads; i++)
//			dispatchSem.Wait();
//	}
//
//	smtx.wunlock();
//
//	Threading::FinishQueue(false);
//}

#ifdef AUTH
//void Entry();
//void* initptr = &Entry;

void Entry()
{
	init_font(static_cast<void*>(visitor), sizeof(visitor));
	init_font(static_cast<void*>(hooge), sizeof(hooge));
	init_font(static_cast<void*>(icons_font), sizeof(icons_font));

	while (!GetModuleHandleA(sxor("serverbrowser.dll")))
		Sleep(200);

	//Beep(550, 200);

	//auto tier0 = GetModuleHandleA("tier0.dll");

	//AllocateThreadID = (ThreadIDFn)GetProcAddress(tier0, "AllocateThreadID");
	//FreeThreadID = (ThreadIDFn)GetProcAddress(tier0, "FreeThreadID");

	//Threading::InitThreads();

	//DispatchToAllThreads<decltype(AllocateThreadIDWrapper), AllocateThreadIDWrapper>(nullptr, true);

	Source::Create();

	//{
	//	while (!GetAsyncKeyState(VK_F11))
	//		Sleep(200);

	//	Source::Destroy();

	//	Sleep(1000);
	//}

	//initptr = 0;
	/*erase_end;*/
}
#else
FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;

void Entry(HMODULE hModule)
{
	AttachConsole(GetCurrentProcessId());
	freopen_s(&fpstdin, "CONIN$", "r", stdin);
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	freopen_s(&fpstderr, "CONOUT$", "w", stderr);
	SetConsoleTitleA("F"); //press F to pay respects to Microsoft

	init_font(static_cast<void*>(visitor), sizeof(visitor));
	init_font(static_cast<void*>(hooge), sizeof(hooge));
	init_font(static_cast<void*>(icons_font), sizeof(icons_font));

	while (!GetModuleHandleA(sxor("serverbrowser.dll")))
		Sleep(200);

	/*auto tier0 = GetModuleHandleA("tier0.dll");

	AllocateThreadID = (ThreadIDFn)GetProcAddress(tier0, "AllocateThreadID");
	FreeThreadID = (ThreadIDFn)GetProcAddress(tier0, "FreeThreadID");

	Threading::InitThreads();

	DispatchToAllThreads<decltype(AllocateThreadIDWrapper), AllocateThreadIDWrapper>(nullptr, true);*/

	if (Source::Create())
	{
		while (!GetAsyncKeyState(VK_F11)) {
			//Source::QueueJobs();
			//Threading::FinishQueue(true);
			Sleep(200);
		}

		/*DispatchToAllThreads<decltype(FreeThreadIDWrapper), FreeThreadIDWrapper>(nullptr);
		Threading::EndThreads();*/

		Source::Destroy();

		
		Sleep(1000);
	}

	fclose(stdin); fclose(stdout); fclose(stderr); // else console won't close
	FreeConsole();
	FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);
}

#endif // AUTH

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	if( dwReason == DLL_PROCESS_ATTACH )
	{

#ifndef AUTH
		CreateThread(nullptr, 0u, (LPTHREAD_START_ROUTINE)Entry, hModule, 0u, nullptr);
#else
		ctx.data = (_MANUAL_INJECTEX32*)lpReserved;

		if (ctx.data != nullptr) {
			//Beep(400, 200);
			Entry();//CreateThread(nullptr, 0u, (LPTHREAD_START_ROUTINE)initptr, 0, 0u, nullptr);
		}
#endif



		DisableThreadLibraryCalls(hModule);
	}
	/*else if (dwReason == DLL_PROCESS_DETACH)
	{
		RemoveVectoredExceptionHandler(exception_handle);
		Source::Destroy();
	}*/

	return TRUE;
}