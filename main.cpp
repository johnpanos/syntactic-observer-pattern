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
    std::vector<std::function<void(T, T)> > observers;

    inline void
    operator=(const T new_value)
    {
        for (auto observer : this->observers)
        {
            observer(this->value, new_value);
        }
        this->value = new_value;
    }

    /**
     * @brief Observer will be called in the event that the backing value changes
     *
     * @param observer Lambda to be called
     */
    void add_observer(std::function<void(T, T)> observer)
    {
        this->observers.push_back(observer);
    }
};

class AnimationCore
{
public:
    static int64_t now()
    {
        return duration_cast<milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief Abstract linear interpolation
 *
 * @tparam T
 * @param start
 * @param end
 * @param prog
 * @return T
 */
template <typename T>
T lerp(T start, T end, double prog)
{
    return (T)start + (prog * (end - start));
}

template <typename T>
class Animation
{
private:
    int64_t start_time;
    int64_t end_time;

public:
    /**
     * @brief Sets up start and end times for an animation
     *
     */
    void prep()
    {
        int64_t now = AnimationCore::now();
        this->start_time = now;
        this->end_time = now + this->duration;
    }

    ObservableProperty<T> *property;
    T start;
    T end;

    int64_t duration;

    /**
     * @brief Get the progress object
     *
     * @param now Timestamp in milliseconds representing the current time
     * @return float Normalized range between 0.0 and 1.0 representing animation progress
     */
    float get_progress(int64_t now)
    {
        int64_t delta = now - this->start_time;

        if (delta == 0)
        {
            return 0.0f;
        }

        float dt = (float)((double)delta / (double)this->duration);
        return dt;
    }

    /**
     * @brief Returns value of an
     *
     * @param prog Normalized range between 0.0 and 1.0 representing animation progress
     * @return T Whatever the value is. For example, float, int, or Color
     */
    T get_value_for_progress(float prog)
    {
        return lerp(this->start, this->end, prog);
    }

    /**
     * @brief Progresses the animation given the current timestamp
     *
     * @param now Time in ms that has elapsed since the steady_clock epoch
     */
    void tick(int64_t now)
    {
        float prog = this->get_progress(now);

        if (prog > 1)
        {
            this->property->value = this->end;
        }
        else if (prog < 0)
        {
            this->property->value = this->start;
        }
        else
        {
            this->property->value = this->get_value_for_progress(prog);
        }
    }

    /**
     * @brief
     *
     * @param now Time in ms that has elapsed since the steady_clock epoch
     * @return true The animation has finished
     * @return false The animation is in progress
     */
    bool finished(int64_t now)
    {
        return this->get_progress(now) >= 1.0;
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

int main()
{
    // Example view
    View *my_view = new View();

    // Animate function observer when value is changed
    auto observe = [&my_view](float old, float current)
    {
        // Create new animation for the current property
        Animation<float> anim;
        anim.property = &my_view->frame.size.width;
        anim.start = old;
        anim.end = current;
        anim.duration = 250; // ms

        // Print start and end values
        std::cout << "start: " << anim.start;
        std::cout << " | end: " << anim.end << "\n";

        // Setup start and end times for the animation
        anim.prep();

        int64_t now = AnimationCore::now();

        // Run animation until it's complete
        while (!anim.finished(now))
        {
            // Tick using the delta from the relative steady_clock epoch
            now = AnimationCore::now();
            anim.tick(now);

            std::cout << "Current value: " << anim.property->value << "\n";

            // Update at 120hz
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 120));
        }
    };

    // Add observer function to be called on width change
    my_view->frame.size.width.add_observer(observe);

    // Observer function will be called on assignment
    my_view->frame.size.width = 500.64f;
}