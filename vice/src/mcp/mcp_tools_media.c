/*
 * mcp_tools_media.c - MCP video/audio recording tool handlers
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "mcp_tools_internal.h"

#include "machine.h"
#include "machine-video.h"
#include "screenshot.h"
#include "vsync.h"

/*
 * NOTE on warp mode: recording is driven off the emulated frame/sample
 * counters (see gfxoutputdrv/ffmpegexedrv.c), not wall-clock time, so a
 * recording started here stays complete and correctly paced whether or
 * not warp mode is enabled - toggle warp separately with
 * vice.machine.config.set ("WarpMode": 1). For best results also make
 * sure "SoundEmulateOnWarp" is left enabled (the default) so audio
 * keeps being generated while warped.
 */

cJSON *mcp_tool_media_record_start(cJSON *params)
{
    cJSON *response, *path_item, *driver_item;
    const char *path;
    const char *driver = "FFMPEG";
    struct video_canvas_s *canvas;

    path_item = cJSON_GetObjectItem(params, "path");
    if (path_item == NULL || !cJSON_IsString(path_item) || path_item->valuestring[0] == '\0') {
        return mcp_error(MCP_ERROR_INVALID_PARAMS, "path is required");
    }
    path = path_item->valuestring;

    driver_item = cJSON_GetObjectItem(params, "driver");
    if (driver_item != NULL && cJSON_IsString(driver_item) && driver_item->valuestring[0] != '\0') {
        driver = driver_item->valuestring;
    }

    if (screenshot_is_recording()) {
        return mcp_error(MCP_ERROR_INVALID_PARAMS,
                          "A recording is already in progress; call vice.media.record_stop first");
    }

    canvas = machine_video_canvas_get(0);
    if (canvas == NULL) {
        return mcp_error(MCP_ERROR_INTERNAL_ERROR, "Cannot get video canvas");
    }

    log_message(mcp_tools_log, "Starting media recording: driver=%s, path=%s", driver, path);

    if (screenshot_save(driver, path, canvas) != 0) {
        return mcp_error(MCP_ERROR_INTERNAL_ERROR, "Failed to start recording (bad driver name, or codec unavailable)");
    }

    response = cJSON_CreateObject();
    if (response == NULL) {
        return mcp_error(MCP_ERROR_INTERNAL_ERROR, "Out of memory");
    }

    cJSON_AddStringToObject(response, "status", "ok");
    cJSON_AddStringToObject(response, "driver", driver);
    cJSON_AddStringToObject(response, "path", path);
    cJSON_AddBoolToObject(response, "warp_mode", vsync_get_warp_mode() ? 1 : 0);

    return response;
}

cJSON *mcp_tool_media_record_stop(cJSON *params)
{
    cJSON *response;

    if (!screenshot_is_recording()) {
        return mcp_error(MCP_ERROR_INVALID_PARAMS, "No recording is in progress");
    }

    screenshot_stop_recording();

    response = cJSON_CreateObject();
    if (response == NULL) {
        return mcp_error(MCP_ERROR_INTERNAL_ERROR, "Out of memory");
    }

    cJSON_AddStringToObject(response, "status", "ok");

    return response;
}

cJSON *mcp_tool_media_record_status(cJSON *params)
{
    cJSON *response = cJSON_CreateObject();

    if (response == NULL) {
        return mcp_error(MCP_ERROR_INTERNAL_ERROR, "Out of memory");
    }

    cJSON_AddBoolToObject(response, "recording", screenshot_is_recording() ? 1 : 0);
    cJSON_AddBoolToObject(response, "warp_mode", vsync_get_warp_mode() ? 1 : 0);

    return response;
}
