#ifndef __SDLL_HPP__
#define __SDLL_HPP__

#include <iostream> // FIXME
#include <mutex>
#include <memory>

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
    ~Sdll() {
        while (head_ != nullptr) {
            head_ = head_->next;
        }
    }
    // Returns true if T1 < T2
    virtual bool Compare (const T* t1, const T* t2)=0;
//         return ((*t1) < (*t2)); 
//     }

    void Insert(T sort, std::unique_ptr<D> data) {
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->sort = sort;
        node->data = std::move(data);
        node->next = nullptr;
        node->prev = nullptr;

        std::lock_guard lock(mutex_);
        if (tail_ == nullptr) {
            // Empty list
            tail_ = node;
            head_ = node;
        } else {
            // There is at least one node
            std::shared_ptr<Node> n = tail_;
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

    std::unique_ptr<D> Extract(){
        std::lock_guard lock(mutex_);
        if (head_ != nullptr) {
            std::unique_ptr<D> ret = std::move(head_->data);
            head_ = head_->next;
            if (head_ == nullptr)
                tail_ = nullptr;
            else
                head_->prev = nullptr;
            return ret;
        }
        return nullptr;
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
        std::unique_ptr<D> data;
        std::shared_ptr<Node> next;
        std::shared_ptr<Node> prev;
    };    
    std::shared_ptr<Node> head_ = nullptr;
    std::shared_ptr<Node> tail_ = nullptr;
    std::mutex mutex_;
};

#endif
