// Reimplements 0x4b31b0: zSys::GetCpuClass
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
int zSys::GetCpuClass() {
    return DetectCpuClassAndFeatures() & 0xffff;
}
