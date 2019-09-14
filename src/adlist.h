/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */
/* 定义list中每个节点的数据结构哦 */
typedef struct listNode {
    struct listNode *prev;  // 指向list中前一个节点的指针
    struct listNode *next;  // 指向list中后一个节点的指针
    void *value;    // node的值，可以为任何类型
} listNode;

/* list单向迭代器 */
typedef struct listIter {
    listNode *next; // 下一个node的指针
    int direction;  // 迭代方向，后面有定义 （AL_START_HEAD，AL_START_TAIL）
} listIter;

/* 定义list的数据结构，list由一个或多个listNode组成 */
typedef struct list {
    listNode *head; // list的头节点
    listNode *tail; // list的尾节点
    void *(*dup)(void *ptr);    // 复制node的方法
    void (*free)(void *ptr);    // 释放node的方法
    int (*match)(void *ptr, void *key); // 匹配node的方法
    unsigned long len;  // list长度
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)    // 获取list的长度
#define listFirst(l) ((l)->head)    // 获取list头节点
#define listLast(l) ((l)->tail)     // 获取list尾节点
#define listPrevNode(n) ((n)->prev) // 指定结点的上一结点
#define listNextNode(n) ((n)->next) // 指定结点的下一结点
#define listNodeValue(n) ((n)->value)   // 指定结点的值

#define listSetDupMethod(l,m) ((l)->dup = (m))  // XXX
#define listSetFreeMethod(l,m) ((l)->free = (m))    // XXX
#define listSetMatchMethod(l,m) ((l)->match = (m))  // XXX

#define listGetDupMethod(l) ((l)->dup)  // XXX
#define listGetFree(l) ((l)->free)  // XXX
#define listGetMatchMethod(l) ((l)->match)  // XXX

/* Prototypes */
/* 函数原型 */
list *listCreate(void); // 创建list
void listRelease(list *list);   // 释放list
list *listAddNodeHead(list *list, void *value); // 增加头节点
list *listAddNodeTail(list *list, void *value); // 增加尾节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);   // 指定位置插入节点
void listDelNode(list *list, listNode *node);   // 删除节点
listIter *listGetIterator(list *list, int direction);   // 获取list指定方向的迭代器
listNode *listNext(listIter *iter); // 获取迭代器的下一节点
void listReleaseIterator(listIter *iter);   // 释放列表迭代器
list *listDup(list *orig);  // 列表复制
listNode *listSearchKey(list *list, void *key); // 搜索列表，匹配key
listNode *listIndex(list *list, long index);    // 根据下标获取节点
void listRewind(list *list, listIter *li);  // 重置list迭代器，从头开始迭代
void listRewindTail(list *list, listIter *li);  // 重置list迭代器，从尾开始迭代
void listRotate(list *list);    // list旋转

/* Directions for iterators */
/* 定义迭代器方向 */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
