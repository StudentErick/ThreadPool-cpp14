#include <iostream>
#include "ThreadPool.h"

class Task: public Object {
  public:
    void process() override {
        int t = rand() % 10;
        printf ("process %d ms\n", t * 100);
        std::this_thread::sleep_for (std::chrono::milliseconds (t * 100) );
        printf ("over %d ms\n", t * 100);
    }
};

class MyPolicy: public Policy {
  public:
    void adjust (std::shared_ptr<Object> obj) override {
        float p = rand() % 100;
        obj->setPriority (p);
    }
};

int main() {
    ThreadPool* it = ThreadPool::getInstance (4);
    it->setPolicy (std::make_shared<MyPolicy>() );
    for (int i = 0; i < 10; ++i) {
        it->addTask (std::make_shared<Task>() );
    }
    it->run();
    std::this_thread::sleep_for (std::chrono::seconds (5) );
    puts ("end");
    it->destroy();

    return 0;
}
