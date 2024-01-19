#ifndef __SDLL_HPP__
#define __SDLL_HPP__

#include <iostream> // FIXME
#include <mutex>

// This is a sorted doubly linked list.
// It assumes that insertion at the tail is more likely. Therefore, insertion starts at
// the tail side (then, searches for the right position).
// Extraction instead occurs at the head side.

//              head_                             tail_
//                |                                 |
//                V                                 V
//              ------           ------           ------           
//    |--prev-- | 12 | ---next-->| 15 | ---next-->| 20 | ---next--> nullptr
//    V         |    | <--prev---|    | <--prev---|    | 
// nullptr      ------           ------           ------           


template <class T, class D>
class Sdll {
public:
    Sdll() : head_(nullptr), tail_(nullptr) {}
    ~Sdll() {
        while (head_ != nullptr) {
            Node* d = head_;
            head_ = head_->next;
            delete d;
        }
    }
    // Returns true if T1 < T2
    virtual bool Compare (const T* t1, const T* t2)=0;
//         return ((*t1) < (*t2)); 
//     }

    void Insert(T sort, D* data) {
        Node* node = new Node;
        node->sort = sort;
        node->data = data;
        node->next = nullptr;
        node->prev = nullptr;

        std::lock_guard lock(mutex_);
        if (tail_ == nullptr) {
            // Empty list
            tail_ = node;
            head_ = node;
        } else {
            // There is at least one node
            Node* n = tail_;
            while ((n != nullptr) && !Compare(&(n->sort), &sort))
                n = n->prev;

            if (n == nullptr) {
                // Head insertion
                node->next = head_;
                head_ = node; 
            } else {
                // Insert after n
                if (tail_ == n)
                    tail_ = node;
                node->next = n->next;
                n->next = node;
                node->prev = n;
            }
        }
    }
    bool CheckFirst(T* out) {
        std::lock_guard lock(mutex_);
        if (head_ == nullptr)
            return false;
        *out = head_->sort;
        return true;
    }

    D* Extract(){
        D* ret = nullptr;
        std::lock_guard lock(mutex_);
        if (head_ != nullptr) {
            Node* remove = head_;
            ret = remove->data;
            head_ = head_->next;
            if (head_ == nullptr)
                tail_ = nullptr;
            else
                head_->prev = nullptr;
            delete remove;
        }
        return ret;
    }
        
    void Print() {
        std::cout << "=============" << std::endl;
        std::lock_guard lock(mutex_);
        Node* n = head_;
        if (n == nullptr)
            std::cout << "Empty list!" << std::endl;
        while (n != nullptr) {
            std::cout << n->sort << std::endl;
            n = n->next;
        }
    }


private:
    struct Node {
        T sort;
        D* data;
        Node* next;
        Node* prev;
    };    
    Node* head_;
    Node* tail_;
    std::mutex mutex_;
};

#endif
