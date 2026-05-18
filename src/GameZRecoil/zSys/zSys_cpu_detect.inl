// Reimplements 0x4b3420: zSys::DetectCpuClassAndFeatures
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL zSys::DetectCpuClassAndFeatures()
{
    int result;
    if ((unsigned short)HasCpuidSupport() != 0) {
        result = ReadCpuidVendorAndFamily();
    } else {
        unsigned int divProbe = (unsigned int)ProbeDivZeroFlagBehavior();
        divProbe &= 0xffffu;
        g_zSys_CpuVendorNonIntelMarker = divProbe;

        result = DetectIs8086ByEflagsHiBits();
        if ((unsigned short)result != 0) {
            result = DetectIs80286ByEflagsHiBits();
            if ((unsigned short)result != 2) {
                result = DetectIs80386ByAcFlag();
                if ((unsigned short)result != 3) {
                    result = 4;
                }
            }
        }
    }

    if (g_zSys_CpuVendorNonIntelMarker != 0) {
        result |= 0x8000;
    }
    return result;
}
