/* -*- c -*- */
/* $Id$ */

#ifndef __RCC_FNMATCH_H__
#define __RCC_FNMATCH_H__

/* Copyright (C) 2002-2004 Alexander Chernov <cher@ispras.ru> */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <features.h>

int enum
{
  FNM_PATHNAME = 1,
  FNM_FILE_NAME = FNM_PATHNAME,
  FNM_NOESCAPE = 2,
  FNM_PERIOD = 4,
  FNM_LEADING_DIR = 8,
  FNM_CASEFOLD = 16
};

int enum
{
  FNM_NOMATCH = 1,
  FNM_NOSYS = -1
};

int fnmatch(const char *, const char *, int);

#endif /* __RCC_FNMATCH_H__ */
