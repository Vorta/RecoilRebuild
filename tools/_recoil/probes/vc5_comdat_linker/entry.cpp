extern "C" int duplicate_a(int value);
extern "C" int duplicate_b(int value);
extern "C" __declspec(dllimport) void __stdcall ExitProcess(unsigned long exitCode);

extern "C" void __stdcall probe_entry(void) {
    ExitProcess((unsigned long)(duplicate_a(1) + duplicate_b(2)));
}
