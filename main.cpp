//
//  main.cpp
//  Pipeline_MAZ
//
//  Created by Mariana on 4/23/19.
//  Copyright © 2019 Mariana. All rights reserved.
//

// I declare that the following source code was written by me, or provided
// by the instructor for this project. I understand that copying source
// code from any other source, providing source code to another student,
// or leaving my code on a public web site constitutes cheating.
// I acknowledge that  If I am found in violation of this policy this may result
// in a zero grade, a permanent record on file and possibly immediate failure of the class.

/* Reflection (1-2 paragraphs):
 This project was simple and easy but also I was stuck for a while because of the tiniest mistake that I couldnt
 find. I did learn a lot though, it helped me remember threading from Operating Systems, and it helped me
 understand it more clearly. I enjoy this project because it was the last one of the semester, it was simple and
 now we're done!
 */

// Note: Uses an atomic to decrement nprod.
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
using namespace std;

class ProducerConsumer {
    static queue<int> q, myQueue;
    static condition_variable q_cond, my_cond;
    static mutex q_sync, print, myq_sync;
    static atomic_size_t nprod, myFilter;
    static ofstream output;
    static size_t threadCounter;
    
public:
    static const size_t nprods = 4, myFilters = 10, ncons = 3;
    
    static void grouping(){
        size_t counter = 0, thread = threadCounter;
        threadCounter++;
        ofstream threadOutput("pipeline" + to_string(thread) + ".txt");
        
        for (;;){
            unique_lock<mutex> mylock(myq_sync);
            
            my_cond.wait(mylock, []() {return !myQueue.empty() || !myFilter; });
            
            if (myQueue.empty()){
                assert(!myFilter);
                break;
            }
            
            auto x = myQueue.front();
            
            if ((x%10) == thread){
                myQueue.pop();
                counter++;
                lock_guard<mutex> plck(print);
                threadOutput << "Thread: " << thread << "Processed: "<< x << endl;
            }
            
            mylock.unlock();
        }
        
        lock_guard<mutex> plck(print);
        cout << "Group " << thread << " has " << counter << " numbers." << endl;
    }
    
    static void consume() {
        for (;;) {
            // Get lock for sync mutex
            unique_lock<mutex> qlck(q_sync);
            
            // Wait for queue to have something to process
            q_cond.wait(qlck, []() {return !q.empty() || !nprod; });
            
            // if queue is empty or we are out of producers,
            // we then break out of the loop, terminating this thread
            if (q.empty()) {
                assert(!nprod);
                break;
            }
            
            //otherwise,  process next item in queue
            auto x = q.front();
            q.pop();
            
            unique_lock<mutex> mylock(myq_sync);
            
            if ((x % 3) != 0)
                myQueue.push(x);
            
            mylock.unlock();
            my_cond.notify_one();
            qlck.unlock();
            
            // Print trace of consumption
            lock_guard<mutex> plck(print);
            output << x << " consumed" << endl;
            
        }
        
        --myFilter;
        my_cond.notify_all();
    }
    
    static void produce(int i) {
        // Generate 10000 random ints
        srand(time(nullptr) + i * (i + 1));
        for (int i = 0; i < 1000; ++i) {
            int n = rand();     // Get random int
            
            // Get lock for queue; push int
            unique_lock<mutex> slck(q_sync);
            q.push(n);
            slck.unlock();
            q_cond.notify_one();
            
            // Get lock for print mutex
            lock_guard<mutex> plck(print);
            output << n << " produced" << endl;
        }
        
        // Notify consumers that a producer has shut down
        --nprod;
        q_cond.notify_all();
    }
};

queue<int> ProducerConsumer::q, ProducerConsumer::myQueue;
condition_variable ProducerConsumer::q_cond, ProducerConsumer::my_cond;
mutex ProducerConsumer::q_sync, ProducerConsumer::print,  ProducerConsumer::myq_sync;
ofstream ProducerConsumer::output("pipeline.out");
atomic_size_t ProducerConsumer::nprod(nprods);
atomic_size_t ProducerConsumer::myFilter(ncons);
size_t ProducerConsumer::threadCounter(0);

int main() {
    vector<thread> prods, cons, group;
    for (size_t i = 0; i < ProducerConsumer::myFilters; ++i)
        group.push_back(thread(&ProducerConsumer::grouping));
    for (size_t i = 0; i < ProducerConsumer::ncons; ++i)
        cons.push_back(thread(&ProducerConsumer::consume));
    for (size_t i = 0; i < ProducerConsumer::nprods; ++i)
        prods.push_back(thread(&ProducerConsumer::produce, i));
    
    // Join all threads
    for (auto &p : prods)
        p.join();
    for (auto &c : cons)
        c.join();
    for (auto &g : group)
        g.join();
    
    getchar();
}

