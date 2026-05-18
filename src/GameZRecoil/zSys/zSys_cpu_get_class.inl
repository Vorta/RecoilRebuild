// Reimplements 0x4b31b0: zSys::GetCpuClass
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL zSys::GetCpuClass()
{
    return DetectCpuClassAndFeatures() & 0xffff;
}
