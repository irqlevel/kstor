#pragma once

namespace Core
{

struct ListEntry
{
    struct ListEntry *Flink;
    struct ListEntry *Blink;
};

void InitializeListHead(ListEntry* listHead);

bool IsListEmpty(ListEntry* listHead);

bool RemoveEntryList(ListEntry* entry);

void RemoveInitEntryList(ListEntry* entry);

ListEntry* RemoveHeadList(ListEntry* listHead);

ListEntry* RemoveTailList(ListEntry* listHead);

void InsertTailList(ListEntry* listHead, ListEntry* entry);

void AppendTailList(ListEntry* listHead, ListEntry* listToAppend);

void InsertHeadList(ListEntry* ListHead, ListEntry* Entry);

}