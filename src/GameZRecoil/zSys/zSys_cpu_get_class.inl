/**
 * Reimplements 0x4b31b0: zSys::GetCpuClass
 * (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp).
 *
 * Purpose: return the low-word CPU class from the recovered CPU detection
 * packet.
 */
int zSys::GetCpuClass() {
    return DetectCpuClassAndFeatures() & 0xffff;
}
