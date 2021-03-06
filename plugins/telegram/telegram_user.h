/* -*- c -*- */
#ifndef __TELEGRAM_USER_H__
#define __TELEGRAM_USER_H__

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

struct _bson;

struct telegram_user
{
    long long _id;
    unsigned char *username;
    unsigned char *first_name;
    unsigned char *last_name;
};

struct telegram_user *
telegram_user_free(struct telegram_user *tu);
struct telegram_user *
telegram_user_create(void);
struct telegram_user *
telegram_user_parse_bson(struct _bson *bson);
struct _bson *
telegram_user_unparse_bson(const struct telegram_user *tu);

struct mongo_conn;

struct telegram_user *
telegram_user_fetch(struct mongo_conn *conn, long long _id);
int
telegram_user_save(struct mongo_conn *conn, const struct telegram_user *tu);

#endif

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
