#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

using uint = unsigned int;

namespace el
{

/*!
 * \brief The TaskBase struct Base class for elevator's tasks.
 */
struct TaskBase
{
    uint location;      // storey's number where to take a passenger
    uint destination;   // destination storey

    TaskBase() : location(0), destination(0) {}
    virtual ~TaskBase() {}

    virtual void wait() = 0;
    virtual void done() = 0;
};

/*!
 * \brief The TaskDummy struct emulates random passengers.
 */
struct TaskDummy: public TaskBase
{
    TaskDummy(const uint &size)
    {
        location = std::rand() % size + 1;
        destination = std::rand() % size + 1;
    }

    void wait() override {
        // Pushing button simulation: one second for making a decision where to go.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    void done() override {}
};

/*!
 * \brief The TaskInteractive struct allows interact with user via std in/out.
 */
struct TaskInteractive : public TaskBase
{

    bool ready;
    std::mutex *mtx;
    std::condition_variable *cond;

    TaskInteractive(std::mutex *m, std::condition_variable *c):
        ready(false), mtx(m), cond(c) {}

    ~TaskInteractive() {
    }

    void wait() // wait destination
    {
        std::unique_lock<std::mutex> lock(*mtx);
        ready = true;
        cond->notify_one(); // ready to select a storey
        cond->wait(lock, [&](){return this->destination>0;});
    }

    void done()
    {
        std::unique_lock<std::mutex> lock(*mtx);
        ready = true;
        cond->notify_one();
    }
};

using STask = std::shared_ptr<TaskBase>;

/*!
 * \brief The Exception class
 */
class Exception: public std::exception
{
public:
    enum Type {
        bad_passenger
    };
    Exception(Exception::Type type): m_type(type) {}

    const char* what() const noexcept
    {
        switch (m_type) {
        case bad_passenger:
            return "Bad passenger!";
        default:
            break;
        }
        return "";
    }
private:
    Type m_type;
};


struct ElevatorData
{
    const uint size;
    const uint hight;
    const uint speed;
    const uint time;

    ElevatorData(uint _size, uint _hight, uint _speed, uint _time):
        size(_size),
        hight(_hight),
        speed(_speed),
        time(_time)
    { }
};

/*!
 * \brief The Elevator class emulates elevator.
 */
class Elevator
{
public:
    Elevator(const ElevatorData &data,
             std::mutex *mutex,
             std::condition_variable *cond,
             std::deque<STask> *queue,
             std::atomic_int *active,
             std::condition_variable *done_cond, uint id);

    ~Elevator();

    void quit();
    void join();

private:
    void worker();
    void move(uint to, bool out); // move to the given storey

private:
    uint m_id;
    uint m_storey;  // current storey
    const ElevatorData m_data;

private:
    bool m_quit;
    std::thread m_thread;

private:
    // Pool synchronization
    std::mutex* m_mutex;
    std::condition_variable* m_condition;
    std::deque<STask>* m_queue;
    std::atomic_int* m_active;
    std::condition_variable* m_done_condition;
};

using SElevator = std::shared_ptr<Elevator>;

/*!
 * \brief The ElevatorPool class - elevators' manager.
 */
class ElevatorPool 
{

public:
    ElevatorPool(uint elnum, const ElevatorData &data);
    ~ElevatorPool();

    /*!
     * \brief size Returns queue size.
     * \return Current queue size (number of unhandled tasks).
     */
    uint size() const;

    /*!
     * \brief add Add task to the queue and notify elevators about it.
     * \param p Task object.
     */
    void add(const STask &p);

private:
    // Workers
    std::vector<SElevator> m_elevators;
private:
    // Queue synchronization
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<STask> m_queue;
private:
    // Quit synchronization
    std::atomic_int m_active; // active jobs
    std::mutex m_done_mutex;
    std::condition_variable m_done_condition;
};

using SElevatorPool = std::shared_ptr<ElevatorPool>;

} // end namespace el

#endif // ELEVATOR_H
