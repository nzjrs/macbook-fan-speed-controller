#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "pid.h"

static void         write_sys_file(const gchar *path, const gchar *str);
static unsigned int get_temperature(unsigned int nsensors);
static unsigned int set_fans(unsigned int rpm, unsigned int nfans);
static int          get_number_of_files_in_path(const char *fstr, int startidx);

#define LOGFILE_PATH    "/var/log/macbookfanspeed"
#define CPUTEMP_PATH    "/sys/devices/platform/"
#define APPLESMC_PATH   "/sys/devices/platform/applesmc.768/"
#define MIN_FAN         2000
#define MAX_FAN         7000

#define SETPOINT        55
#define DT              1.0

static void no_debug_logger (const gchar *log_domain,
                             GLogLevelFlags log_level,
                             const gchar *message,
                             gpointer user_data)
{
    ;
}

int main (int argc, char **argv)
{
    PID_t pid;
    int nfans, nsensors;

    /* moderate proportional constant for response, some integral constant
       so we approach the correct value, positive derivitive constant to resist
       rapid fan speed changes for transient CPU speed changes */
    pid_init(&pid, -400, -80, 20);
    /* some windup to prevent us from sitting at full fan speed when we cannot
       quite meet the target temperature */
    pid_enable_feature(&pid, PID_ENABLE_WINDUP, 2500);
    /* clamp output to safe fan range */
    pid_enable_feature(&pid, PID_OUTPUT_SAT_MIN, MIN_FAN);
    pid_enable_feature(&pid, PID_OUTPUT_SAT_MAX, MAX_FAN);
    pid_set(&pid, SETPOINT);

    /* look for --quiet */
    if (argc > 1 && argv[1] && g_strcmp0(argv[1], "--quiet") == 0)
        g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, no_debug_logger, NULL);
    else
        pid_enable_feature(&pid, PID_DEBUG, 0);

    /* get number of fans, numbering starts at 1 */
    nfans = get_number_of_files_in_path(APPLESMC_PATH"fan%d_manual", 1);
    /* get number of coretemp sensors, numbering starts at 0 */
    nsensors = get_number_of_files_in_path(CPUTEMP_PATH"coretemp.%d/temp1_input", 0);

    g_debug("FOUND: %d fans %d sensors", nfans, nsensors);

    while ( nfans && nsensors ) {
        float t,rpm;
        char *log;

        g_usleep(G_USEC_PER_SEC * DT);

        t  = get_temperature(nsensors) * 1e-3;
        rpm = pid_calculate(&pid, t, DT);
        set_fans(rpm, nfans);

        log = g_strdup_printf("%.1f",t);
        if (!g_file_set_contents(LOGFILE_PATH,log,-1,NULL))
            g_warning("ERROR: Failed to write to logfile "LOGFILE_PATH);
        g_free(log);

        g_debug("T: %f RPM: %f", t, rpm);
    }
    
    return 0;
}

static void
write_sys_file(const gchar *path, const gchar *str)
{
    FILE *f = fopen(path, "w");
    if (f) {
        if ( fprintf(f, "%s", str) != strlen(str) )
            g_critical("ERROR WRITING %s TO: %s", str, path);
        fclose(f);
    } else {
        g_critical("ERROR OPENING: %s", path);
    }
}

static unsigned int
get_temperature(unsigned int nsensors)
{
    unsigned int i, maxtemp;

    /* coretemp sensors are numbered from 0 */
    for (i = 0, maxtemp = 0; i < nsensors; i++) {
        gchar *tempstring = NULL;
        GError *err = NULL;
        gchar *path = g_strdup_printf(CPUTEMP_PATH"coretemp.%d/temp1_input", i);

        if ( g_file_get_contents(path, &tempstring, NULL, &err) ) {
            maxtemp = MAX( maxtemp, g_ascii_strtoull(tempstring, NULL, 10) );
        } else {
            g_critical("ERROR READING TEMP SENSOR: %s", err->message);
            g_error_free(err);
        }

        g_free(tempstring);
        g_free(path);
    }

    return maxtemp;
}

static unsigned int
set_fans(unsigned int rpm, unsigned int nfans)
{
    unsigned int i;

    /* fans are numbered from 1 */
    for (i = 1; i <= nfans; i++) {
        gchar *manual_path = g_strdup_printf(APPLESMC_PATH"fan%d_manual", i);
        gchar *speed_path = g_strdup_printf(APPLESMC_PATH"fan%d_output", i);
        gchar *speed = g_strdup_printf("%d", rpm);

        write_sys_file(manual_path, "1");
        write_sys_file(speed_path, speed);

        g_free(manual_path);
        g_free(speed_path);
        g_free(speed);
    }
    return TRUE;
}

static int 
get_number_of_files_in_path(const char *fstr, int startidx)
{
    int i,num;

    for (i = startidx, num = 0;; i++, num++) {
        char *path = g_strdup_printf(fstr, i);

        if ( !g_file_test(path, G_FILE_TEST_EXISTS) ) {
            g_free(path);
            break;
        }

        g_free(path);
    }
    return num;
}

