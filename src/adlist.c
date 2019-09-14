/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h" // XXX

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
/* 创建一个新list，已创建的list可以使用AlFreeList()函数释放，但是在调用AlFreeList()释放列表之前
 * 需要先将列表中的每个node先释放
 * 
 * 如果创建出错，会返回NULL，否则范围指向新创建的list的指针
 * */
list *listCreate(void)
{
    struct list *list;
    // 申请内存空间，如果失败则返回NULL
    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    /* list的初始化操作，新创建的list没有node，因此head，tail都是NULL
     * 长度为0
     * XXX
     * XXX
     * XXX
    */
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/* Free the whole list.
 *
 * This function can't fail. */
/* 释放整个列表
 * 这个函数不会失败？XXX
*/
void listRelease(list *list)
{
    unsigned long len;
    listNode *current, *next;
    
    current = list->head;   // 从list的头开始释放
    len = list->len;        // 需要释放的node的数量就是list的长度
    while(len--) {  // 依次释放
        next = current->next;
        if (list->free) list->free(current->value); // 如果list有定义free方法，则调用自己的free方法来释放node
        zfree(current); // 调用redis定义的zfree函数释放当前节点
        current = next; 
    }
    zfree(list);    // 最后使用zfree释放list
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
/* 添加一个节点到list的头部
 * 如果失败，则返回NULL，不会执行任何操作，列表不会修改
 * 如果成功，则返回传递给函数的list的指针
*/
list *listAddNodeHead(list *list, void *value)
{
    listNode *node; // 定义新增节点
    // 如果分配内存失败，则返回NULL
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;    // 给新增的节点赋值
    if (list->len == 0) {   // 如果当前list为空，则将该节点作为头节点和尾节点，并且pre和next都为空
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {    // 如果当前list不为空
        node->prev = NULL;  // 则新增的头结点pre为空，因为它即将成为新的头节点
        node->next = list->head;    // 新增的头节点的next为目前头节点，目前的头节点即将成为第二个节点
        list->head->prev = node;    // list当前的头节点的pre为新增的节点
        list->head = node;  // 最后将list的头节点指向新增的node
    }
    list->len++;    // list长度加1
    return list;    // 返回list的指针
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
// 新增节点到尾部，和listAddNodeHead函数基本一致，不再注释
list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

// 指定位置插入新节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (after) {    // 如果在指定的node之后添加节点
        node->prev = old_node;  // 设置新增的node节点的前一个节点
        node->next = old_node->next;    // 设置新增的node节点的后一个节点
        // 如果被添加的节点为list的最后一个节点，则将新增的节点设置为尾节点
        if (list->tail == old_node) {
            list->tail = node;
        }
    } else {
        // 与上面逻辑相似，将新节点添加的指定节点的前面
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {
            list->head = node;
        }
    }
    // 链接新增节点的前后节点
    if (node->prev != NULL) {
        node->prev->next = node;
    }
    if (node->next != NULL) {
        node->next->prev = node;
    }
    
    list->len++;    // list长度加1
    return list;    // 返回list
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
/* 从list删除指定的节点 */
void listDelNode(list *list, listNode *node)
{
    if (node->prev) // 如果被删除的节点不是头节点，则将前一个节点的next设置的被删除节点的next节点
        node->prev->next = node->next;
    else    // 如果被删除的是头节点，则将后一个节点设置为头节点
        list->head = node->next;
    if (node->next) // 如果被删除的节点不是尾节点，则将前一个节点的pre设置的被删除节点的pre节点
        node->next->prev = node->prev;
    else    // 如果被删除的是尾节点，则将前一个的节点设置为尾节点
        list->tail = node->prev;
    if (list->free) list->free(node->value);    // 如果list定义了free函数，则使用该free函数释放node
    zfree(node);
    list->len--;    // list长度减1
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. 
 * 返回一个list迭代器，初始化之后每次调用listNext()函数都会返回list中的下一个元素 */
listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;
    // 申请内存空间，失败则直接返回NULL
    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;
    if (direction == AL_START_HEAD) // 如果从list头开始迭代，则迭代器的下一个节点为头节点
        iter->next = list->head;
    else
        iter->next = list->tail;    // 如果从list尾开始迭代，则迭代器的下一个节点为尾节点
    iter->direction = direction;    // 设置迭代器的方向
    return iter;
}

/* Release the iterator memory 
 * 释放迭代器内存 */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure 
 * 将迭代器的位置重置为头节点 */
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

/* 将迭代器的位置重置为尾节点 */
void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * 
 * 返回迭代器的下一个元素 
 * 函数范围下一个元素的指针，如果后面没有元素则返回NULL */
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next; // 设置迭代器的下一个节点为next节点
        else
            iter->next = current->prev; // 设置迭代器的下一个节点为pre节点
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. 
 * 复制整个list，如果内存溢出则返回NULL 
 * 如果成功则返回原始列表的复制 
 * 无论成功失败，原始列表都不会改变 */
list *listDup(list *orig)
{
    list *copy; // 定义新列表
    listIter iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)
        return NULL;    // 如果创建新列表失败，则直接返回NULL
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;
    listRewind(orig, &iter);    // 从头开始迭代的迭代器
    while((node = listNext(&iter)) != NULL) {
        void *value;

        if (copy->dup) {
            value = copy->dup(node->value);
            if (value == NULL) {    // 如果内存溢出，则释放目标list空间，直接返回NULL
                listRelease(copy);
                return NULL;
            }
        } else
            value = node->value;
        if (listAddNodeTail(copy, value) == NULL) { // 如果内存溢出，则释放目标list空间，直接返回NULL
            listRelease(copy);
            return NULL;
        }
    }
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. 
 * 匹配给定的key，搜索list 
 * 匹配成功则返回第一个搜索到的node的指针（从头搜索）
 * 没有搜索到则返回NULL */
listNode *listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;

    listRewind(list, &iter);    // 从头部开始的迭代器
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {  // 如果定义的match方法，则使用match方法匹配
            if (list->match(node->value, key)) {
                return node;
            }
        } else {    // 如果没有定义match方法，则直接比较node的值
            if (key == node->value) {
                return node;
            }
        }
    }
    return NULL;    // 没有匹配到则返回NULL
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. 
 * 根据给定的下标返回元素， 如果给定的是负数，则从list的尾部开始计算
 * -1是最后一个节点（尾节点），如果给定的索引越界则返回NULL */
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {    // 给定的索引小于0则从尾开始
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;    // index递减，向前寻找node
    } else {
        n = list->head;
        while(index-- && n) n = n->next;    // index递减，向后寻找node
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. 
 * 把list的尾移动到list的头部 */
void listRotate(list *list) {
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}
