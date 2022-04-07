#include <iostream>
#include <vector>
#include <functional>
#include <chrono>
#include <ctime>
#include <thread>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

#define UIFloat ObservableProperty<float>
#define UIInt ObservableProperty<int>

template <typename T>
struct ObservableProperty
{
    T value;
    std::vector<std::function<void(T, T)>> observers;

    inline void
    operator=(const T new_value)
    {
        for (auto observer : this->observers)
        {
            observer(this->value, new_value);
        }
        this->value = new_value;
    }

    void add_observer(std::function<void(T, T)> observer)
    {
        this->observers.push_back(observer);
    }
};

template <typename T>
class Animation
{
private:
    int64_t start_time;
    int64_t end_time;

public:
    void prep()
    {
        auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        this->start_time = millisec_since_epoch;
        this->end_time = millisec_since_epoch + this->duration;
    }
    ObservableProperty<T> *property;
    T start;
    T end;

    int64_t duration;

    float get_progress(int64_t now)
    {
        int64_t delta = now - this->start_time;

        if (delta == 0)
        {
            return 0.0f;
        }

        double dt = (double)delta / (double)this->duration;
        return dt;
    }
};

struct Point
{
    UIFloat x;
    UIFloat y;
};

struct Size
{
    UIFloat width;
    UIFloat height;
};

struct Rect
{
    Point position;
    Size size;
};

struct Color
{
    UIInt r;
    UIInt g;
    UIInt b;
};

class View
{
public:
    Color color;
    Rect frame;

    View()
    {
        this->frame.size.width.add_observer(
            [](float old, float current)
            {
                std::cout << "old width: " << old << "\n";
                std::cout << "current width: " << current << "\n";
            });
        this->frame.size.height.add_observer(
            [](float old, float current)
            {
                std::cout << "old height: " << old << "\n";
                std::cout << "current height: " << current << "\n";
            });
    }
};

double lerp(double a, double b, double t)
{
    return a + (t * (b - a));
}

int main()
{
    View *my_view = new View();
    my_view->frame.size.width = 420.10;
    my_view->frame.size.height = 123.123;
    my_view->frame.size.height = 234.234;

    std::cout << "height: " << my_view->frame.size.height.value << "\n";

    auto observe = [&my_view](int old, int current)
    {
        Animation<int> anim;
        anim.property = &my_view->color.r;
        anim.start = old;
        anim.end = current;
        anim.duration = 1000;

        std::cout << "start: " << anim.start;
        std::cout << " | end: " << anim.end << "\n";

        anim.prep();

        float prog = 0;
        do
        {
            // TODO: Make this use steady_clock
            int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            prog = anim.get_progress(now);

            if (prog > 1)
            {
                anim.property->value = anim.end;
            }
            else
            {
                anim.property->value = (int)(lerp((double)anim.start, (double)anim.end, prog));
            }

            std::cout << "value: " << anim.property->value << "\n";
        } while (prog < 1);
    };
    my_view->color.r.add_observer(observe);

    std::cout << "red: " << my_view->color.r.value << "\n";
    my_view->color.r = 255;
    my_view->color.r = 0;
}