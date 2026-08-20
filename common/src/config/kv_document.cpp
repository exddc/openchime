#include "oc/config/kv_document.h"

#include "oc/config/kv_config.h"

namespace oc::config {
namespace {

std::string_view StripCR(std::string_view line) {
    if (!line.empty() && line.back() == '\r') {
        return line.substr(0, line.size() - 1);
    }
    return line;
}

} // namespace

std::vector<KvEntry> ParseKvDocument(std::string_view content) {
    std::vector<KvEntry> entries;
    std::size_t start = 0;
    while (start < content.size()) {
        const auto end = content.find('\n', start);
        const std::string_view raw =
            StripCR(end == std::string_view::npos ? content.substr(start) : content.substr(start, end - start));
        KvEntry entry;
        entry.text = std::string(raw);
        const std::string cleaned = trim(raw);
        if (cleaned.empty()) {
            entry.kind = KvEntry::Kind::kBlank;
        } else if (cleaned.front() == '#') {
            entry.kind = KvEntry::Kind::kComment;
        } else {
            const auto sep = cleaned.find('=');
            if (sep == std::string::npos) {
                entry.kind = KvEntry::Kind::kOther;
            } else {
                entry.kind = KvEntry::Kind::kAssignment;
                entry.key = trim(std::string_view(cleaned.data(), sep));
            }
        }
        entries.push_back(std::move(entry));
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return entries;
}

std::string RenderKvDocument(const std::vector<KvEntry> &entries) {
    std::string output;
    for (const auto &entry : entries) {
        output += entry.text;
        output.push_back('\n');
    }
    if (entries.empty()) {
        output.push_back('\n');
    }
    return output;
}

std::string KvDocumentValue(const std::vector<KvEntry> &entries, std::string_view key) {
    std::string value;
    for (const auto &entry : entries) {
        if (entry.kind != KvEntry::Kind::kAssignment || entry.key != key) {
            continue;
        }
        const auto sep = entry.text.find('=');
        if (sep == std::string::npos) {
            continue;
        }
        value = trim(std::string_view(entry.text.data() + sep + 1, entry.text.size() - sep - 1));
    }
    return value;
}

bool KvDocumentHasKey(const std::vector<KvEntry> &entries, std::string_view key) {
    for (const auto &entry : entries) {
        if (entry.kind == KvEntry::Kind::kAssignment && entry.key == key) {
            return true;
        }
    }
    return false;
}

void KvDocumentSetValue(std::vector<KvEntry> &entries, const std::string &key, const std::string &value) {
    const std::string line = key + "=" + value;
    bool found = false;
    for (auto &entry : entries) {
        if (entry.kind == KvEntry::Kind::kAssignment && entry.key == key) {
            entry.text = line;
            found = true;
        }
    }
    if (found) {
        return;
    }
    KvEntry entry;
    entry.kind = KvEntry::Kind::kAssignment;
    entry.text = line;
    entry.key = key;
    entries.push_back(std::move(entry));
}

void KvDocumentRemoveKey(std::vector<KvEntry> &entries, std::string_view key) {
    std::vector<KvEntry> kept;
    kept.reserve(entries.size());
    for (auto &entry : entries) {
        if (entry.kind == KvEntry::Kind::kAssignment && entry.key == key) {
            continue;
        }
        kept.push_back(std::move(entry));
    }
    entries.swap(kept);
}

} // namespace oc::config
