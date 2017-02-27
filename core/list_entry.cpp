#include "list_entry.h"

namespace Core
{

void InitializeListHead(ListEntry* listHead)
{
    listHead->Flink = listHead->Blink = listHead;
    return;
}

bool IsListEmpty(ListEntry* listHead)
{
    return (listHead->Flink == listHead) ? true : false;
}

bool RemoveEntryList(ListEntry* entry)
{
    ListEntry* blink;
    ListEntry* flink;

    flink = entry->Flink;
    blink = entry->Blink;
    blink->Flink = flink;
    flink->Blink = blink;
    return (flink == blink) ? true : false;
}

void RemoveInitEntryList(ListEntry* entry)
{
    RemoveEntryList(entry);
    InitializeListHead(entry);
}

ListEntry* RemoveHeadList(ListEntry* listHead)
{
    ListEntry* flink;
    ListEntry* entry;

    entry = listHead->Flink;
    flink = entry->Flink;
    listHead->Flink = flink;
    flink->Blink = listHead;
    return entry;
}

ListEntry* RemoveTailList(ListEntry* listHead)
{
    ListEntry* blink;
    ListEntry* entry;

    entry = listHead->Blink;
    blink = entry->Blink;
    listHead->Blink = blink;
    blink->Flink = listHead;
    return entry;
}

void InsertTailList(ListEntry* listHead, ListEntry* entry)
{

    ListEntry* blink;

    blink = listHead->Blink;
    entry->Flink = listHead;
    entry->Blink = blink;
    blink->Flink = entry;
    listHead->Blink = entry;
    return;
}

void AppendTailList(ListEntry* listHead, ListEntry* listToAppend)
{
    ListEntry* listEnd = listHead->Blink;

    listHead->Blink->Flink = listToAppend;
    listHead->Blink = listToAppend->Blink;
    listToAppend->Blink->Flink = listHead;
    listToAppend->Blink = listEnd;
}

void InsertHeadList(ListEntry* listHead, ListEntry* entry)
{

    ListEntry* flink;

    flink = listHead->Flink;
    entry->Flink = flink;
    entry->Blink = listHead;
    flink->Blink = entry;
    listHead->Flink = entry;
    return;
}

}