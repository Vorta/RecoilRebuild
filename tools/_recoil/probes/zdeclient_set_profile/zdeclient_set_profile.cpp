#include <vector>
#include <set>

struct zGeometry_ClipPatchNodeView {
    int probeTag;
};

struct ProbeFeatureEntry {
    unsigned char bytes[52];
};

std::vector<ProbeFeatureEntry> g_probeFeatureEntries;
std::set<zGeometry_ClipPatchNodeView *> g_probeFeatureNodes;

typedef char ProbeSetSizeMustBe16[
    sizeof(g_probeFeatureNodes) == 16 ? 1 : -1
];

int __fastcall InsertNodeProbe(zGeometry_ClipPatchNodeView *node) {
    return g_probeFeatureNodes.insert(node).second ? 1 : 0;
}

typedef void (__fastcall *NodeVisitorProbe)(zGeometry_ClipPatchNodeView *node);

int __fastcall VisitNodesProbe(NodeVisitorProbe visitor) {
    int count = 0;
    for (std::set<zGeometry_ClipPatchNodeView *>::iterator node =
            g_probeFeatureNodes.begin();
        node != g_probeFeatureNodes.end();
        ++node) {
        visitor(*node);
        ++count;
    }
    return count;
}

void __cdecl ClearNodesProbe() {
    g_probeFeatureNodes.clear();
}

void __cdecl EraseNodeRangeProbe() {
    g_probeFeatureNodes.erase(
        g_probeFeatureNodes.begin(),
        g_probeFeatureNodes.end()
    );
}
