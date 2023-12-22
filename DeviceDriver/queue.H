#ifndef QUEUE_H
#define QUEUE_H
#include "thread.H"

class Queue {
private:
    struct Node {
        Thread* thread;
        Node* next;
    };

    Node* front;
    Node* rear;

public:
    Queue() : front(nullptr), rear(nullptr) {}
    void enqueue(Thread* thread) {
        Node* newNode = new Node{thread, nullptr};
        if (isEmpty()) {
            front = newNode;
            rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
    }

    Thread* dequeue() {
        if (isEmpty()) {
            return nullptr;
        }

        Thread* thread = front->thread;
        Node* temp = front;
        front = front->next;
        return thread;
    }

    bool isEmpty() {
        return front == nullptr;
    }
};

#endif
