#include <stdio.h>
#include <string.h>

extern "C" int zhud_fill_bitmap_update_normalized_call_contract_smoke(void);
extern "C" int zhud_options_panel_sound_volume_on_activate_smoke(void);
extern "C" int zhud_options_panel_music_volume_on_activate_smoke(void);

namespace {

struct SmokeTest {
    const char *name;
    int (*run)(void);
};

const SmokeTest kSmokeTests[] = {
    {"zhud_fill_bitmap_update_normalized_call_contract_smoke",
     zhud_fill_bitmap_update_normalized_call_contract_smoke},
    {"zhud_options_panel_sound_volume_on_activate_smoke",
     zhud_options_panel_sound_volume_on_activate_smoke},
    {"zhud_options_panel_music_volume_on_activate_smoke",
     zhud_options_panel_music_volume_on_activate_smoke},
};

} // namespace

int main(int argc, char **argv) {
    const char *const selectedName = argc > 1 ? argv[1] : 0;
    int selectedCount = 0;
    int failureCount = 0;

    for (int index = 0;
         index < static_cast<int>(sizeof(kSmokeTests) / sizeof(kSmokeTests[0]));
         ++index) {
        const SmokeTest &test = kSmokeTests[index];
        if (selectedName != 0 && strcmp(selectedName, test.name) != 0) {
            continue;
        }

        ++selectedCount;
        const int result = test.run();
        if (result != 0) {
            ++failureCount;
            printf("[FAIL] %s: exit %d\n", test.name, result);
        }
    }

    if (selectedName != 0 && selectedCount == 0) {
        fprintf(stderr, "unknown smoke: %s\n", selectedName);
        return 2;
    }
    return failureCount == 0 ? 0 : 1;
}
