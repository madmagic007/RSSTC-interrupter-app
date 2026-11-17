#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_heap_caps.h>
#include <TonePlayer.h>
#include <vector>

class TrackStore {
public:
    bool begin() {
        if (!LittleFS.begin(false)) { err_ = "LittleFS mount failed (upload the filesystem image first)"; return false; }
        err_ = "";
        return true;
    }

    bool getTrackNames(std::vector<String>& names) {
        names.clear();
        File root = LittleFS.open("/");
        if (!root || !root.isDirectory()) return fail("track directory unavailable");

        for (File f = root.openNextFile(); f; f = root.openNextFile()) {
            if (f.isDirectory()) continue;
            String name = f.name();
            if (!name.endsWith(".tsq")) continue;
            if (name.startsWith("/")) name.remove(0, 1);
            name.remove(name.length() - 4);
            if (!name.isEmpty()) names.push_back(name);
        }

        err_ = "";
        return true;
    }

    void list(Print& out) {
        std::vector<String> names;
        if (!getTrackNames(names)) {
            out.println(error());
            return;
        }

        for (const String& name : names) {
            File f = LittleFS.open(String("/") + name + ".tsq", "r");
            if (!f) continue;
            const size_t size = f.size();
            const size_t entries = size >= 8 ? (size - 8) / 7 : 0;
            out.printf("%-24s %6u bytes (~%u entries)\n", name.c_str(),
                       (unsigned)size, (unsigned)entries);
        }
    }

    bool load(const char* name) {
        unload();
        String path = String("/") + name;
        if (!path.endsWith(".tsq")) path += ".tsq";

        File f = LittleFS.open(path, "r");
        if (!f) return fail("track not found");

        uint8_t hdr[8];
        if (f.read(hdr, 8) != 8 || memcmp(hdr, "TSQ1", 4) != 0) return fail("bad header");
        uint32_t count;
        memcpy(&count, hdr + 4, 4);
        if (count == 0 || (size_t)f.size() != 8 + (size_t)count * 7) return fail("size mismatch (truncated upload?)");

        ToneDataEntry* buf = (ToneDataEntry*)heap_caps_malloc(
            (size_t)count * sizeof(ToneDataEntry), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!buf) return fail("out of RAM");

        uint8_t chunk[7 * 64];
        uint32_t i = 0;
        while (i < count) {
            uint32_t n = count - i < 64 ? count - i : 64;
            if (f.read(chunk, n * 7) != n * 7) { heap_caps_free(buf); return fail("read error"); }
            for (uint32_t k = 0; k < n; k++) {
                const uint8_t* p = chunk + k * 7;
                uint32_t us; uint16_t freq;
                memcpy(&us, p, 4);
                memcpy(&freq, p + 5, 2);
                
                if (p[4] >= 3 || freq > 1000) { heap_caps_free(buf); return fail("corrupt entry"); }
                buf[i + k].usSinceLast = us;
                buf[i + k].channel = p[4];
                buf[i + k].freq = freq;
            }
            i += n;
        }
        data_ = buf;
        len_ = count;
        err_ = "";
        return true;
    }

    void unload() {
        if (data_) heap_caps_free(data_);
        data_ = nullptr;
        len_ = 0;
    }

    const ToneDataEntry* data() const { return data_; }
    uint32_t length() const { return len_; }
    const char* error() const { return err_; }

private:
    bool fail(const char* msg) { err_ = msg; return false; }

    ToneDataEntry* data_ = nullptr;
    uint32_t len_ = 0;
    const char* err_ = "";
};