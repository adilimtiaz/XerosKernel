/* mem.c : memory manager
*/

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern long freemem;    /* start of free memory (set in i386.c) */
extern int end(void);    /* end of kernel image, use &end        */
extern char *maxaddr;    /* max memory address (set in i386.c)  */
long freememStart;
struct memHeader {
    unsigned long size;
    struct memHeader *prev;
    struct memHeader *next;
    char *sanityCheck;
    unsigned char dataStart[0];

};

struct memHeader *freeList;


void printMemHeader(char *name, struct memHeader *node) {
    kprintf("%s Pointer Address: %x \n", name, node);
    kprintf("%s Size: %x \n", name, node->size);
    kprintf("%s Prev: %x \n", name, (void *) node->prev);
    kprintf("%s Next: %x \n", name, (void *) node->next);
    kprintf("%s SanityCheck: %x \n", name, node->sanityCheck);
    kprintf("%s DataStart: %x \n", name, &node->dataStart);

}

void printFreeList(void) {
    struct memHeader *currentNode = freeList;
    int i = 0;
    while (currentNode != NULL) {
        kprintf("%d Pointer Address %x \n", i, currentNode);
        kprintf("%d ->next %x \n", i, currentNode->next);
        kprintf("%d ->prev %x \n", i, currentNode->prev);
        currentNode = currentNode->next;
        i++;
    }
}

void kmeminit(void) {
    // use freemem to see where memory starts from
    /* kprintf("\n===== Starting kmeminit\n"); */
    struct memHeader *nodeBeforeHole = (struct memHeader *) freemem;

    // This is for testing. I wanted to see if the second block gets used, cause in init i try to alloc a block with size > 0x1000.
    // Uncomment line 36 to see it work properly.
    // TODO: Fix this hard code
    /* nodeBeforeHole->size = 0x1000; */
    nodeBeforeHole->size =  ((HOLESTART - 1) - freemem - sizeof(struct memHeader));
    nodeBeforeHole->prev = NULL; // First node of linked list
    // So dataStart is 0 bytes long. It can't hold any meaningful value.
    // But it's address represents the end of the memHeader
    // That's why we set sanityCheck to &dataStart
    nodeBeforeHole->sanityCheck = (char *) &nodeBeforeHole->dataStart;

    struct memHeader *nodeAfterHole = (struct memHeader *) HOLEEND;
    nodeAfterHole->size = ((int) maxaddr - HOLEEND - sizeof(struct memHeader));
    nodeAfterHole->sanityCheck = (char *) &nodeAfterHole->dataStart;
    nodeAfterHole->prev = nodeBeforeHole;
    nodeAfterHole->next = NULL;

    nodeBeforeHole->next = nodeAfterHole;

    freeList = nodeBeforeHole;
    /* kprintf("===== Done with kmeminit\n"); */
}

void insertIntoFreelist(struct memHeader *inputNode) {
    if (freeList == NULL) {
        // case when all memory has been allocated
        freeList = inputNode;
        return;
    }

    if (freeList > inputNode) {
        // case when freelist has moved beyond freemem
        // Updating head of linked list
        inputNode->next = freeList;
        inputNode->prev = NULL;
        freeList->prev = inputNode;
        freeList = inputNode;
        return;
    }

    struct memHeader *currentNode = freeList;
    while (currentNode->next != NULL && currentNode->next < inputNode) {
        currentNode = currentNode->next;

    }
    if (currentNode->next == NULL) {
        // case when this is the last element
        currentNode->next = inputNode;
        inputNode->prev = currentNode;
        return;

    } else {
        // case when current node is not last element
        // and inputNode is < currentNode->next but > currentNode
        inputNode->next = currentNode->next;
        currentNode->next->prev = inputNode;
        currentNode->next = inputNode;
        inputNode->prev = currentNode;

    }

}

struct memHeader *getLeftAdjacentNode(struct memHeader *inputNode) {
    struct memHeader *currentNode = freeList;
    int isThereALeftNodeToMerge = 0;
    // Check If there is a free list node to the left of node being freed
    while (currentNode != NULL && currentNode < inputNode) {
        if ((int) currentNode->sanityCheck + currentNode->size == (int) inputNode) {
            /* printMemHeader("Original Left Block", currentNode); */
            isThereALeftNodeToMerge = 1;
            break;
        }
        currentNode = currentNode->next;
    }

    struct memHeader *leftNode = isThereALeftNodeToMerge == 1 ? currentNode : NULL;
    // returns leftNode if a freeNode was left of inputNode, else NULL
    return leftNode;
}

struct memHeader *getRightAdjacentNode(struct memHeader *inputNode) {
    struct memHeader *currentNode = freeList;
    int isThereARightNodeToMerge = 0;
    /* printMemHeader("Node Being Freed", inputNode); */
    while (currentNode != NULL) {
        if ((int) inputNode->sanityCheck + inputNode->size == (int) currentNode) {
            // right adjacent free node found
            /* printMemHeader("Original Left Free Block", currentNode); */
            isThereARightNodeToMerge = 1;
            break;
        }
        currentNode = currentNode->next;
    }

    struct memHeader *rightNode = isThereARightNodeToMerge == 1 ? currentNode : NULL;
    return rightNode;
}

struct memHeader *findBlockWithEnoughSize(struct memHeader *freeListNode, size_t amnt) {
    // we are comparing different data types so that may be a source of bugs. Check with tas
    while ((freeListNode != NULL) && (amnt > freeListNode->size)) {
        freeListNode = freeListNode->next;
    }
    return freeListNode;
}


struct memHeader *allocateBlock(struct memHeader *nodeWithEnoughSize, size_t amnt) {
    // We modify the free node to change it's size
    // Prev and next should remain unchanged
    // memHeaders should not be relocated. So they always stay in the same place. We change size.
    struct memHeader *allocatedBlockPtr = (struct memHeader *) ((size_t) & nodeWithEnoughSize->dataStart +
            nodeWithEnoughSize->size - amnt);
    nodeWithEnoughSize->size = nodeWithEnoughSize->size - amnt;
    //printMemHeader("Modified Node", nodeWithEnoughSize);

    // Allocate a new memHeader for the block that is being mallocd
    // Do not add it to the free list
    // It's address should be at the very end of nodeWithEnoughSize block.
    // That means we subtract amnt from the higher boundary of the nodeWithEnoughSizeBlock
    allocatedBlockPtr->size = amnt;
    allocatedBlockPtr->prev = NULL;
    allocatedBlockPtr->next = NULL;
    allocatedBlockPtr->sanityCheck = (char *) &allocatedBlockPtr->dataStart;
    /* printMemHeader("Allocated block", allocatedBlockPtr); */
    return allocatedBlockPtr;
}

void *kmalloc(size_t size) {
    size_t amnt = (size) / 16 + ((size % 16) ? 1 : 0);
    amnt = amnt * 16 + sizeof(struct memHeader);

    // Iterate through freelist and find a block with size enough for this malloc
    struct memHeader *firstFreeNode = freeList;
    struct memHeader *nodeWithEnoughSize = findBlockWithEnoughSize(firstFreeNode, amnt);

    if (nodeWithEnoughSize == NULL) {
        // There is not this much contiguous free memory in the system
        kprintf("No node was this large \n");
        return 0;
    }

    //printMemHeader("Selected Node", nodeWithEnoughSize);

    struct memHeader *startAddress = allocateBlock(nodeWithEnoughSize, amnt);

    return (void *) &startAddress->dataStart;
}

int kfree(void *ptr) {
    // check if this ptr has an adjacent node
    // node + size == ptr, then adjacent
    // if adjacent, merge node with memory allocated to this
    // merge = node->size += ptr->size
    // if no such node create another freelist node
    struct memHeader *nodeToBeFreed = (struct memHeader *) ptr - 1;

    if (ptr != nodeToBeFreed->sanityCheck) {
        /* printMemHeader("Node Being Freed", nodeToBeFreed); */
        /* kprintf("ptr: %x", ptr); */
        /* kprintf("Good one"); */
        /* kprintf("Sanity Check does not match dataStart. Critical Error! \n"); */
        return 0;
    }

    struct memHeader *rightNode = getRightAdjacentNode(nodeToBeFreed);
    struct memHeader *leftNode = getLeftAdjacentNode(nodeToBeFreed);
    if (rightNode == NULL && leftNode == NULL) {
        insertIntoFreelist(nodeToBeFreed);
        /* printFreeList(); */
    } else if (leftNode != NULL && rightNode == NULL) {
        leftNode->size += nodeToBeFreed->size + sizeof(struct memHeader);
        /* printMemHeader("Resized left block", leftNode); */
    } else if (rightNode != NULL && leftNode == NULL) {
        // nodeToBeFreed < rightNode
        // So update size, next and prev for nodeToBeFreed
        // delete rightNode from freelist
        // update inputNode sanityCheck (because we move the actual memHeader)
        nodeToBeFreed->size += rightNode->size + sizeof(struct memHeader);
        nodeToBeFreed->next = rightNode->next;
        if (rightNode->prev != NULL) {
            // then set nodeToBeFreed to be prevs next
            rightNode->prev->next = nodeToBeFreed;
        } else {
            // else if rightNode was head of list, make node head
            freeList = nodeToBeFreed;
        }
    } else if (rightNode != NULL && leftNode != NULL) {
        leftNode->size += rightNode->size + (2 * sizeof(struct memHeader)) + nodeToBeFreed->size;
        leftNode->next = rightNode->next;
    }

    //printMemHeader("Node freed", nodeToBeFreed);
    //printMemHeader("Modified node", freeListNode);
    return 1;
}
