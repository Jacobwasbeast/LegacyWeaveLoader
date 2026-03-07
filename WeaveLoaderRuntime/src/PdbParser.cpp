#include "PdbParser.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cstring>
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

    // 3) Search per-module symbol streams (S_LPROC32, S_GPROC32, S_LPROC32_ID, S_GPROC32_ID, S_LDATA32, S_GDATA32)
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
