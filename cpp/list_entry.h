#pragma once

#include "main.h"

struct LIST_ENTRY
{
    struct LIST_ENTRY *Flink;
    struct LIST_ENTRY *Blink;
};

typedef struct LIST_ENTRY* PLIST_ENTRY;

void
InitializeListHead(
    PLIST_ENTRY ListHead
    );

bool
IsListEmpty(
    LIST_ENTRY * ListHead
    );

bool
RemoveEntryList(
    PLIST_ENTRY Entry
    );

void
RemoveInitEntryList(
    PLIST_ENTRY Entry
    );

PLIST_ENTRY
RemoveHeadList(
    PLIST_ENTRY ListHead
    );


PLIST_ENTRY
RemoveTailList(
    PLIST_ENTRY ListHead
    );

void
InsertTailList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    );

void
InsertHeadList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    );
