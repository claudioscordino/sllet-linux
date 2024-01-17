#include <iostream>
#include <functional>
#include <chrono>
#include <thread>

// https://stackoverflow.com/questions/48967706/how-to-pass-stdfunction-with-different-parameters-to-same-function
// https://stackoverflow.com/questions/16868129/how-to-store-variadic-template-arguments
// https://stackoverflow.com/questions/15537817/c-how-to-store-a-parameter-pack-as-a-variable
// https://stackoverflow.com/questions/29220248/is-it-possible-to-store-variadic-arguments-into-a-member-variable


template <typename ... Args>
class Delay {
public:
    Delay (uint64_t delay, bool oneshot = false): delay_(delay), oneshot_(oneshot), first_activation_(true) {}

    void schedule(std::function<void(Args...)> const & func, Args... as) {
        vec_.push_back(std::tuple<Args...> {as...});
        if (first_activation_) {
            first_activation_ = false;
            first_time_ = std::chrono::system_clock::now();
        } else {
            first_activation_ = true;
            auto duration = std::chrono::system_clock::now() - first_time_;
            auto millis = std::chrono::duration_cast<std::chrono::microseconds>(duration);
            if (millis >= delay_)
                exec(func);
        }
    }
private:
    std::chrono::microseconds delay_;
    bool oneshot_;
    bool first_activation_;
    std::vector<std::tuple<Args...>> vec_;
    std::chrono::time_point<std::chrono::system_clock> first_time_; 

    void exec(std::function<void(Args...)> const & func) {
        try {
            if (oneshot_)
                std::apply(func, vec_.back());
            else
                for (auto i: vec_)
                    std::apply(func, i);
        } catch(const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }
};


struct mydata {
    std::string text;
};

void myfunction(int a, mydata md)
{
    std::cout << "Hello world!" << std::endl;
    std::cout << a << std::endl;
    std::cout << md.text << std::endl;
}

int main()
{
    Delay<int, mydata> d (1);
    mydata md;
    md.text = "Cruel world";
    d.schedule(myfunction, 3, md);
    d.schedule(myfunction, 4, md);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    d.schedule(myfunction, 5, md);
}
