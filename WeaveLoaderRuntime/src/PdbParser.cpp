#include "PdbParser.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cctype>
#include <cstring>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include "PDB.h"
#include "PDB_RawFile.h"
#include "PDB_InfoStream.h"
#include "PDB_DBIStream.h"
#include "PDB_PublicSymbolStream.h"
#include "PDB_GlobalSymbolStream.h"
#include "PDB_ImageSectionStream.h"
#include "PDB_CoalescedMSFStream.h"
#include "PDB_ModuleInfoStream.h"
#include "PDB_ModuleSymbolStream.h"
#include "PDB_Util.h"
#include "Foundation/PDB_BitUtil.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

struct SymEntry
{
    uint32_t    rva;
    std::string name;
};

static std::vector<SymEntry> s_addrIndex;

struct MappedFile
{
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    HANDLE hMapping     = nullptr;
    void*  baseAddress  = nullptr;
    size_t fileSize     = 0;
};

static MappedFile  s_mapped;
static bool        s_open = false;

static PDB::RawFile*            s_rawFile        = nullptr;
static PDB::DBIStream*          s_dbiStream      = nullptr;
static PDB::ImageSectionStream* s_sectionStream  = nullptr;
static PDB::PublicSymbolStream* s_publicStream   = nullptr;
static PDB::GlobalSymbolStream* s_globalStream   = nullptr;
static PDB::ModuleInfoStream*   s_moduleStream   = nullptr;
static PDB::CoalescedMSFStream* s_symbolRecords  = nullptr;

struct SimilarMatch
{
    std::string name;
    uint32_t    rva;
    int         score;
};

static void CloseMappedFile(MappedFile& mf)
{
    if (mf.baseAddress) { UnmapViewOfFile(mf.baseAddress); mf.baseAddress = nullptr; }
    if (mf.hMapping)    { CloseHandle(mf.hMapping);        mf.hMapping = nullptr; }
    if (mf.hFile != INVALID_HANDLE_VALUE) { CloseHandle(mf.hFile); mf.hFile = INVALID_HANDLE_VALUE; }
    mf.fileSize = 0;
}

static bool OpenMappedFile(const char* path, MappedFile& mf)
{
    mf.hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
    if (mf.hFile == INVALID_HANDLE_VALUE) return false;

    mf.hMapping = CreateFileMappingW(mf.hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mf.hMapping) { CloseMappedFile(mf); return false; }

    mf.baseAddress = MapViewOfFile(mf.hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!mf.baseAddress) { CloseMappedFile(mf); return false; }

    BY_HANDLE_FILE_INFORMATION fi;
    if (!GetFileInformationByHandle(mf.hFile, &fi)) { CloseMappedFile(mf); return false; }
    mf.fileSize = (static_cast<size_t>(fi.nFileSizeHigh) << 32) | fi.nFileSizeLow;
    return true;
}

// Helpers to extract name + section/offset from various global symbol record kinds
static const char* GetGlobalSymName(const PDB::CodeView::DBI::Record* record,
                                    uint16_t& outSection, uint32_t& outOffset)
{
    switch (record->header.kind)
    {
    case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
        outSection = record->data.S_GDATA32.section;
        outOffset  = record->data.S_GDATA32.offset;
        return record->data.S_GDATA32.name;
    case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
        outSection = record->data.S_LDATA32.section;
        outOffset  = record->data.S_LDATA32.offset;
        return record->data.S_LDATA32.name;
    case PDB::CodeView::DBI::SymbolRecordKind::S_GTHREAD32:
        outSection = record->data.S_GTHREAD32.section;
        outOffset  = record->data.S_GTHREAD32.offset;
        return record->data.S_GTHREAD32.name;
    case PDB::CodeView::DBI::SymbolRecordKind::S_LTHREAD32:
        outSection = record->data.S_LTHREAD32.section;
        outOffset  = record->data.S_LTHREAD32.offset;
        return record->data.S_LTHREAD32.name;
    default:
        return nullptr;
    }
}

#pragma pack(push, 1)
struct ProcRefRecordData
{
    uint32_t sumName;
    uint32_t ibSym;
    uint16_t imod;
    PDB_FLEXIBLE_ARRAY_MEMBER(char, name);
};
#pragma pack(pop)

static const char* GetProcRefName(const PDB::CodeView::DBI::Record* record,
                                  uint16_t& outModuleIndex, uint32_t& outSymbolOffset)
{
    switch (record->header.kind)
    {
    case PDB::CodeView::DBI::SymbolRecordKind::S_PROCREF:
    case PDB::CodeView::DBI::SymbolRecordKind::S_LPROCREF:
        {
            const ProcRefRecordData* ref = reinterpret_cast<const ProcRefRecordData*>(&record->data);
            outModuleIndex = ref->imod;
            outSymbolOffset = ref->ibSym;
            return ref->name;
        }
    default:
        return nullptr;
    }
}

static uint32_t ResolveProcRecordRVA(const PDB::CodeView::DBI::Record* record, const PDB::ImageSectionStream* sectionStream)
{
    if (!record || !sectionStream)
        return 0;

    uint16_t section = 0;
    uint32_t offset = 0;

    switch (record->header.kind)
    {
    case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
        section = record->data.S_LPROC32.section;
        offset = record->data.S_LPROC32.offset;
        break;
    case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
        section = record->data.S_GPROC32.section;
        offset = record->data.S_GPROC32.offset;
        break;
    case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
        section = record->data.S_LPROC32_ID.section;
        offset = record->data.S_LPROC32_ID.offset;
        break;
    case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
        section = record->data.S_GPROC32_ID.section;
        offset = record->data.S_GPROC32_ID.offset;
        break;
    default:
        return 0;
    }

    return sectionStream->ConvertSectionOffsetToRVA(section, offset);
}

static uint32_t ResolveProcRefRVA(uint16_t moduleIndex, uint32_t symbolOffset)
{
    if (!s_moduleStream || !s_rawFile)
        return 0;

    const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
    if (modules.GetLength() == 0)
        return 0;

    const size_t candidateIndices[2] = {
        (moduleIndex > 0) ? static_cast<size_t>(moduleIndex - 1u) : static_cast<size_t>(-1),
        static_cast<size_t>(moduleIndex)
    };
    const uint32_t candidateOffsets[2] = {
        symbolOffset,
        symbolOffset + 4u
    };

    for (size_t moduleArrayIndex : candidateIndices)
    {
        if (moduleArrayIndex >= modules.GetLength())
            continue;

        const PDB::ModuleInfoStream::Module& mod = modules[moduleArrayIndex];
        if (!mod.HasSymbolStream())
            continue;

        const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
        for (uint32_t candidateOffset : candidateOffsets)
        {
            const PDB::CodeView::DBI::Record* target = nullptr;
            size_t currentOffset = sizeof(uint32_t);
            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record) {
                if (target == nullptr && currentOffset == static_cast<size_t>(candidateOffset))
                    target = record;
                const uint32_t recordSize = PDB::GetCodeViewRecordSize(record);
                currentOffset = PDB::BitUtil::RoundUpToMultiple<size_t>(
                    currentOffset + sizeof(PDB::CodeView::DBI::RecordHeader) + recordSize, 4u);
            });
            const uint32_t rva = ResolveProcRecordRVA(target, s_sectionStream);
            if (rva != 0)
                return rva;
        }
    }

    return 0;
}

static void AddUniquePattern(std::vector<std::string>& patterns, const std::string& pattern)
{
    if (pattern.empty())
        return;

    for (const std::string& existing : patterns)
    {
        if (existing == pattern)
            return;
    }

    patterns.push_back(pattern);
}

static std::string ToLowerCopy(const char* text)
{
    std::string out = text ? text : "";
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

static size_t CommonPrefixLength(const std::string& lhs, const std::string& rhs)
{
    const size_t limit = std::min(lhs.size(), rhs.size());
    size_t n = 0;
    while (n < limit && lhs[n] == rhs[n])
        ++n;
    return n;
}

static size_t GreedySubsequenceMatches(const std::string& needle, const std::string& haystack)
{
    size_t matched = 0;
    size_t j = 0;
    for (char c : needle)
    {
        while (j < haystack.size() && haystack[j] != c)
            ++j;
        if (j >= haystack.size())
            break;
        ++matched;
        ++j;
    }
    return matched;
}

static size_t LevenshteinDistance(const std::string& lhs, const std::string& rhs)
{
    if (lhs.empty()) return rhs.size();
    if (rhs.empty()) return lhs.size();

    std::vector<size_t> prev(rhs.size() + 1);
    std::vector<size_t> curr(rhs.size() + 1);
    for (size_t j = 0; j <= rhs.size(); ++j)
        prev[j] = j;

    for (size_t i = 0; i < lhs.size(); ++i)
    {
        curr[0] = i + 1;
        for (size_t j = 0; j < rhs.size(); ++j)
        {
            const size_t cost = (lhs[i] == rhs[j]) ? 0u : 1u;
            curr[j + 1] = std::min({ prev[j + 1] + 1u, curr[j] + 1u, prev[j] + cost });
        }
        prev.swap(curr);
    }

    return prev[rhs.size()];
}

static int ScoreSimilarName(const std::string& missing, const std::string& candidate)
{
    if (candidate.empty())
        return -1000000;

    const size_t prefix = CommonPrefixLength(missing, candidate);
    const size_t subseq = GreedySubsequenceMatches(missing, candidate);
    const size_t distance = LevenshteinDistance(missing, candidate);
    const bool contains = candidate.find(missing) != std::string::npos
        || missing.find(candidate) != std::string::npos;

    int score = 0;
    if (contains) score += 200;
    score += static_cast<int>(prefix * 8);
    score += static_cast<int>(subseq * 3);
    score -= static_cast<int>(distance * 4);
    score -= static_cast<int>((missing.size() > candidate.size())
        ? (missing.size() - candidate.size())
        : (candidate.size() - missing.size()));
    return score;
}

static void AddOrUpdateSimilar(std::vector<SimilarMatch>& out, const char* name, uint32_t rva, int score)
{
    if (!name || !name[0])
        return;

    for (SimilarMatch& entry : out)
    {
        if (entry.name == name)
        {
            if (score > entry.score)
            {
                entry.score = score;
                entry.rva = rva;
            }
            return;
        }
    }

    out.push_back({ name, rva, score });
}

static void ExtractExactNamePatterns(const char* missingName, std::vector<std::string>& patterns)
{
    const std::string name(missingName ? missingName : "");
    if (name.empty())
        return;

    AddUniquePattern(patterns, name);

    const size_t scopePos = name.rfind("::");
    if (scopePos != std::string::npos)
    {
        AddUniquePattern(patterns, name.substr(scopePos + 2));
        AddUniquePattern(patterns, name.substr(0, scopePos));
    }
}

static void ExtractDecoratedNamePatterns(const char* missingName, std::vector<std::string>& patterns)
{
    const std::string name(missingName ? missingName : "");
    if (name.empty())
        return;

    size_t start = 0;
    while (start < name.size() && name[start] == '?')
        ++start;

    size_t end = name.find('@', start);
    if (end != std::string::npos && end > start)
        AddUniquePattern(patterns, name.substr(start, end - start));

    size_t typeStart = end;
    while (typeStart != std::string::npos && typeStart + 1 < name.size())
    {
        ++typeStart;
        size_t typeEnd = name.find('@', typeStart);
        if (typeEnd == std::string::npos || typeEnd == typeStart)
            break;

        const std::string token = name.substr(typeStart, typeEnd - typeStart);
        if (token != "std" && token != "_W" && token != "_N" && token != "PEAV" && token != "QEAA")
            AddUniquePattern(patterns, token);

        typeStart = typeEnd;
    }
}

namespace PdbParser
{

bool Open(const char* pdbPath)
{
    if (s_open) Close();

    LogUtil::Log("[WeaveLoader] PdbParser: opening %s", pdbPath);

    if (!OpenMappedFile(pdbPath, s_mapped))
    {
        LogUtil::Log("[WeaveLoader] PdbParser: failed to memory-map PDB file");
        return false;
    }

    if (PDB::ValidateFile(s_mapped.baseAddress, s_mapped.fileSize) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: PDB validation failed");
        CloseMappedFile(s_mapped);
        return false;
    }

    s_rawFile = new PDB::RawFile(PDB::CreateRawFile(s_mapped.baseAddress));

    if (PDB::HasValidDBIStream(*s_rawFile) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: invalid DBI stream");
        Close();
        return false;
    }

    const PDB::InfoStream infoStream(*s_rawFile);
    if (infoStream.UsesDebugFastLink())
    {
        LogUtil::Log("[WeaveLoader] PdbParser: PDB uses unsupported /DEBUG:FASTLINK");
        Close();
        return false;
    }

    s_dbiStream = new PDB::DBIStream(PDB::CreateDBIStream(*s_rawFile));

    if (s_dbiStream->HasValidImageSectionStream(*s_rawFile) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: invalid image section stream");
        Close();
        return false;
    }
    if (s_dbiStream->HasValidPublicSymbolStream(*s_rawFile) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: invalid public symbol stream");
        Close();
        return false;
    }
    if (s_dbiStream->HasValidGlobalSymbolStream(*s_rawFile) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: invalid global symbol stream");
        Close();
        return false;
    }
    if (s_dbiStream->HasValidSymbolRecordStream(*s_rawFile) != PDB::ErrorCode::Success)
    {
        LogUtil::Log("[WeaveLoader] PdbParser: invalid symbol record stream");
        Close();
        return false;
    }

    s_sectionStream = new PDB::ImageSectionStream(s_dbiStream->CreateImageSectionStream(*s_rawFile));
    s_symbolRecords = new PDB::CoalescedMSFStream(s_dbiStream->CreateSymbolRecordStream(*s_rawFile));
    s_publicStream  = new PDB::PublicSymbolStream(s_dbiStream->CreatePublicSymbolStream(*s_rawFile));
    s_globalStream  = new PDB::GlobalSymbolStream(s_dbiStream->CreateGlobalSymbolStream(*s_rawFile));
    s_moduleStream  = new PDB::ModuleInfoStream(s_dbiStream->CreateModuleInfoStream(*s_rawFile));

    s_open = true;
    LogUtil::Log("[WeaveLoader] PdbParser: PDB opened successfully");
    return true;
}

uint32_t FindSymbolRVA(const char* decoratedName)
{
    if (!s_open) return 0;

    // 1) Search public symbol stream (S_PUB32)
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_publicStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_publicStream->GetRecord(*s_symbolRecords, hashRecord);
            if (record->header.kind != PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
                continue;

            if (strcmp(record->data.S_PUB32.name, decoratedName) == 0)
            {
                return s_sectionStream->ConvertSectionOffsetToRVA(
                    record->data.S_PUB32.section,
                    record->data.S_PUB32.offset);
            }
        }
    }

    // 2) Search global symbol stream (S_GDATA32, S_LDATA32, S_GTHREAD32, S_LTHREAD32)
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t section = 0;
            uint32_t offset = 0;
            const char* name = GetGlobalSymName(record, section, offset);
            if (name && strcmp(name, decoratedName) == 0)
            {
                uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                if (rva != 0) return rva;
            }
        }
    }

    // 3) Search global PROCREF/LPROCREF symbols and chase them into the referenced module record.
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t moduleIndex = 0;
            uint32_t symbolOffset = 0;
            const char* name = GetProcRefName(record, moduleIndex, symbolOffset);

            if (!name || strcmp(name, decoratedName) != 0)
                continue;

            const uint32_t rva = ResolveProcRefRVA(moduleIndex, symbolOffset);
            if (rva != 0)
                return rva;
        }
    }

    // 4) Search per-module symbol streams (S_LPROC32, S_GPROC32, S_LPROC32_ID, S_GPROC32_ID, S_LDATA32, S_GDATA32)
    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream())
                continue;

            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            uint32_t foundRVA = 0;

            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                if (foundRVA != 0) return;

                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name;
                    section = record->data.S_LPROC32.section;
                    offset = record->data.S_LPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name;
                    section = record->data.S_GPROC32.section;
                    offset = record->data.S_GPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name;
                    section = record->data.S_LPROC32_ID.section;
                    offset = record->data.S_LPROC32_ID.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name;
                    section = record->data.S_GPROC32_ID.section;
                    offset = record->data.S_GPROC32_ID.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
                    name = record->data.S_LDATA32.name;
                    section = record->data.S_LDATA32.section;
                    offset = record->data.S_LDATA32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
                    name = record->data.S_GDATA32.name;
                    section = record->data.S_GDATA32.section;
                    offset = record->data.S_GDATA32.offset;
                    break;
                default:
                    return;
                }

                if (name && strcmp(name, decoratedName) == 0)
                {
                    uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                    if (rva != 0) foundRVA = rva;
                }
            });

            if (foundRVA != 0) return foundRVA;
        }
    }

    return 0;
}

uint32_t FindSymbolRVAByName(const char* exactName)
{
    if (!s_open || !exactName || !exactName[0]) return 0;

    // 1) Search global symbol stream for exact name matches on data/thread symbols.
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t section = 0;
            uint32_t offset = 0;
            const char* name = GetGlobalSymName(record, section, offset);

            if (!name || strcmp(name, exactName) != 0)
                continue;

            uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
            if (rva != 0) return rva;
        }
    }

    // 2) Search global PROCREF/LPROCREF symbols and chase them into the referenced module record.
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t moduleIndex = 0;
            uint32_t symbolOffset = 0;
            const char* name = GetProcRefName(record, moduleIndex, symbolOffset);

            if (!name || strcmp(name, exactName) != 0)
                continue;

            const uint32_t rva = ResolveProcRefRVA(moduleIndex, symbolOffset);
            if (rva != 0)
                return rva;
        }
    }

    // 3) Search per-module symbol streams for exact name matches.
    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream())
                continue;

            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            uint32_t foundRVA = 0;

            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                if (foundRVA != 0) return;

                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name;
                    section = record->data.S_LPROC32.section;
                    offset = record->data.S_LPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name;
                    section = record->data.S_GPROC32.section;
                    offset = record->data.S_GPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name;
                    section = record->data.S_LPROC32_ID.section;
                    offset = record->data.S_LPROC32_ID.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name;
                    section = record->data.S_GPROC32_ID.section;
                    offset = record->data.S_GPROC32_ID.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
                    name = record->data.S_LDATA32.name;
                    section = record->data.S_LDATA32.section;
                    offset = record->data.S_LDATA32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
                    name = record->data.S_GDATA32.name;
                    section = record->data.S_GDATA32.section;
                    offset = record->data.S_GDATA32.offset;
                    break;
                default:
                    return;
                }

                if (name && strcmp(name, exactName) == 0)
                {
                    uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                    if (rva != 0) foundRVA = rva;
                }
            });

            if (foundRVA != 0)
                return foundRVA;
        }
    }

    return 0;
}

void DumpMatching(const char* substring)
{
    if (!s_open) return;

    LogUtil::Log("[WeaveLoader] PdbParser: dumping symbols containing '%s'...", substring);
    int count = 0;

    // Public symbols
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_publicStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_publicStream->GetRecord(*s_symbolRecords, hashRecord);
            if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
            {
                if (strstr(record->data.S_PUB32.name, substring))
                {
                    uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(
                        record->data.S_PUB32.section, record->data.S_PUB32.offset);
                    LogUtil::Log("  [PUB] rva=0x%08X  %s", rva, record->data.S_PUB32.name);
                    count++;
                }
            }
        }
    }

    // Global symbols
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t section = 0;
            uint32_t offset = 0;
            const char* name = GetGlobalSymName(record, section, offset);
            if (name && strstr(name, substring))
            {
                uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                LogUtil::Log("  [GLB kind=0x%04X] rva=0x%08X  %s",
                             (unsigned)record->header.kind, rva, name);
                count++;
            }
        }
    }

    // Module symbols
    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream()) continue;
            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name; section = record->data.S_LPROC32.section; offset = record->data.S_LPROC32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name; section = record->data.S_GPROC32.section; offset = record->data.S_GPROC32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name; section = record->data.S_LPROC32_ID.section; offset = record->data.S_LPROC32_ID.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name; section = record->data.S_GPROC32_ID.section; offset = record->data.S_GPROC32_ID.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
                    name = record->data.S_LDATA32.name; section = record->data.S_LDATA32.section; offset = record->data.S_LDATA32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
                    name = record->data.S_GDATA32.name; section = record->data.S_GDATA32.section; offset = record->data.S_GDATA32.offset; break;
                default:
                    return;
                }

                if (name && strstr(name, substring))
                {
                    uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                    LogUtil::Log("  [MOD kind=0x%04X] rva=0x%08X  %s",
                                 (unsigned)record->header.kind, rva, name);
                    count++;
                }
            });
        }
    }

    LogUtil::Log("[WeaveLoader] PdbParser: found %d matching symbols", count);
}

void DumpSimilar(const char* missingName)
{
#ifndef WEAVELOADER_DEBUG_BUILD
    (void)missingName;
#else
    if (!missingName || !missingName[0])
        return;

    LogUtil::Log("[WeaveLoader] PdbParser: symbol '%s' is missing. Run WeaveLoader.exe --extensive-symbol-scan for a full similarity dump.", missingName);
#endif
}

void DumpSimilarFull(const char* missingName, const char* logPath, size_t maxResults)
{
    if (!s_open || !missingName || !missingName[0] || !logPath || !logPath[0])
        return;

    std::ofstream out(logPath, std::ios::out | std::ios::app);
    if (!out.is_open())
    {
        LogUtil::Log("[WeaveLoader] PdbParser: failed to open full similarity log '%s'", logPath);
        return;
    }

    out << "[WeaveLoader] Full similar symbol dump\n";
    out << "[WeaveLoader] Missing symbol: " << missingName << "\n";

    std::vector<SimilarMatch> matches;
    matches.reserve(4096);
    const std::string missingLower = ToLowerCopy(missingName);

    {
        const PDB::ArrayView<PDB::HashRecord> records = s_publicStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_publicStream->GetRecord(*s_symbolRecords, hashRecord);
            if (record->header.kind != PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
                continue;

            const char* name = record->data.S_PUB32.name;
            const int score = ScoreSimilarName(missingLower, ToLowerCopy(name));
            if (score <= 0)
                continue;

            const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(
                record->data.S_PUB32.section, record->data.S_PUB32.offset);
            AddOrUpdateSimilar(matches, name, rva, score);
        }
    }

    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t section = 0;
            uint32_t offset = 0;
            const char* name = GetGlobalSymName(record, section, offset);
            if (!name)
                continue;

            const int score = ScoreSimilarName(missingLower, ToLowerCopy(name));
            if (score <= 0)
                continue;

            const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
            AddOrUpdateSimilar(matches, name, rva, score);
        }
    }

    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream())
                continue;

            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name; section = record->data.S_LPROC32.section; offset = record->data.S_LPROC32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name; section = record->data.S_GPROC32.section; offset = record->data.S_GPROC32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name; section = record->data.S_LPROC32_ID.section; offset = record->data.S_LPROC32_ID.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name; section = record->data.S_GPROC32_ID.section; offset = record->data.S_GPROC32_ID.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
                    name = record->data.S_LDATA32.name; section = record->data.S_LDATA32.section; offset = record->data.S_LDATA32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
                    name = record->data.S_GDATA32.name; section = record->data.S_GDATA32.section; offset = record->data.S_GDATA32.offset; break;
                default:
                    return;
                }

                const int score = ScoreSimilarName(missingLower, ToLowerCopy(name));
                if (score <= 0)
                    return;

                const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                AddOrUpdateSimilar(matches, name, rva, score);
            });
        }
    }

    std::sort(matches.begin(), matches.end(), [](const SimilarMatch& a, const SimilarMatch& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.rva != b.rva) return a.rva < b.rva;
        return a.name < b.name;
    });

    out << "[WeaveLoader] Similar matches: " << matches.size() << "\n";
    const size_t count = std::min(maxResults, matches.size());
    for (size_t i = 0; i < count; ++i)
    {
        char hexBuf[16];
        std::snprintf(hexBuf, sizeof(hexBuf), "%08X", matches[i].rva);
        out << "[SIM score=" << matches[i].score << "] rva=0x" << hexBuf << " " << matches[i].name << "\n";
    }
    out << "\n";
    out.flush();

    LogUtil::Log("[WeaveLoader] PdbParser: wrote full similarity dump for '%s' to %s", missingName, logPath);
}

void BuildAddressIndex()
{
    if (!s_open) return;

    s_addrIndex.clear();

    // Collect all public symbols (S_PUB32) -- these cover exported and
    // non-static functions/data with their decorated names.
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_publicStream->GetRecords();
        s_addrIndex.reserve(records.GetLength());
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record =
                s_publicStream->GetRecord(*s_symbolRecords, hashRecord);
            if (record->header.kind != PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
                continue;

            uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(
                record->data.S_PUB32.section, record->data.S_PUB32.offset);
            if (rva != 0)
                s_addrIndex.push_back({ rva, record->data.S_PUB32.name });
        }
    }

    // Also pull in per-module procedure symbols (S_GPROC32/S_LPROC32) which
    // include internal/static functions not in the public stream.
    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream()) continue;
            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name;
                    section = record->data.S_LPROC32.section;
                    offset = record->data.S_LPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name;
                    section = record->data.S_GPROC32.section;
                    offset = record->data.S_GPROC32.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name;
                    section = record->data.S_LPROC32_ID.section;
                    offset = record->data.S_LPROC32_ID.offset;
                    break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name;
                    section = record->data.S_GPROC32_ID.section;
                    offset = record->data.S_GPROC32_ID.offset;
                    break;
                default:
                    return;
                }

                if (!name) return;
                uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                if (rva != 0)
                    s_addrIndex.push_back({ rva, name });
            });
        }
    }

    // Sort by RVA and deduplicate
    std::sort(s_addrIndex.begin(), s_addrIndex.end(),
              [](const SymEntry& a, const SymEntry& b) { return a.rva < b.rva; });

    // Remove duplicates (same RVA), keeping the first entry
    auto last = std::unique(s_addrIndex.begin(), s_addrIndex.end(),
                            [](const SymEntry& a, const SymEntry& b) { return a.rva == b.rva; });
    s_addrIndex.erase(last, s_addrIndex.end());

    LogUtil::Log("[WeaveLoader] PdbParser: built address index with %zu symbols", s_addrIndex.size());
}

bool FindNameByRVA(uint32_t rva, char* outName, size_t nameSize, uint32_t* outOffset)
{
    if (s_addrIndex.empty() || rva == 0)
        return false;

    // Binary search for the largest RVA <= target
    SymEntry key = { rva, {} };
    auto it = std::upper_bound(s_addrIndex.begin(), s_addrIndex.end(), key,
                               [](const SymEntry& a, const SymEntry& b) { return a.rva < b.rva; });

    if (it == s_addrIndex.begin())
        return false;

    --it;

    // Sanity: don't report symbols more than 1MB away
    if (rva - it->rva > 0x100000)
        return false;

    if (outName && nameSize > 0)
    {
        strncpy(outName, it->name.c_str(), nameSize - 1);
        outName[nameSize - 1] = '\0';
    }
    if (outOffset)
        *outOffset = rva - it->rva;

    return true;
}

bool EnumerateSymbols(std::vector<SymbolInfo>& outSymbols)
{
    if (!s_open)
        return false;

    outSymbols.clear();

    auto pushSymbol = [&](const char* name, uint32_t rva, bool isProc, uint8_t source)
    {
        if (!name || !name[0] || rva == 0)
            return;
        SymbolInfo info;
        info.name = name;
        info.rva = rva;
        info.isProc = isProc;
        info.source = source;
        outSymbols.push_back(std::move(info));
    };

    // Public symbols
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_publicStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_publicStream->GetRecord(*s_symbolRecords, hashRecord);
            if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
            {
                const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(
                    record->data.S_PUB32.section, record->data.S_PUB32.offset);
                pushSymbol(record->data.S_PUB32.name, rva, true, 1);
            }
        }
    }

    // Global symbols
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t section = 0;
            uint32_t offset = 0;
            const char* name = GetGlobalSymName(record, section, offset);
            if (name)
            {
                const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                pushSymbol(name, rva, false, 2);
            }
        }
    }

    // Global PROCREF/LPROCREF
    {
        const PDB::ArrayView<PDB::HashRecord> records = s_globalStream->GetRecords();
        for (const PDB::HashRecord& hashRecord : records)
        {
            const PDB::CodeView::DBI::Record* record = s_globalStream->GetRecord(*s_symbolRecords, hashRecord);
            uint16_t moduleIndex = 0;
            uint32_t symbolOffset = 0;
            const char* name = GetProcRefName(record, moduleIndex, symbolOffset);
            if (!name)
                continue;
            const uint32_t rva = ResolveProcRefRVA(moduleIndex, symbolOffset);
            if (rva != 0)
                pushSymbol(name, rva, true, 3);
        }
    }

    // Module symbols
    {
        const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = s_moduleStream->GetModules();
        for (const PDB::ModuleInfoStream::Module& mod : modules)
        {
            if (!mod.HasSymbolStream())
                continue;

            const PDB::ModuleSymbolStream modSymStream = mod.CreateSymbolStream(*s_rawFile);
            modSymStream.ForEachSymbol([&](const PDB::CodeView::DBI::Record* record)
            {
                const char* name = nullptr;
                uint16_t section = 0;
                uint32_t offset = 0;
                bool isProc = false;

                switch (record->header.kind)
                {
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32:
                    name = record->data.S_LPROC32.name; section = record->data.S_LPROC32.section; offset = record->data.S_LPROC32.offset; isProc = true; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32:
                    name = record->data.S_GPROC32.name; section = record->data.S_GPROC32.section; offset = record->data.S_GPROC32.offset; isProc = true; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID:
                    name = record->data.S_LPROC32_ID.name; section = record->data.S_LPROC32_ID.section; offset = record->data.S_LPROC32_ID.offset; isProc = true; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID:
                    name = record->data.S_GPROC32_ID.name; section = record->data.S_GPROC32_ID.section; offset = record->data.S_GPROC32_ID.offset; isProc = true; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_LDATA32:
                    name = record->data.S_LDATA32.name; section = record->data.S_LDATA32.section; offset = record->data.S_LDATA32.offset; break;
                case PDB::CodeView::DBI::SymbolRecordKind::S_GDATA32:
                    name = record->data.S_GDATA32.name; section = record->data.S_GDATA32.section; offset = record->data.S_GDATA32.offset; break;
                default:
                    return;
                }

                if (name)
                {
                    const uint32_t rva = s_sectionStream->ConvertSectionOffsetToRVA(section, offset);
                    pushSymbol(name, rva, isProc, 4);
                }
            });
        }
    }

    return true;
}

void Close()
{
    delete s_moduleStream;   s_moduleStream  = nullptr;
    delete s_globalStream;   s_globalStream  = nullptr;
    delete s_publicStream;   s_publicStream  = nullptr;
    delete s_symbolRecords;  s_symbolRecords = nullptr;
    delete s_sectionStream;  s_sectionStream = nullptr;
    delete s_dbiStream;      s_dbiStream     = nullptr;
    delete s_rawFile;        s_rawFile       = nullptr;
    CloseMappedFile(s_mapped);
    s_open = false;
    // Note: s_addrIndex intentionally NOT cleared -- it survives Close()
    // so the crash handler can resolve addresses after PDB is released.
}

} // namespace PdbParser
