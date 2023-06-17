#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lvgl.h>

#define LCD_WIDTH 384
#define LCD_HEIGHT 448
#define MULTIPLIER 100
static lv_color_t buf_1[LCD_WIDTH * MULTIPLIER] = {0};
static lv_color_t buf_2[LCD_WIDTH * MULTIPLIER] = {0};

lv_disp_drv_t disp_drv;
lv_disp_draw_buf_t disp_buf;
lv_disp_t *disp;
void frame_buffer_flush_callback(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

lv_fs_drv_t fs_drv;
bool drive_ready_cb(lv_fs_drv_t *drv);
void *drive_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
lv_fs_res_t drive_close_cb(lv_fs_drv_t *drv, void *file_p);
lv_fs_res_t drive_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
lv_fs_res_t drive_write_cb(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
lv_fs_res_t drive_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t drive_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);

void app_main(void)
{
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, LCD_WIDTH * MULTIPLIER);
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = frame_buffer_flush_callback;
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp = lv_disp_drv_register(&disp_drv);

    lv_fs_drv_init(&fs_drv);
    fs_drv.letter = 'A'; /*An uppercase letter to identify the drive */
    fs_drv.cache_size = 0;
    fs_drv.ready_cb = drive_ready_cb; /*Callback to tell if the drive is ready to use */
    fs_drv.open_cb = drive_open_cb;   /*Callback to open a file */
    fs_drv.close_cb = drive_close_cb; /*Callback to close a file */
    fs_drv.read_cb = drive_read_cb;   /*Callback to read a file */
    fs_drv.write_cb = drive_write_cb; /*Callback to write a file */
    fs_drv.seek_cb = drive_seek_cb;   /*Callback to seek in a file (Move cursor) */
    fs_drv.tell_cb = drive_tell_cb;   /*Callback to tell the cursor position  */
    lv_fs_drv_register(&fs_drv);

    for (;;)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
        lv_tick_inc(20);
    }
}

void frame_buffer_flush_callback(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    lv_disp_flush_ready(disp_drv);
}

bool drive_ready_cb(lv_fs_drv_t *drv)
{
    LV_UNUSED(drv);
    return true;
}

void *drive_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);

    const char *flags = "";
    if (mode == LV_FS_MODE_WR)
    {
        flags = "wb";
    }
    else if (mode == LV_FS_MODE_RD)
    {
        flags = "rb";
    }
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    {
        flags = "w+";
    }

    char buf[256];
    sprintf(buf, "/lvgl_storage/%s", path);
    FILE *f = fopen(buf, flags);
    if (f != NULL)
    {
        return f;
    }
    return f;
}

lv_fs_res_t drive_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    LV_UNUSED(drv);
    int result = fclose((FILE *)file_p);
    switch (result)
    {
    case 0:
    {
        return LV_FS_RES_OK;
        break;
    }
    default:
    {
        return LV_FS_RES_UNKNOWN;
    }
    }
}

lv_fs_res_t drive_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    LV_UNUSED(drv);
    *br = fread(buf, 1, btr, (FILE *)file_p);
    return (int32_t)(*br) < 0 ? LV_FS_RES_UNKNOWN : LV_FS_RES_OK;
}

lv_fs_res_t drive_write_cb(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    LV_UNUSED(drv);
    *bw = fwrite(buf, 1, btw, (FILE *)file_p);
    return (int32_t)(*bw) < 0 ? LV_FS_RES_UNKNOWN : LV_FS_RES_OK;
}

lv_fs_res_t drive_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    int result = fseek((FILE *)file_p, pos, whence);
    if (!result)
    {
        return LV_FS_RES_OK;
    }
    else
    {
        return LV_FS_RES_UNKNOWN;
    }
}

lv_fs_res_t drive_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{

    LV_UNUSED(drv);
    *pos_p = ftell((FILE *)file_p);
    if (*pos_p != -1)
    {
        return LV_FS_RES_OK;
    }
    else
    {
        return LV_FS_RES_UNKNOWN;
    }
}
