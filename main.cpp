#include <iostream>
#include <ctime>
#include "elevator.h"

#define MIN_STOREY_COUNT 5     // minimum number of storeys
#define MAX_STOREY_COUNT 20    // maximum number of storeys
#define MAX_STOREY_HIGHT 10    // meters
#define MAX_ELEVATOR_SPEED 10  // meters per second
#define MAX_DOOR_INTERVAL 20   // seconds
#define MAX_ELEVATOR_COUNT 8   // maximum number of elevators


int print_usage()
{
    std::cout << "Usage: elevator STOREY_COUNT HIHG SPEED INTERVAL ELEVATOR_COUNT\n\n"
              << "\t\tSTOREY_COUNT      number of storeys in the buiding ("<<MIN_STOREY_COUNT << " - " << MAX_STOREY_COUNT <<"),\n"
              << "\t\tHIGHT             storey's hight (in meters, " << MAX_STOREY_HIGHT << " maximum),\n"
              << "\t\tSPEED             elevator's speed (m/sec, " << MAX_ELEVATOR_SPEED << " maximum),\n"
              << "\t\tINTERVAL          opening/closing time interval (sec, " << MAX_DOOR_INTERVAL << " maximum),\n"
              << "\t\tELEVATOR_COUNT    number of elevators in the building (" << MAX_ELEVATOR_COUNT << " maximum)\n"
              << std::endl;
    return 0;
}

uint to_number(const std::string &s)
{
    try {
        return std::stoi(s);
    } catch(std::invalid_argument e){
        return 0;
    } catch (std::out_of_range e) {
        return 0;
    }
}

uint validate(uint value, uint max, uint min=1)
{
    if(value > max)
        return max;
    else if(value < min)
        return min;
    else
        return value;
}

int main(int argc, char* argv[])
{
    if (argc != 6)
        return print_usage();

    el::ElevatorData data(validate(std::stoi(argv[1]), MAX_STOREY_COUNT, MIN_STOREY_COUNT),
                          validate(std::stoi(argv[2]), MAX_STOREY_HIGHT),
                          validate(std::stoi(argv[3]), MAX_ELEVATOR_SPEED),
                          validate(std::stoi(argv[4]), MAX_DOOR_INTERVAL));

    const uint elnum = validate(std::stoi(argv[5]), MAX_ELEVATOR_COUNT);

    std::srand(std::time(0));

    try
    {
        std::mutex mtx;
        std::condition_variable cond;

        auto elptr = std::make_shared<el::ElevatorPool>(elnum, data);
        auto pptr  = std::make_shared<el::TaskInteractive>(&mtx, &cond);

        bool g_stop = false;

        while(!g_stop)
        {
            // Add some dummy passengers
            for(int i=0; i<std::rand()%data.size; ++i)
                elptr->add(std::make_shared<el::TaskDummy>(data.size));

            bool valid = false;
            while(!valid)
            {
                std::string s;
                std::cout << "Please, enter your location (1-"<< data.size << ") or 'q' to quit: ";
                std::cin >> s;
                std::cout << std::endl;
                if (s == "q")
                {
                    g_stop = true;
                    break;
                }
                pptr->location = to_number(s);

                if (pptr->location > data.size || pptr->location == 0)
                    std::cout << "Invalid location, choose in the range 1-" << data.size << std::endl;
                else
                    valid = true;
            }

            if(g_stop)
                break;

            elptr->add(pptr);

            std::cout << "Please wait, elevator is coming. Queue size: "
                      << elptr->size() << std::endl;

            std::unique_lock<std::mutex> lock(mtx);
            cond.wait(lock, [&](){return pptr->ready;});
            lock.unlock();

            valid = false;
            while(!valid)
            {
                std::string s;
                std::cout << "Please, enter storey to lift or 'q' to quit: ";
                std::cin >> s;
                std::cout << std::endl;
                if(s == "q")
                {
                    g_stop = true;
                    break;
                }
                pptr->destination = to_number(s);

                if (pptr->destination > data.size || pptr->destination == 0)
                    std::cout << "Invalid destination, choose in the range 1-" << data.size << std::endl;
                else
                    valid = true;
            }

            if(g_stop)
                break;

            lock.lock();
            pptr->ready = false;
            cond.notify_one();
            cond.wait(lock, [&](){return pptr->ready;});

            pptr->ready = false;
            pptr->destination = 0;
        }

        // Interactive task can wait for incoming information, so force it to quit.
        pptr->destination = 1;
        pptr->cond->notify_one();
    }
    catch(el::Exception e)
    {
        std::cout << e.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "Unexpected error" << std::endl;
    }

    return 0;
}
