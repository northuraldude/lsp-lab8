/**
 * Дисциплина: Организация процессов и программировние в среде Linux
 * Разработал: Белоусов Евгений
 * Группа: 6305
 * Тема: Взаимодействие процессов на основе сообщений (часть 2: параллельное чтение из файла)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <time.h>

#define BUFFER_SIZE 64

void closing_smq(int queue);

struct request
{
    long receiver;
    int sender;
    int qid;
    struct tm tm;
};

struct answer
{
    long receiver;
    int sender;
    struct tm tm;
};

/**
 * Функция создаёт очередь (если таковой ещё нет) и локальную очередь сообщений,
 * затем запрашивает разрешение на чтение файла у двух других запущенных программ,
 * после получения ответов на запросы, читает файл, выводит его содержимое на экран,
 * отвечает на запросы других двух программ, если таковые не были обработаны ранее
 * 
 * Принимает параметры запуска:
 *  - номер программы (от 1 до 3)
 */
int main(int argc, char const *argv[])
{
    int prog = atoi(argv[1]);
    char buffer[BUFFER_SIZE];

    int qid_req, qid_ans;
    key_t q_shared = 27;
    int req1, req2;
    struct request request[4];
    struct answer answer[4];

    time_t T = time(NULL);
    struct tm t;

    FILE *fd;
    
    int req1_handled = 0, req2_handled = 0;

    //Проверка количества передаваемых параметров в программу
    if (argc != 2)
    {
        fprintf(stderr, "Неверное количество параметров для запуска программы!\n");
        exit(EXIT_FAILURE);
    }
    //Определение специфики работы нужной программы
    switch (prog)
    {
        case 1:
            req1 = 2;
            req2 = 3;
            break;

        case 2:
            req1 = 1;
            req2 = 3;
            break;

        case 3:
            req1 = 1;
            req2 = 2;
            break;
        
        default:
            fprintf(stderr, "Некорректный параметр для запуска программы!\n");
            exit(EXIT_FAILURE);
            break;
    }

    //Создание общей очереди и проверка на её существование
    if (msgget(q_shared, IPC_CREAT | IPC_EXCL | 0666) != -1)
    {
        printf("Общая очередь создана программой %d!\n", prog);
    }
    if((qid_req = msgget(q_shared, IPC_CREAT | 0666)) == -1)
    {
        fprintf(stderr, "Не удалось создать общую очередь!\n");
        printf("%s - %d\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    //Создание локальной очереди программы для приёма ответов
    if((qid_ans = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1)
    {
        fprintf(stderr, "Не удалось создать локальную очередь!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }  
    printf("Локальная очередь создана программой %d!\n", prog);

    //Формирование и отправка первого запроса другой программе
    request[0].sender = prog;
    request[0].receiver = req1;
    request[0].qid = qid_ans;
    request[0].tm = *localtime(&T);
    if(msgsnd(qid_req, &request[0], sizeof(request[0]), 0) == -1) 
    {
        fprintf(stderr, "Не удалось отправить первый запрос в общую очередь!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    printf("Программа %d отправила запрос программе %d (%d:%d:%d)\n", prog, req1, request[0].tm.tm_hour, request[0].tm.tm_min, request[0].tm.tm_sec);
    
    //Формирование и отправка второго запроса другой программе
    request[1].sender = prog;
    request[1].receiver = req2;
    request[1].qid = qid_ans;
    request[1].tm = *localtime(&T);
    if(msgsnd(qid_req, &request[1], sizeof(request[1]), 0) == -1) 
    {
        fprintf(stderr, "Не удалось отправить второй запрос в общую очередь!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    printf("Программа %d отправила запрос программе %d (%d:%d:%d)\n", prog, req2, request[1].tm.tm_hour, request[1].tm.tm_min, request[1].tm.tm_sec);
    
    
    //Приём запроса от одной программы
    if (msgrcv(qid_req, &request[2], sizeof(request[2]), prog, 0) == -1)
    {
        fprintf(stderr, "Не удалось получить сообщение из общей очереди!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    t = *localtime(&T);
    printf("Программа %d получила запрос от программы %d (%d:%d:%d)\n", prog, request[2].sender, t.tm_hour, t.tm_min, t.tm_sec);
    //Обработка запроса
    if ((request[2].tm.tm_hour < request[0].tm.tm_hour) || ((request[2].tm.tm_min < request[0].tm.tm_min)) || ((request[2].tm.tm_sec <= request[0].tm.tm_sec)))
    {
        req1_handled = 1;
        answer[0].sender = prog;
        answer[0].receiver = request[2].sender;
        answer[0].tm = *localtime(&T);
        //Отправка ответа на запрос
        if(msgsnd(request[2].qid, &answer[0], sizeof(answer[0]), 0) == -1) 
        {
            fprintf(stderr, "Не удалось отправить ответ на запрос!\n");
            printf("%s - %d\n", strerror(errno), errno);
            closing_smq(qid_req);
            exit(EXIT_FAILURE);
        }
        printf("Программа %d ответила программе %ld (%d:%d:%d)\n", prog, answer[0].receiver, answer[0].tm.tm_hour, answer[0].tm.tm_min, answer[0].tm.tm_sec);
    }
    
    //Приём запроса от другой программы
    if (msgrcv(qid_req, &request[3], sizeof(request[3]), prog, 0) == -1)
    {
        fprintf(stderr, "Не удалось получить сообщение из общей очереди!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    t = *localtime(&T);
    printf("Программа %d получила запрос от программы %d (%d:%d:%d)\n", prog, request[3].sender, t.tm_hour, t.tm_min, t.tm_sec);
    //Обработка запроса
    if ((request[3].tm.tm_hour < request[1].tm.tm_hour) || ((request[3].tm.tm_min < request[1].tm.tm_min)) || ((request[3].tm.tm_sec <= request[1].tm.tm_sec)))
    {
        req2_handled = 1;
        answer[1].sender = prog;
        answer[1].receiver = request[3].sender;
        answer[1].tm = *localtime(&T);
        //Отправка ответа на запрос
        if(msgsnd(request[3].qid, &answer[1], sizeof(answer[1]), 0) == -1) 
        {
            fprintf(stderr, "Не удалось отправить ответ на запрос!\n");
            printf("%s - %d\n", strerror(errno), errno);
            closing_smq(qid_req);
            exit(EXIT_FAILURE);
        }
        printf("Программа %d ответила программе %ld (%d:%d:%d)\n", prog, answer[1].receiver, answer[1].tm.tm_hour, answer[1].tm.tm_min, answer[1].tm.tm_sec);
    }

    //Приём ответа на запрос от одной программы
    if (msgrcv(qid_ans, &answer[2], sizeof(answer[2]), prog, 0) == -1)
    {
        fprintf(stderr, "Не удалось получить ответ на запрос!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    t = *localtime(&T);
    printf("Программа %d получила ответ на запрос от программы %d (%d:%d:%d)\n", prog, answer[2].sender, t.tm_hour, t.tm_min, t.tm_sec);
    //Приём ответа на запрос от другой программы
    if (msgrcv(qid_ans, &answer[3], sizeof(answer[3]), prog, 0) == -1)
    {
        fprintf(stderr, "Не удалось получить ответ на запрос!\n");
        printf("%s - %d\n", strerror(errno), errno);
        closing_smq(qid_req);
        exit(EXIT_FAILURE);
    }
    t = *localtime(&T);
    printf("Программа %d получила ответ на запрос от программы %d (%d:%d:%d)\n", prog, answer[3].sender, t.tm_hour, t.tm_min, t.tm_sec);

    //Работа с файлом
    //Открытие файла в режиме "только для чтения"
    if ((fd = fopen("./../parallel.txt", "r")) == NULL)
    {
        fprintf(stderr, "Не удалось открыть файл!\n");
        exit(EXIT_FAILURE);
    }
    //Чтение из файла и вывод в стандартный поток вывода
    while (fgets(buffer, BUFFER_SIZE, fd) != NULL)
    {
        printf("\nДанные: %s\n", buffer);
    }
    //Закрытие файла
    fclose(fd);
    
    //Обработка ещё не обслуженных запросов
    if (req2_handled == 0)
    {        
        answer[1].sender = prog;
        answer[1].receiver = request[3].sender;
        answer[1].tm = *localtime(&T);
        if(msgsnd(request[3].qid, &answer[1], sizeof(answer[1]), 0) == -1) 
        {
            fprintf(stderr, "Не удалось отправить ответ на запрос!\n");
            printf("%s - %d\n", strerror(errno), errno);
            closing_smq(qid_req);
            exit(EXIT_FAILURE);
        }
        printf("Программа %d ответила программе %ld (%d:%d:%d)\n", prog, answer[1].receiver, answer[1].tm.tm_hour, answer[1].tm.tm_min, answer[1].tm.tm_sec);
    }
    if (req1_handled == 0)
    {
        answer[0].sender = prog;
        answer[0].receiver = request[2].sender;
        answer[0].tm = *localtime(&T);
        if(msgsnd(request[2].qid, &answer[0], sizeof(answer[0]), 0) == -1) 
        {
            fprintf(stderr, "Не удалось отправить ответ на запрос!\n");
            printf("%s - %d\n", strerror(errno), errno);
            closing_smq(qid_req);
            exit(EXIT_FAILURE);
        }
        printf("Программа %d ответила программе %ld (%d:%d:%d)\n", prog, answer[0].receiver, answer[0].tm.tm_hour, answer[0].tm.tm_min, answer[0].tm.tm_sec);
    }
    
    //Удаление локальной очереди сообщений
    msgctl(qid_ans, IPC_RMID, NULL);
    printf("Локальная очередь сообщений программы %d удалена!\n", prog);

    //Удаление общей очереди сообщений
    if (req1_handled == 1 && req2_handled == 1)
    {
        closing_smq(qid_req);
    }

    return 0;
}

/**
 * Функция выполняет уничтожение общей очереди сообщений
 * 
 * Принимает: идентификатор очереди сообщений
 */
void closing_smq(int queue)
{
    msgctl(queue, IPC_RMID, NULL);
    printf("Общая очередь сообщений удалена!\n");
}