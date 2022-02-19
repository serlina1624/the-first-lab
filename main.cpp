#include <iostream>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <unistd.h>

bool g_debug = false;
int g_consumers_count = 0;
int g_sleep_milliseconds = 0;
std::vector<pthread_t> g_consumer_ids;
int global = 0;
bool ready = false;
bool finish = false;

pthread_cond_t producer_condition;
pthread_cond_t consumer_condition;
pthread_mutex_t mutex;

void* producer_routine(void* arg) {
    (void)arg;
    if (g_debug) {
        //std::cout << "Я продюсер! Мой номер: " << pthread_self()<< std:: endl;
        std::ifstream file_stream("in.txt");
        std::string value;
        if (file_stream.is_open()) {
            int value;
            while (file_stream >> value) {
                pthread_mutex_lock(&mutex);// захватываем примитив
                global = value; // обновляем переменную
                ready = true;
                pthread_cond_signal(&consumer_condition);
                while (ready)
                    pthread_cond_wait(&producer_condition, &mutex);
                pthread_mutex_unlock(&mutex);
            }
            finish = true;
            pthread_mutex_lock(&mutex);
            pthread_cond_broadcast(&consumer_condition);
            pthread_mutex_unlock(&mutex);
        }
    }
    return nullptr;
}

void* consumer_routine(void* arg) {
    (void)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    int* sum = new int(0);

    //std::cout << "Я консумер! Мой номер: " << pthread_self() << std:: endl;
    while (!finish) {
        pthread_mutex_lock(&mutex);
        while (!ready && !finish)
            pthread_cond_wait(&consumer_condition, &mutex);
        if (!ready) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(sum);
        }

        (*sum) += global;
        //if (g_debug)
       //  std::cout << "(" << pthread_self() << ' ' << sum << " " << ")" << std::endl;
        ready = false;
        pthread_cond_signal(&producer_condition);
        pthread_mutex_unlock(&mutex);
        usleep(rand() % (g_sleep_milliseconds));
    }

    return sum;
}

void* consumer_interruptor_routine(void* arg) {
    (void)arg;

    while (!finish) {
        unsigned long index = std::rand() % (g_consumer_ids.size());
        pthread_cancel(g_consumer_ids.at(index));
    }
    return nullptr;
}

int run_threads() {

    pthread_t producer_id;
    pthread_t interaptor_id;
    g_consumer_ids.resize(g_consumers_count);
    int concl_sum = 0;

    if (g_debug) {
        //std::cout << "Создаем потоки, номер main: " << pthread_self()<< std:: endl;
        pthread_create(&producer_id, NULL, producer_routine, NULL);
        pthread_create(&interaptor_id, nullptr, &consumer_interruptor_routine, NULL);

        for (int i = 0; i < g_consumers_count; i++)
            pthread_create(&g_consumer_ids[i], NULL, consumer_routine, NULL);

        for (int i = 0; i < g_consumers_count; i++) {
            void* sum = 0;
            pthread_join(g_consumer_ids[i], &sum);
            concl_sum += *((int*)sum);
        }

        pthread_join(producer_id, NULL);
        pthread_join(interaptor_id, nullptr);
    }
    return concl_sum;
}

int main(int argc, char* argv[]) {
    g_consumers_count = atoi(argv[1]);
    g_sleep_milliseconds = atoi(argv[2]);

    if (argc == 4)
        g_debug = true;

    if (g_debug)
        std::cout << "Приняты параметры: потоков " << g_consumers_count << ", спим " << g_sleep_milliseconds << std::endl;

    std::cout << run_threads() << std::endl;
    return 0;
}