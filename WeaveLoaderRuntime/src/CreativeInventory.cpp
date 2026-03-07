#include "CreativeInventory.h"
#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <cstring>
#include <algorithm>

namespace CreativeInventory
{

void* pCategoryGroups = nullptr;
void* pItemInstanceCtor = nullptr;
void* pSharedPtrCtor = nullptr;
void* pVectorPushBack = nullptr;
void* pSpecs = nullptr;

std::vector<PendingCreativeItem> s_pendingItems;

static const int CREATIVE_GROUP_COUNT = 15;
static const int SIZEOF_MSVC_VECTOR = 24;
static const int SIZEOF_MSVC_SHARED_PTR = 16;
static const int ITEMINSTANCE_ALLOC_SIZE = 256;

// TabSpec field offsets (MSVC x64 layout, derived from IUIScene_CreativeMenu.h)
// LPCWSTR m_icon              @ 0  (8)
// int m_descriptionId         @ 8  (4)
// int m_staticGroupsCount     @ 12 (4)
// ECreative_Inventory_Groups* @ 16 (8)
// int m_dynamicGroupsCount    @ 24 (4) + 4 pad
// ECreative_Inventory_Groups* @ 32 (8)
// int m_debugGroupsCount      @ 40 (4) + 4 pad
// ECreative_Inventory_Groups* @ 48 (8)
// uint m_pages                @ 56 (4)
// uint m_staticPerPage        @ 60 (4)
// uint m_staticItems          @ 64 (4)
// uint m_debugItems           @ 68 (4)
static const int TABSPEC_STATIC_GROUPS_COUNT_OFF = 12;
static const int TABSPEC_STATIC_GROUPS_A_OFF     = 16;
static const int TABSPEC_PAGES_OFF               = 56;
static const int TABSPEC_STATIC_ITEMS_OFF        = 64;
static const int TAB_COUNT = 8;
static const int COLUMNS = 10;
static const int ROWS = 5;

typedef void (__fastcall *ItemInstanceCtor_fn)(void* thisPtr, int id, int count, int auxValue);
typedef void (__fastcall *SharedPtrCtor_fn)(void* sharedPtrThis, void* rawItemPtr);
typedef void (__fastcall *VectorPushBackMove_fn)(void* vectorThis, void* sharedPtrRvalueRef);

void AddPending(int itemId, int count, int auxValue, int groupIndex)
{
    s_pendingItems.push_back({ itemId, count, auxValue, groupIndex });
    LogUtil::Log("[WeaveLoader] Queued creative item: id=%d group=%d", itemId, groupIndex);
}

bool ResolveSymbols(SymbolResolver& resolver)
{
    pCategoryGroups = resolver.Resolve(
        "?categoryGroups@IUIScene_CreativeMenu@@1PAV?$vector@V?$shared_ptr@VItemInstance@@@std@@"
        "V?$allocator@V?$shared_ptr@VItemInstance@@@std@@@2@@std@@A");

    pItemInstanceCtor = resolver.Resolve("??0ItemInstance@@QEAA@HHH@Z");

    pSharedPtrCtor = resolver.Resolve(
        "??$?0VItemInstance@@$0A@@?$shared_ptr@VItemInstance@@@std@@QEAA@PEAVItemInstance@@@Z");

    pVectorPushBack = resolver.Resolve(
        "?push_back@?$vector@V?$shared_ptr@VItemInstance@@@std@@V?$allocator@V?$shared_ptr@VItemInstance@@@std@@@2@@std@@"
        "QEAAX$$QEAV?$shared_ptr@VItemInstance@@@2@@Z");

    pSpecs = resolver.Resolve(
        "?specs@IUIScene_CreativeMenu@@1PEAPEAUTabSpec@1@EA");

    if (pCategoryGroups)  LogUtil::Log("[WeaveLoader] categoryGroups       @ %p", pCategoryGroups);
    else                  LogUtil::Log("[WeaveLoader] MISSING: categoryGroups");

    if (pItemInstanceCtor) LogUtil::Log("[WeaveLoader] ItemInstance ctor     @ %p", pItemInstanceCtor);
    else                   LogUtil::Log("[WeaveLoader] MISSING: ItemInstance(int,int,int)");

    if (pSharedPtrCtor)   LogUtil::Log("[WeaveLoader] shared_ptr<II> ctor   @ %p", pSharedPtrCtor);
    else                  LogUtil::Log("[WeaveLoader] MISSING: shared_ptr<ItemInstance>(ItemInstance*)");

    if (pVectorPushBack)  LogUtil::Log("[WeaveLoader] vector::push_back     @ %p", pVectorPushBack);
    else                  LogUtil::Log("[WeaveLoader] MISSING: vector<shared_ptr<II>>::push_back");

    if (pSpecs)           LogUtil::Log("[WeaveLoader] specs                 @ %p", pSpecs);
    else
    {
        LogUtil::Log("[WeaveLoader] MISSING: specs (page counts won't be updated)");
        PdbParser::DumpMatching("specs@IUIScene_CreativeMenu");
    }

    return pCategoryGroups && pItemInstanceCtor && pSharedPtrCtor && pVectorPushBack;
}

static size_t ReadVectorSize(char* vec)
{
    char* first = *reinterpret_cast<char**>(vec);
    char* last  = *reinterpret_cast<char**>(vec + 8);
    if (!first || last <= first) return 0;
    return static_cast<size_t>((last - first) / SIZEOF_MSVC_SHARED_PTR);
}

void UpdateTabPageCounts()
{
    if (!pSpecs || !pCategoryGroups)
    {
        LogUtil::Log("[WeaveLoader] Cannot update tab page counts: specs=%p categoryGroups=%p",
                     pSpecs, pCategoryGroups);
        return;
    }

    void** specsArray = *reinterpret_cast<void***>(pSpecs);
    if (!specsArray)
    {
        LogUtil::Log("[WeaveLoader] specs pointer is null, TabSpec array not yet allocated");
        return;
    }

    char* groups = reinterpret_cast<char*>(pCategoryGroups);

    for (int tabIdx = 0; tabIdx < TAB_COUNT; ++tabIdx)
    {
        char* tab = reinterpret_cast<char*>(specsArray[tabIdx]);
        if (!tab) continue;

        int staticGroupsCount = *reinterpret_cast<int*>(tab + TABSPEC_STATIC_GROUPS_COUNT_OFF);
        int* staticGroupsA = *reinterpret_cast<int**>(tab + TABSPEC_STATIC_GROUPS_A_OFF);
        if (!staticGroupsA || staticGroupsCount <= 0) continue;

        unsigned int totalItems = 0;
        for (int i = 0; i < staticGroupsCount; ++i)
        {
            int groupIdx = staticGroupsA[i];
            if (groupIdx < 0 || groupIdx >= CREATIVE_GROUP_COUNT) continue;
            totalItems += static_cast<unsigned int>(
                ReadVectorSize(groups + groupIdx * SIZEOF_MSVC_VECTOR));
        }

        unsigned int oldItems = *reinterpret_cast<unsigned int*>(tab + TABSPEC_STATIC_ITEMS_OFF);
        unsigned int oldPages = *reinterpret_cast<unsigned int*>(tab + TABSPEC_PAGES_OFF);

        *reinterpret_cast<unsigned int*>(tab + TABSPEC_STATIC_ITEMS_OFF) = totalItems;

        int totalRows = (totalItems + COLUMNS - 1) / COLUMNS;
        int newPages = totalRows - ROWS + 1;
        if (newPages < 1) newPages = 1;
        *reinterpret_cast<unsigned int*>(tab + TABSPEC_PAGES_OFF) = static_cast<unsigned int>(newPages);

        if (totalItems != oldItems)
        {
            LogUtil::Log("[WeaveLoader] Tab %d: staticItems %u -> %u, pages %u -> %u",
                         tabIdx, oldItems, totalItems, oldPages, static_cast<unsigned int>(newPages));
        }
    }
}

void InjectItems()
{
    if (!pCategoryGroups || !pItemInstanceCtor || !pSharedPtrCtor || !pVectorPushBack)
    {
        LogUtil::Log("[WeaveLoader] Cannot inject creative items: missing symbols");
        return;
    }

    if (s_pendingItems.empty())
    {
        LogUtil::Log("[WeaveLoader] No creative items to inject");
        return;
    }

    auto ctorFn = reinterpret_cast<ItemInstanceCtor_fn>(pItemInstanceCtor);
    auto spCtorFn = reinterpret_cast<SharedPtrCtor_fn>(pSharedPtrCtor);
    auto pushFn = reinterpret_cast<VectorPushBackMove_fn>(pVectorPushBack);

    // categoryGroups is a static array of vectors (vector<...> categoryGroups[15]).
    // pCategoryGroups is the address of the first vector; no dereference needed.
    char* groups = reinterpret_cast<char*>(pCategoryGroups);

    for (auto& item : s_pendingItems)
    {
        if (item.groupIndex < 0 || item.groupIndex >= CREATIVE_GROUP_COUNT)
        {
            LogUtil::Log("[WeaveLoader] Skipping creative item id=%d: invalid group %d",
                         item.itemId, item.groupIndex);
            continue;
        }

        void* rawItem = ::operator new(ITEMINSTANCE_ALLOC_SIZE);
        memset(rawItem, 0, ITEMINSTANCE_ALLOC_SIZE);
        ctorFn(rawItem, item.itemId, item.count, item.auxValue);

        // Verify ItemInstance vtable was set (first 8 bytes should be non-null)
        void* vtable = *reinterpret_cast<void**>(rawItem);
        LogUtil::Log("[WeaveLoader] ItemInstance(%d,%d,%d) @ %p, vtable=%p",
                     item.itemId, item.count, item.auxValue, rawItem, vtable);

        char spBuf[16];
        memset(spBuf, 0, sizeof(spBuf));
        spCtorFn(spBuf, rawItem);

        // Log shared_ptr contents (ptr + control block)
        void* spPtr = *reinterpret_cast<void**>(spBuf);
        void* spCtrl = *reinterpret_cast<void**>(spBuf + 8);
        LogUtil::Log("[WeaveLoader] shared_ptr: ptr=%p ctrl=%p", spPtr, spCtrl);

        char* vec = groups + item.groupIndex * SIZEOF_MSVC_VECTOR;
        size_t sizeBefore = ReadVectorSize(vec);

        pushFn(vec, spBuf);

        size_t sizeAfter = ReadVectorSize(vec);
        LogUtil::Log("[WeaveLoader] Injected item id=%d into creative group %d "
                     "(vector @ %p, size: %zu -> %zu)",
                     item.itemId, item.groupIndex, vec, sizeBefore, sizeAfter);
    }

    LogUtil::Log("[WeaveLoader] Injected %zu items into creative inventory", s_pendingItems.size());
    s_pendingItems.clear();
}

} // namespace CreativeInventory
