/*
 * Date and time — calendar math + clock_gettime syscall.
 */
#include <lightc/time.h>
#include <lightc/syscall.h>
#include <lightc/format.h>

/* ------------------------------------------------------------------ */
/* Internal: civil date algorithms (Howard Hinnant)                   */
/* ------------------------------------------------------------------ */

/*
 * Convert days since 1970-01-01 to year/month/day.
 * Handles any year an int32_t holds, years before 0 included.
 */
static void days_to_date(int64_t days, int32_t *year, int32_t *month, int32_t *day) {
    /* Shift to March epoch (March 1, 0000) */
    days += 719468;  /* days from 0000-03-01 to 1970-01-01 */

    int64_t era = (days >= 0 ? days : days - 146096) / 146097;  /* 400-year era */
    int64_t doe = days - era * 146097;                           /* day of era [0, 146096] */
    int64_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;  /* year of era */
    int64_t y = yoe + era * 400;
    int64_t doy = doe - (365*yoe + yoe/4 - yoe/100);            /* day of year [0, 365] */
    int64_t mp = (5*doy + 2) / 153;                              /* month [0, 11] from March */
    int64_t d = doy - (153*mp + 2) / 5 + 1;                     /* day [1, 31] */
    int64_t m = mp + (mp < 10 ? 3 : -9);                        /* month [1, 12] */

    *year  = (int32_t)(y + (m <= 2));
    *month = (int32_t)m;
    *day   = (int32_t)d;
}

/*
 * Convert year/month/day to days since 1970-01-01.
 * Inverse of days_to_date.
 */
static int64_t date_to_days(int32_t year, int32_t month, int32_t day) {
    int64_t y = year;
    int64_t m = month;
    if (m <= 2) { y--; m += 9; } else { m -= 3; }

    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t yoe = y - era * 400;
    int64_t doy = (153 * m + 2) / 5 + day - 1;
    int64_t doe = yoe * 365 + yoe/4 - yoe/100 + doy;
    return era * 146097 + doe - 719468;
}

/*
 * Is this a leap year?
 */
static bool is_leap_year(int32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/*
 * Compute day-of-year (0 = January 1).
 */
static int32_t compute_day_of_year(int32_t year, int32_t month, int32_t day) {
    static const int32_t cumulative_days[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int32_t doy = cumulative_days[month - 1] + day - 1;
    if (month > 2 && is_leap_year(year)) {
        doy++;
    }
    return doy;
}

/* ------------------------------------------------------------------ */
/* Getting the current time                                           */
/* ------------------------------------------------------------------ */

lc_date_time lc_time_now(void) {
    lc_timespec ts;
    if (lc_kernel_get_clock(LC_CLOCK_REALTIME, &ts) < 0) {
        lc_date_time zero = {0};
        return zero;
    }
    return lc_time_from_unix(ts.seconds);
}

int64_t lc_time_now_unix(void) {
    lc_timespec ts;
    if (lc_kernel_get_clock(LC_CLOCK_REALTIME, &ts) < 0) {
        return 0;
    }
    return ts.seconds;
}

int64_t lc_time_now_monotonic(void) {
    return lc_time_now_ns(LC_CLOCK_MONOTONIC);
}

int64_t lc_time_now_ns(int32_t clock_id) {
    lc_timespec ts;
    if (lc_kernel_get_clock(clock_id, &ts) < 0) {
        return 0;
    }
    return ts.seconds * 1000000000LL + ts.nanoseconds;
}

/* ------------------------------------------------------------------ */
/* Conversion                                                         */
/* ------------------------------------------------------------------ */

lc_date_time lc_time_from_unix(int64_t epoch_seconds) {
    lc_date_time dt;

    int64_t days = epoch_seconds / 86400;
    int64_t rem  = epoch_seconds % 86400;

    /* Handle negative remainder (dates before 1970) */
    if (rem < 0) {
        rem += 86400;
        days--;
    }

    dt.hour   = (int32_t)(rem / 3600);
    dt.minute = (int32_t)((rem % 3600) / 60);
    dt.second = (int32_t)(rem % 60);

    /* Day of week: Jan 1, 1970 was Thursday (4) */
    int64_t dow = (days % 7 + 4) % 7;
    if (dow < 0) dow += 7;
    dt.day_of_week = (int32_t)dow;

    days_to_date(days, &dt.year, &dt.month, &dt.day);
    dt.day_of_year = compute_day_of_year(dt.year, dt.month, dt.day);

    return dt;
}

int64_t lc_time_to_unix(const lc_date_time *dt) {
    int64_t days = date_to_days(dt->year, dt->month, dt->day);
    return days * 86400 + dt->hour * 3600 + dt->minute * 60 + dt->second;
}

/* ------------------------------------------------------------------ */
/* Measuring elapsed time                                             */
/* ------------------------------------------------------------------ */

int64_t lc_time_start_timer(void) {
    return lc_time_now_monotonic();
}

int64_t lc_time_elapsed_nanoseconds(int64_t start) {
    return lc_time_now_monotonic() - start;
}

int64_t lc_time_elapsed_microseconds(int64_t start) {
    return (lc_time_now_monotonic() - start) / 1000;
}

int64_t lc_time_elapsed_milliseconds(int64_t start) {
    return (lc_time_now_monotonic() - start) / 1000000;
}

/* ------------------------------------------------------------------ */
/* Formatting                                                         */
/* ------------------------------------------------------------------ */

size_t lc_time_format(const lc_date_time *dt, char *buf, size_t buf_size) {
    lc_format fmt = lc_format_start(buf, buf_size);
    lc_format_add_signed_padded(&fmt, dt->year, dt->year < 0 ? 5 : 4, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->month, 2, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->day, 2, '0');
    lc_format_add_char(&fmt, ' ');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->hour, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->minute, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->second, 2, '0');
    return lc_format_finish(&fmt);
}

size_t lc_time_format_date(const lc_date_time *dt, char *buf, size_t buf_size) {
    lc_format fmt = lc_format_start(buf, buf_size);
    lc_format_add_signed_padded(&fmt, dt->year, dt->year < 0 ? 5 : 4, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->month, 2, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->day, 2, '0');
    return lc_format_finish(&fmt);
}

size_t lc_time_format_time(const lc_date_time *dt, char *buf, size_t buf_size) {
    lc_format fmt = lc_format_start(buf, buf_size);
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->hour, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->minute, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->second, 2, '0');
    return lc_format_finish(&fmt);
}

size_t lc_time_format_iso8601(const lc_date_time *dt, char *buf, size_t buf_size) {
    lc_format fmt = lc_format_start(buf, buf_size);
    lc_format_add_signed_padded(&fmt, dt->year, dt->year < 0 ? 5 : 4, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->month, 2, '0');
    lc_format_add_char(&fmt, '-');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->day, 2, '0');
    lc_format_add_char(&fmt, 'T');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->hour, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->minute, 2, '0');
    lc_format_add_char(&fmt, ':');
    lc_format_add_unsigned_padded(&fmt, (uint64_t)dt->second, 2, '0');
    lc_format_add_char(&fmt, 'Z');
    return lc_format_finish(&fmt);
}

/* ------------------------------------------------------------------ */
/* Sleep                                                              */
/* ------------------------------------------------------------------ */

/* A sleep that a signal interrupts sleeps again to the same deadline, so
 * the interruptions cost no more than the handlers' own time. Sleeping
 * again for the whole duration, as the sleeps did, never ended while a
 * signal came more often than the duration was long. */
void lc_time_sleep_until(int64_t deadline_ns) {
    if (deadline_ns <= 0) return;
    lc_timespec deadline;
    deadline.seconds     = deadline_ns / 1000000000LL;
    deadline.nanoseconds = deadline_ns % 1000000000LL;
    while (lc_kernel_clock_sleep(LC_CLOCK_MONOTONIC, LC_TIMER_ABSTIME, &deadline, NULL) == -LC_ERR_INTR) {
        /* interrupted: the same deadline again */
    }
}

/* now + amount * unit, or INT64_MAX when that does not fit. */
static int64_t deadline_after(int64_t amount, int64_t unit) {
    int64_t span, deadline;
    if (__builtin_mul_overflow(amount, unit, &span) ||
        __builtin_add_overflow(lc_time_now_ns(LC_CLOCK_MONOTONIC), span, &deadline))
        return INT64_MAX;
    return deadline;
}

void lc_time_sleep_milliseconds(int64_t ms) {
    if (ms <= 0) return;
    lc_time_sleep_until(deadline_after(ms, 1000000));
}

void lc_time_sleep_microseconds(int64_t us) {
    if (us <= 0) return;
    lc_time_sleep_until(deadline_after(us, 1000));
}
