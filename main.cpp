// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <future>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::map<std::string, std::chrono::steady_clock::time_point> has_login;  // 换成 std::chrono::seconds 之类的

std::shared_mutex mtx1;
std::shared_mutex mtx2;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    std::unique_lock grd(mtx1);
    User user = {password, school, phone};
    std::string str;
    if (users.emplace(username, user).second) {
        str = "register succeed";
    }
    else {
        str = "username has already been registered";
    }
    return str;
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::unique_lock grd(mtx2);
    auto now = std::chrono::steady_clock::now();
    if (has_login.find(username) != has_login.end()) {
        auto sec = now - has_login.at(username);  // chrono算时间差
        using double_ms = std::chrono::duration<double, std::milli>;
        double elapsed = std::chrono::duration_cast<double_ms>(sec).count();
        return std::to_string(elapsed) + " milliseconds within have logged in";
    }
    has_login[username] = now;

    {
        std::shared_lock grd(mtx1);
        if (users.find(username) == users.end()) {
            return "wrong username";
        }
        if (users.at(username).password != password) {
            return "wrong password";
        }
    }
    return "login success";
    
    
}

std::string do_queryuser(std::string username) {
    std::shared_lock grd(mtx1);
    if (users.find(username) != users.end()) {
        auto& user = users.at(username);
        std::stringstream ss;
        ss << "username: " << username << std::endl;
        ss << "school:" << user.school << std::endl;
        ss << "phone: " << user.phone << std::endl;
        return ss.str();
    }
    return "no such user";
}


struct ThreadPool {
    std::vector<std::future<int>> pool;
    void create(std::function<int()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        //std::thread thr(start);
        std::future<int> fret = std::async(std::launch::deferred, start);
        pool.push_back(std::move(fret));
    }
};

ThreadPool tpool;


namespace test {  // 测试用例？出水用力！
//std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string username[] = { "zhang", "wang", "peng", "hu" };
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
//std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
std::string school[] = { "jiubawu", "zhejiang", "jianqiao", "mashengligong" };
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 10; i++) {
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
            return 401;
        });
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
            return 402;
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
            return 403;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    for (auto& fret : tpool.pool) std::cout<<fret.get()<<std::endl;

    return 0;
}
