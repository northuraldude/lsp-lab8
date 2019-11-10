/**
 * Дисциплина: Организация процессов и программировние в среде Linux
 * Разработал: Белоусов Евгений
 * Группа: 6305
 * Тема: Взаимодействие процессов на основе сообщений (часть 1: обмен сообщениями)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/time.h>

#define SIZE 8192
#define QUEUE_NAME "/my_queue"

int stop = 0;

void timer_handler(int signum);
void closing_mq(mqd_t queue);

/**
 * Функция создаёт очередь сообщений и определённое время ожидает сообщение от второй программы,
 * если сообщение не успело поступить, то очередь уничтожается, а программа завершает работу.
 * При запуске второй программы принимается очередное время ожидания сообщения для данной программы
 * 
 * Принимает параметры запуска:
 *  - время ожидания сообщения от второй программы
 */

int main(int argc, char const *argv[])
{
    int start_time = atoi(argv[1]);
    int timeout;
    char buffer[SIZE];

    mqd_t queue;
    struct mq_attr queue_attr;
    int msg_prio;

    struct itimerval timer, zero;
    struct sigaction act;

    //Проверка количества передаваемых параметров в программу
    if (argc != 2)
    {
        fprintf(stderr, "Неверное количество параметров для запуска программы!\n");
        exit(EXIT_FAILURE);
    }

    //Очистка всей структуры нулями во избежание неопределённого поведения
    memset(&act, 0, sizeof(act));
    //Указываем на функцию новой обработки сигнала
    act.sa_handler = timer_handler;
    //Установка нового обработчика для сигнала SIGALRM (сигнал об окончании таймера времени процесса)
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        fprintf(stderr, "Не удалось установить обработчик сигнала!\n");
        return errno;
    }

    //Создание очереди (RDONLY для владельца)
    if ((queue = mq_open(QUEUE_NAME, O_CREAT|O_RDONLY, 0666, NULL)) == (mqd_t) -1)
    {
        fprintf(stderr, "Не удалось создать очередь!\n");
        printf("%s - %d\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    //Параметры таймера
    timer.it_value.tv_sec = start_time;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    //Параметры для сброса таймера
    zero.it_value.tv_sec = 0;
    zero.it_value.tv_usec = 0;
    zero.it_interval.tv_sec = 0;
    zero.it_interval.tv_usec = 0;

    //Вывод оставшегося промежутка времени
    printf("Сообщения могут быть приняты в течение следующего времени: %d [с]\n", start_time);
    //Установка таймера ITIMER_REAL для подсчёта времени работы
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        fprintf(stderr, "Не удалось установить таймер!\n");
        closing_mq(queue);
        return errno;
    }

    while (stop != 1)
    {
        //Принятие сообщения с новым временем таймера из очереди
        if (mq_receive(queue, buffer, SIZE, &msg_prio) == -1 || stop == 1)
        {
            if (stop == 1)
                break;
            fprintf(stderr, "Не удалось принять сообщение из очереди!\n");
            closing_mq(queue);
            exit(EXIT_FAILURE);
        }
        //Обнуление таймера
        if (setitimer(ITIMER_REAL, &zero, NULL) == -1)
        {
            fprintf(stderr, "Не удалось обнулить таймер!\n");
            closing_mq(queue);
            return errno;
        }
        //Новые параметры таймера
        timeout = atoi(buffer);
        if (timeout == 0)
        {
            timer.it_value.tv_sec = timeout;
            timer.it_value.tv_usec = 50;
        } else
        {
            timer.it_value.tv_sec = timeout;
            timer.it_value.tv_usec = 0;
        }
        
        //Вывод оставшегося промежутка времени
        printf("ОБНОВЛЕНО: Сообщения могут быть приняты в течение следующего времени: %d [с]\n", timeout);
        //Установка обновлённого таймера
        if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
        {
            fprintf(stderr, "Не удалось установить таймер!\n");
            closing_mq(queue);
            return errno;
        }
        
        //Принятие сообщения с информацией из очереди
        if (mq_receive(queue, buffer, SIZE, &msg_prio) == -1 || stop == 1)
        {
            if (stop == 1)
                break;
            fprintf(stderr, "Не удалось принять сообщение из очереди!\n");
            closing_mq(queue);
            exit(EXIT_FAILURE);
        } 
        //Вывод полученного сообщения
        printf("Сообщение (приоритет - %d): \"%s\"\n", msg_prio, buffer);
        
    }
    
    closing_mq(queue);

    return 0;
}

/**
 * Функция выполняет обработку сигнала SIGALARM
 * 
 * Принимает: номер сигнала
 */
void timer_handler(int signum)
{
    stop = 1;
    printf("Истекло время работы очереди! Завершение работы...\n");
}

/**
 * Функция выполняет уничтожение очереди сообщений
 * 
 * Принимает: дескриптор очереди сообщений
 */
void closing_mq(mqd_t queue)
{
    //Закрытие очереди
    if (mq_close(queue) == -1)
    {
        fprintf(stderr, "Не удалось закрыть очередь!\n");
        exit(EXIT_FAILURE);
    }

    //Удаление очереди
    if (mq_unlink(QUEUE_NAME) == -1)
    {
        fprintf(stderr, "Не удалось удалить очередь!\n");
        exit(EXIT_FAILURE);
    }

    printf("Очередь сообщений уничтожена!\n");
}