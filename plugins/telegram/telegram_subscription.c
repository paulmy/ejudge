/* -*- mode: c -*- */

/* Copyright (C) 2016 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "telegram_subscription.h"
#include "mongo_conn.h"

#include "ejudge/bson_utils.h"
#include "ejudge/xalloc.h"
#include "ejudge/osdeps.h"
#include "ejudge/errlog.h"

#include <mongo.h>

#include <errno.h>
#include <string.h>

#define TELEGRAM_SUBSCRIPTIONS_TABLE_NAME "telegram_subscriptions"

struct telegram_subscription *
telegram_subscription_free(struct telegram_subscription *sub)
{
    if (sub) {
        xfree(sub->_id);
        xfree(sub->bot_id);
        memset(sub, 0xff, sizeof(*sub));
        xfree(sub);
    }
    return NULL;
}

struct telegram_subscription *
telegram_subscription_create(const unsigned char *bot_id, int contest_id, int user_id)
{
    struct telegram_subscription *sub = NULL;
    unsigned char buf[1024];

    if (!bot_id || !*bot_id || contest_id <= 0 || user_id <= 0) return NULL;
    snprintf(buf, sizeof(buf), "%s-%d-%d", bot_id, contest_id, user_id);

    XCALLOC(sub, 1);
    sub->_id = xstrdup(buf);
    sub->bot_id = xstrdup(bot_id);
    sub->contest_id = contest_id;
    sub->user_id = user_id;
    return sub;
}

struct telegram_subscription *
telegram_subscription_parse_bson(struct _bson *bson)
{
    struct telegram_subscription *sub = NULL;
    bson_cursor *bc = NULL;

    XCALLOC(sub, 1);
    bc = bson_cursor_new(bson);
    while (bson_cursor_next(bc)) {
        const unsigned char *key = bson_cursor_key(bc);
        if (!strcmp(key, "_id")) {
            if (ej_bson_parse_string(bc, "_id", &sub->_id) < 0) goto cleanup;
        } else if (!strcmp(key, "bot_id")) {
            if (ej_bson_parse_string(bc, "bot_id", &sub->bot_id) < 0) goto cleanup;
        } else if (!strcmp(key, "user_id")) {
            if (ej_bson_parse_int(bc, "user_id", &sub->user_id, 1, 1, 0, 0) < 0) goto cleanup;
        } else if (!strcmp(key, "contest_id")) {
            if (ej_bson_parse_int(bc, "contest_id", &sub->contest_id, 1, 0, 0, 0) < 0) goto cleanup;
        } else if (!strcmp(key, "review_flag")) {
            if (ej_bson_parse_int(bc, "review_flag", &sub->review_flag, 1, 0, 0, 0) < 0) goto cleanup;
        } else if (!strcmp(key, "reply_flag")) {
            if (ej_bson_parse_int(bc, "reply_flag", &sub->reply_flag, 1, 0, 0, 0) < 0) goto cleanup;
        } else if (!strcmp(key, "chat_id")) {
            if (ej_bson_parse_int64(bc, "chat_id", &sub->chat_id) < 0) goto cleanup;
        }
    }
    bson_cursor_free(bc);
    return sub;

cleanup:
    telegram_subscription_free(sub);
    return NULL;
}

struct _bson *
telegram_subscription_unparse_bson(const struct telegram_subscription *sub)
{
    if (!sub) return NULL;

    bson *b = bson_new();
    if (sub->_id && *sub->_id) {
        bson_append_string(b, "_id", sub->_id, strlen(sub->_id));
    }
    if (sub->bot_id && *sub->bot_id) {
        bson_append_string(b, "bot_id", sub->bot_id, strlen(sub->bot_id));
    }
    if (sub->user_id > 0) {
        bson_append_int32(b, "user_id", sub->user_id);
    }
    if (sub->contest_id > 0) {
        bson_append_int32(b, "contest_id", sub->contest_id);
    }
    if (sub->review_flag > 0) {
        bson_append_int32(b, "review_flag", sub->review_flag);
    }
    if (sub->reply_flag > 0) {
        bson_append_int32(b, "reply_flag", sub->reply_flag);
    }
    if (sub->chat_id) {
        bson_append_int64(b, "chat_id", sub->chat_id);
    }
    bson_finish(b);
    return b;
}

struct telegram_subscription *
telegram_subscription_fetch(struct mongo_conn *conn, const unsigned char *bot_id, int contest_id, int user_id)
{
    if (!mongo_conn_open(conn)) return NULL;

    unsigned char buf[1024];
    if (!bot_id || !*bot_id || contest_id <= 0 || user_id <= 0) return NULL;
    snprintf(buf, sizeof(buf), "%s-%d-%d", bot_id, contest_id, user_id);

    bson *query = NULL;
    mongo_packet *pkt = NULL;
    mongo_sync_cursor *cursor = NULL;
    bson *result = NULL;
    struct telegram_subscription *retval = NULL;

    query = bson_new();
    bson_append_string(query, "_id", buf, strlen(buf));
    bson_finish(query);

    pkt = mongo_sync_cmd_query(conn->conn, mongo_conn_ns(conn, TELEGRAM_SUBSCRIPTIONS_TABLE_NAME), 0, 0, 1, query, NULL);
    if (!pkt && errno == ENOENT) {
        goto cleanup;
    }
    if (!pkt) {
        err("mongo query failed: %s", os_ErrorMsg());
        goto cleanup;
    }
    bson_free(query); query = NULL;

    cursor = mongo_sync_cursor_new(conn->conn, conn->ns, pkt);
    if (!cursor) {
        err("mongo query failed: cannot create cursor: %s", os_ErrorMsg());
        goto cleanup;
    }
    pkt = NULL;
    if (mongo_sync_cursor_next(cursor)) {
        result = mongo_sync_cursor_get_data(cursor);
        if (result) {
            retval = telegram_subscription_parse_bson(result);
        }
    }

cleanup:
    if (result) bson_free(result);
    if (cursor) mongo_sync_cursor_free(cursor);
    if (pkt) mongo_wire_packet_free(pkt);
    if (query) bson_free(query);
    return retval;
}

int
telegram_subscription_save(struct mongo_conn *conn, const struct telegram_subscription *sub)
{
    if (!mongo_conn_open(conn)) return -1;
    int retval = -1;

    bson *b = telegram_subscription_unparse_bson(sub);
    bson *q = bson_new();
    bson_append_string(q, "_id", sub->_id, strlen(sub->_id));
    bson_finish(q);

    if (!mongo_sync_cmd_update(conn->conn, mongo_conn_ns(conn, TELEGRAM_SUBSCRIPTIONS_TABLE_NAME), MONGO_WIRE_FLAG_UPDATE_UPSERT, q, b)) {
        err("save_token: failed: %s", os_ErrorMsg());
        goto cleanup;
    }

    retval = 0;

cleanup:
    bson_free(q);
    bson_free(b);
    return retval;
}

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
