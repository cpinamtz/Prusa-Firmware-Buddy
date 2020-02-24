/*
 * wui_api.c
 * \brief
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_api.h"

#include "wui.h"
#include "filament.h"

#include "cmsis_os.h"
#include "stdarg.h"

#define BDY_WUI_API_BUFFER_SIZE 512
#define BDY_NO_FS_FLAGS         0  // no flags for fs_open
#define BDY_API_PRINTER_LEN     12 // length of "/api/printer" string
#define BDY_API_JOB_LEN         8  // length of "/api/job" string
#define X_AXIS_POS              0
#define Y_AXIS_POS              1
#define Z_AXIS_POS              2
// for data exchange between wui thread and HTTP thread
extern marlin_vars_t webserver_marlin_vars;
extern osMutexId wui_web_mutex_id;
static marlin_vars_t webserver_marlin_vars_copy;
// for storing /api/* data
static struct fs_file api_file;

const char *get_progress_str(void) {

    osStatus status = osMutexWait(wui_web_mutex_id, osWaitForever);
    if (status == osOK) {
        webserver_marlin_vars_copy = webserver_marlin_vars;
    }
    osMutexRelease(wui_web_mutex_id);

    const char *file_name = "test.gcode";
    uint8_t sd_percent_done = (uint8_t)(webserver_marlin_vars_copy.sd_percent_done);
    uint32_t print_duration = (uint32_t)(webserver_marlin_vars_copy.print_duration);

    return char_streamer("{"
                         "\"file\":\"%s\","
                         "\"total_print_time\":%d, "
                         "\"progress\":{\"precent_done\":%d}"
                         "}",
        file_name,
        print_duration,
        sd_percent_done);
}

const char *get_update_str(const char *header) {

    osStatus status = osMutexWait(wui_web_mutex_id, osWaitForever);
    if (status == osOK) {
        webserver_marlin_vars_copy = webserver_marlin_vars;
    }
    osMutexRelease(wui_web_mutex_id);

    int32_t actual_nozzle = (int32_t)(webserver_marlin_vars_copy.temp_nozzle);
    //int32_t target_nozzle = (int32_t)(webserver_marlin_vars_copy.target_nozzle);
    int32_t actual_heatbed = (int32_t)(webserver_marlin_vars_copy.temp_bed);
    //int32_t target_heatbed = (int32_t)(webserver_marlin_vars_copy.target_bed);

    double z_pos_mm = (double)webserver_marlin_vars_copy.pos[Z_AXIS_POS];
    //uint16_t print_speed = (uint16_t)(webserver_marlin_vars_copy.print_speed);
    //uint16_t flow_factor = (uint16_t)(webserver_marlin_vars_copy.flow_factor);
    //const char *filament_material = filaments[get_filament()].name;

    return char_streamer("%s\r\n\r\n{"
                         "\"temp_nozzle\":%d,"
                         "\"temp_bed\":%d,"
                         "\"z_axis\":%.2f,"
                         "}",
        header,
        actual_nozzle, actual_heatbed, z_pos_mm);
}

static void wui_api_job(struct fs_file *file) {

    const char *ptr = get_progress_str();

    uint16_t response_len = strlen(ptr);
    file->len = response_len;
    file->data = ptr;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS;
}

static void wui_api_printer(struct fs_file *file) {

    const char *ptr = get_update_str("");

    uint16_t response_len = strlen(ptr);
    file->len = response_len;
    file->data = ptr;
    file->index = response_len;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
}

struct fs_file *wui_api_main(const char *uri, struct fs_file *file) {

    file = &api_file;
    file->len = 0;
    file->data = NULL;
    file->index = 0;
    file->pextension = NULL;
    file->flags = BDY_NO_FS_FLAGS; // http server adds response header
    if (!strncmp(uri, "/api/printer", BDY_API_PRINTER_LEN) && (BDY_API_PRINTER_LEN == strlen(uri))) {
        wui_api_printer(file);
        return file;
    } else if (!strncmp(uri, "/api/job", BDY_API_JOB_LEN) && (BDY_API_JOB_LEN == strlen(uri))) {
        wui_api_job(file);
        return file;
    }
    return NULL;
}
