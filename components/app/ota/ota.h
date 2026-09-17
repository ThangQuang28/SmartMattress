#ifndef OTA_H
#define OTA_H

#include <stdbool.h>

/**
 * Start an HTTPS firmware update in a background task.
 *
 * The URL is copied before this function returns. Only one update can run at
 * a time. The device restarts only after the downloaded image is validated.
 */
bool ota_start(const char *url);

bool ota_is_in_progress(void);

#endif /* OTA_H */
