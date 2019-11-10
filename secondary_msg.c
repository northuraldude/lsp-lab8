/**
 * Дисциплина: Организация процессов и программировние в среде Linux
 * Разработал: Белоусов Евгений
 * Группа: 6305
 * Тема: Взаимодействие процессов на основе сообщений (часть 1: обмен сообщениями)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>


#define SIZE 256
#define QUEUE_NAME "/my_queue"


/**
 * Функция открывает существующую очередь сообщений и отправляет в неё сообщение.
 * Если очередь не существует, работа завершается.
 * 
 * Принимает параметры запуска:
 *  - время ожидания сообщения для программы, принимающей сообщение из очереди
 */

int main(int argc, char const *argv[])
{
    char buffer[SIZE];

    mqd_t queue;
    int msg_prio = 1;

    //Проверка количества передаваемых параметров в программу
    if (argc != 2)
    {
        fprintf(stderr, "Неверное количество параметров для запуска программы!\n");
        exit(EXIT_FAILURE);
    }

    //Открытие очереди (WRONLY для владельца)
    if ((queue = mq_open(QUEUE_NAME, O_WRONLY | O_NONBLOCK)) == (mqd_t) -1)
    {
        fprintf(stderr, "Не удалось открыть очередь!\n");
        //printf("%s - %d\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    printf("\nВведите сообщение: ");
    scanf("%s", buffer);

    //Отправка сообщения в очередь
    if (mq_send(queue, argv[1], sizeof(argv[1]), 2) == -1)
    {
        fprintf(stderr, "Не удалось отправить сообщение в очередь!\n");
        exit(EXIT_FAILURE);
    }
    if (mq_send(queue, buffer, SIZE, msg_prio) == -1)
    {
        fprintf(stderr, "Не удалось отправить сообщение в очередь!\n");
        exit(EXIT_FAILURE);
    }

    //Закрытие очереди
    if (mq_close(queue) == -1)
    {
        fprintf(stderr, "Не удалось закрыть очередь!\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
