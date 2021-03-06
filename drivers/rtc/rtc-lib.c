/**
 * Copyright (c) 2012 Anup Patel.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file rtc-lib.c
 * @author Anup Patel (anup@brainfault.org)
 * @brief Real-Time Clock Library source
 *
 * This source has been adapted from Linux 3.xx.xx drivers/rtc/rtc-lib.c
 *
 * rtc and date/time utility functions
 *
 * Copyright (C) 2005-06 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * The original code is licensed under the GPL.
 */

#include <vmm_error.h>
#include <vmm_wallclock.h>
#include <vmm_modules.h>
#include <libs/mathlib.h>
#include <drv/rtc.h>

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

bool rtc_is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}
VMM_EXPORT_SYMBOL(rtc_is_leap_year);

unsigned int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] +
		(rtc_is_leap_year(year) && month == 1);
}
VMM_EXPORT_SYMBOL(rtc_month_days);

unsigned int rtc_year_days(unsigned int day,
				unsigned int month, unsigned int year)
{
	return rtc_ydays[rtc_is_leap_year(year)][month] + day-1;
}
VMM_EXPORT_SYMBOL(rtc_year_days);

bool rtc_valid_tm(struct rtc_time *tm)
{
	if (tm->tm_year < 70
		|| ((unsigned)tm->tm_mon) >= 12
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
		|| ((unsigned)tm->tm_hour) >= 24
		|| ((unsigned)tm->tm_min) >= 60
		|| ((unsigned)tm->tm_sec) >= 60) {
		return FALSE;
	}
	return TRUE;
}
VMM_EXPORT_SYMBOL(rtc_valid_tm);

/* WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines where long is 32-bit!
 */
int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time)
{
	if (!tm || !time) {
		return VMM_EFAIL;
	}

	*time = (unsigned long)vmm_wallclock_mktime(tm->tm_year + 1900,
						    tm->tm_mon + 1,
						    tm->tm_mday,
						    tm->tm_hour,
						    tm->tm_min,
						    tm->tm_sec);

	return VMM_OK;
}
VMM_EXPORT_SYMBOL(rtc_tm_to_time);

time64_t rtc_tm_to_time64(struct rtc_time *tm)
{
	if (!tm) {
		return VMM_EINVALID;
	}

	return (time64_t)vmm_wallclock_mktime(tm->tm_year + 1900,
						tm->tm_mon + 1,
						tm->tm_mday,
						tm->tm_hour,
						tm->tm_min,
						tm->tm_sec);
}
VMM_EXPORT_SYMBOL(rtc_tm_to_time64);

static void __rtc_time_to_tm(u64 time, struct rtc_time *tm)
{
	unsigned int month, year;
	int days;

	days = udiv64(time, 86400);
	time -= (unsigned int) days * 86400;

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	year = 1970 + days / 365;
	days -= (year - 1970) * 365
		+ LEAPS_THRU_END_OF(year - 1)
		- LEAPS_THRU_END_OF(1970 - 1);
	if (days < 0) {
		year -= 1;
		days += 365 + rtc_is_leap_year(year);
	}
	tm->tm_year = year - 1900;
	tm->tm_yday = days + 1;

	for (month = 0; month < 11; month++) {
		int newdays;

		newdays = days - rtc_month_days(month, year);
		if (newdays < 0)
			break;
		days = newdays;
	}
	tm->tm_mon = month;
	tm->tm_mday = days + 1;

	tm->tm_hour = udiv64(time, 3600);
	time -= tm->tm_hour * 3600;
	tm->tm_min = udiv64(time, 60);
	tm->tm_sec = time - tm->tm_min * 60;
}

void rtc_time_to_tm(unsigned long time, struct rtc_time *tm)
{
	__rtc_time_to_tm(time, tm);
}
VMM_EXPORT_SYMBOL(rtc_time_to_tm);

void rtc_time64_to_tm(time64_t time, struct rtc_time *tm)
{
	__rtc_time_to_tm(time, tm);
}
VMM_EXPORT_SYMBOL(rtc_time64_to_tm);
