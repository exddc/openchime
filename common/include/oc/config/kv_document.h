#ifndef OC_CONFIG_KV_DOCUMENT_H
#define OC_CONFIG_KV_DOCUMENT_H

#include <string>
#include <string_view>
#include <vector>

namespace oc::config {

struct KvEntry {
    enum class Kind { kBlank, kComment, kAssignment, kOther };
    Kind kind = Kind::kOther;
    std::string text;
    std::string key;
};

std::vector<KvEntry> ParseKvDocument(std::string_view content);
std::string RenderKvDocument(const std::vector<KvEntry> &entries);
std::string KvDocumentValue(const std::vector<KvEntry> &entries, std::string_view key);
bool KvDocumentHasKey(const std::vector<KvEntry> &entries, std::string_view key);
void KvDocumentSetValue(std::vector<KvEntry> &entries, const std::string &key, const std::string &value);
void KvDocumentRemoveKey(std::vector<KvEntry> &entries, std::string_view key);

} // namespace oc::config

#endif
