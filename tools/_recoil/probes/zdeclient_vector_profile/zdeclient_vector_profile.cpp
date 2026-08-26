#include <vector>
#include <cstring>

struct FeatureEventData {
    unsigned char bytes[44];
};

struct FeatureEntry {
    int featureType;
    FeatureEventData eventData;
    int reloadFlag;
};

std::vector<FeatureEntry> g_featureEntries;

int __fastcall AppendFeatureEntryProbe(
    int featureType,
    const void *featureEventData
) {
    unsigned int eventDataBytes = 0;
    if (featureType == 1) {
        eventDataBytes = 40;
    } else if (featureType == 3) {
        eventDataBytes = 44;
    } else {
        return 0;
    }

    FeatureEntry entry;
    entry.featureType = featureType;
    memset(&entry.eventData, 0, sizeof(entry.eventData));
    memcpy(&entry.eventData, featureEventData, eventDataBytes);
    entry.reloadFlag = 0;
    g_featureEntries.push_back(entry);
    return 0;
}
