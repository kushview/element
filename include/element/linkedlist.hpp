// Copyright Copyright (C) 2005-2012, rncbc aka Rui Nuno Capela. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later
// Modified: 2013 Michael Fisher <mfisher@kushview.net>

#pragma once

namespace element {

/** A doubly linked list */
template <class Node>
class LinkedList {
public:
    LinkedList() {}
    ~LinkedList() { clear(); }

    Node* first() const { return firstNode; }
    Node* last() const { return lastNode; }

    int count() const { return numNodes; }
    void setScoped (bool scoped) { scopedList = scoped; }
    bool isScoped() const { return scopedList; }
    void insertAfter (Node* node, Node* prev = nullptr);
    void insertBefore (Node* node, Node* next = nullptr);
    void unlink (Node* node);
    void remove (Node* node);
    void clear();
    Node* at (int index) const;
    void prepend (Node* node) { insertBefore (node); }
    void append (Node* node) { insertAfter (node); }
    Node* operator[] (int index) const { return at (index); }
    int find (Node* node) const;

    /** Base list node */
    class Link {
    public:
        Link() : prevNode (nullptr),
                 nextNode (nullptr),
                 nextFreeNode (nullptr) {}

        Node* prev() const { return prevNode; }
        Node* next() const { return nextNode; }

        void setPrevious (Node* prev) { prevNode = prev; }
        void setNext (Node* next) { nextNode = next; }

        Node* nextFree() const { return nextFreeNode; }
        void setNextFree (Node* node) { nextFreeNode = node; }

    private:
        Node* prevNode;
        Node* nextNode;
        Node* nextFreeNode;
    };

    class iterator {
    public:
        inline iterator (LinkedList<Node>& list, Node* ptr) : nodes (list), nodePtr (ptr) {}
        inline iterator (const iterator& it) : nodes (it.nodes), nodePtr (it.nodePtr) {}

        inline iterator& first()
        {
            nodePtr = nodes.first();
            return *this;
        }
        inline iterator& next()
        {
            nodePtr = nodePtr != nullptr ? nodePtr->next() : nullptr;
            return *this;
        }
        inline iterator& prev()
        {
            nodePtr = nodePtr == nullptr ? this->last() : nodePtr->prev();
            return *this;
        }
        inline iterator& last()
        {
            nodePtr = nodes.last();
            return *this;
        }

        /* Operators */
        inline bool operator== (const iterator& other) const { return nodePtr == other.nodePtr; }
        inline bool operator!= (const iterator& other) const { return nodePtr != other.nodePtr; }

        inline iterator& operator= (const iterator& iter)
        {
            nodePtr = iter.nodePtr;
            return *this;
        }
        inline iterator& operator= (Node* pNode)
        {
            nodePtr = pNode;
            return *this;
        }

        inline iterator& operator++() { return next(); }
        inline iterator operator++ (int)
        {
            iterator it (*this);
            next();
            return it;
        }

        inline iterator& operator--() { return prev(); }
        inline iterator operator-- (int)
        {
            iterator it (*this);
            prev();
            return it;
        }

        inline Node* operator->() { return nodePtr; }
        inline const Node* operator->() const { return nodePtr; }

        inline Node* operator*() { return nodePtr; }
        inline const Node* operator*() const { return nodePtr; }

        /* Access */
        const LinkedList<Node>& list() const { return nodes; }
        Node* node() const { return nodePtr; }

    private:
        LinkedList<Node>& nodes;
        Node* nodePtr;
    };

    iterator begin() { return iterator (*this, first()); }
    iterator end() { return iterator (*this, nullptr); }

private:
    Node* firstNode { nullptr };
    Node* lastNode { nullptr };
    int numNodes { 0 };
    Node* freeList { nullptr };
    bool scopedList { false };
};

template <class Node>
void LinkedList<Node>::insertAfter (Node* node, Node* previous)
{
    if (previous == 0)
        previous = lastNode;

    node->setPrevious (previous);
    if (previous) {
        node->setNext (previous->next());
        if (previous->next())
            (previous->next())->setPrevious (node);
        else
            lastNode = node;
        previous->setNext (node);
    } else {
        firstNode = lastNode = node;
        node->setNext (0);
    }

    ++numNodes;
}

template <class Node>
void LinkedList<Node>::insertBefore (Node* node, Node* next_node)
{
    if (next_node == 0)
        next_node = firstNode;

    node->setNext (next_node);

    if (next_node) {
        node->setPrevious (next_node->prev());
        if (next_node->prev())
            (next_node->prev())->setNext (node);
        else
            firstNode = node;
        next_node->setPrevious (node);
    } else {
        lastNode = firstNode = node;
        node->setPrevious (0);
    }

    ++numNodes;
}

template <class Node>
void LinkedList<Node>::unlink (Node* node)
{
    if (node->prev())
        (node->prev())->setNext (node->next());
    else
        firstNode = node->next();

    if (node->next())
        (node->next())->setPrevious (node->prev());
    else
        lastNode = node->prev();

    --numNodes;
}

// Remove method.
template <class Node>
void LinkedList<Node>::remove (Node* node)
{
    unlink (node);

    // Add it to the alternate free list.
    if (scopedList) {
        Node* nextFree = freeList;
        node->setNextFree (nextFree);
        freeList = node;
    }
}

// Reset methods.
template <class Node>
void LinkedList<Node>::clear()
{
    // Remove pending items.
    Node* last = lastNode;
    while (last) {
        remove (last);
        last = lastNode;
    }

    Node* free_list = freeList;
    while (free_list) {
        Node* nextFree = free_list->nextFree();
        delete free_list;
        free_list = nextFree;
    }

    // Force clean up.
    firstNode = lastNode = 0;
    numNodes = 0;
    freeList = 0;
}

template <class Node>
Node* LinkedList<Node>::at (int index) const
{
    int i;
    Node* node;

    if (index < 0 || index >= numNodes)
        return 0;

    if (index > (numNodes >> 1)) {
        for (i = numNodes - 1, node = lastNode; node && i > index; --i, node = node->prev()) {
            ;
        }
    } else {
        for (i = 0, node = firstNode; node && i < index; ++i, node = node->next()) {
            ;
        }
    }

    return node;
}

template <class Node>
int LinkedList<Node>::find (Node* node) const
{
    int index = 0;
    Node* n = firstNode;

    while (n) {
        if (node == n)
            return index;

        n = n->next();
        ++index;
    }

    return -1;
}

} // namespace element
