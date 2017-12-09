#include "elevator.h"
#include <iostream>

using namespace el;

// Class Elevator

Elevator::Elevator(
        const ElevatorData &data,
        std::mutex *mutex,
        std::condition_variable *cond,
        std::deque<STask> *queue,
        std::atomic_int *active,
        std::condition_variable *done_cond,
        uint id):
    m_id(id),
    m_data(data),
    m_mutex(mutex),
    m_condition(cond),
    m_done_condition(done_cond),
    m_queue(queue),
    m_active(active),
    m_quit(false),
    m_storey(1),
    m_thread(&Elevator::worker, this)
{
    //
}

Elevator::~Elevator()
{
    //
}

void Elevator::quit()
{
    m_quit = true;
}

void Elevator::join()
{
    if (m_thread.joinable())
        m_thread.join(); 

    std::cout << "Elevator " << m_id
              << " finished." << std::endl;
}

void Elevator::move(uint to, bool out)
{
    int sign = (to > m_storey) ? 1 : -1; // up or down
    uint dist = std::max(to, m_storey) - std::min(to, m_storey);
    uint msec = m_data.hight*1000/m_data.speed; // how long does it take to lift one storey

    // Elevator moving emulation
    for(int i=0; i < dist; ++i)
    {
        if(m_quit)
            return;

        if (out)
        {
            std::cout << "Elevator: " << m_id
                      << " Floor: " << m_storey
                      << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
        m_storey += sign;
    }

    if(m_quit)
        return;

    if (out)
    {
        std::cout << "Elevator: " << m_id
                  << " Floor: " << m_storey
                  << " Door is opened!"
                  << std::endl;
    }

    // Wait while passengers get in or get out of elevater.
    std::this_thread::sleep_for(std::chrono::seconds(m_data.time));

    if (out)
    {
        std::cout << "Elevator: " << m_id
                  << " Floor: " << m_storey
                  << " Door is closed!"
                  << std::endl;
    }
}

void Elevator::worker()
{
    while(!m_quit)
    {
        std::unique_lock<std::mutex> lock(*m_mutex);

        m_condition->wait(lock, [&](){return (m_queue->size() > 0 || m_quit);});

        if (m_quit)
        {
            m_done_condition->notify_one();
            break;
        }

        ++(*m_active);

        STask task = m_queue->front(); m_queue->pop_front();
        lock.unlock();

        bool interactive = (dynamic_cast<TaskInteractive*>(task.get()) != nullptr);

        this->move(task->location, interactive);
        task->wait();
        this->move(task->destination, interactive);
        task->done();

        --(*m_active);
        m_done_condition->notify_one();
    }
}

// Class ElevatorPool

ElevatorPool::ElevatorPool(uint elnum, const ElevatorData &data):
    m_elevators(elnum)
{
    for(int i=0; i<elnum; ++i)
        m_elevators[i] = std::make_shared<Elevator>(
                    data, &m_mutex, &m_condition, &m_queue, &m_active, &m_done_condition, i);
}

ElevatorPool::~ElevatorPool()
{
    m_queue.clear();

    for(auto &e: m_elevators)
        e->quit();

    m_condition.notify_all();

    if( m_active > 0 )
    {
        // Wait for all working elevators finish their tasks.
        std::unique_lock<std::mutex> lock(m_done_mutex);
        m_done_condition.wait(lock, [&](){ return m_active == 0; } );
    }

    m_condition.notify_all(); // For those that were before m_condition.wait()

    for(auto &e: m_elevators)
        e->join();
}

uint ElevatorPool::size() const
{
    std::lock_guard<std::mutex> _(m_mutex);
    return m_queue.size();
}

void ElevatorPool::add(const STask &p)
{
    if (!p)
        throw Exception(Exception::bad_passenger);

    std::lock_guard<std::mutex> _(m_mutex);

    // Check whether "button" has already pushed.
    // Take under consideration an interactive tasks that
    // shouldn't be overwritten due to interactive logic.
    bool ignore = false;
    auto it = m_queue.begin();
    while (it != m_queue.end()) {
        if ((*it)->location == p->location) // if button already pushed
        {
            // Do not replace interactive!
            if(dynamic_cast<TaskInteractive*>((*it).get())==nullptr)
                *it = p;
            ignore = true;
        }
        ++it;
    }

    if (!ignore)
        m_queue.push_back(p);

    // Notify elevators.
    m_condition.notify_one();
}
